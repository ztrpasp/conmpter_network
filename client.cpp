#include<iostream>
#include<stdio.h>
#include<windows.h>
#include<time.h>
#include<winsock.h>
#include<string.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
#define nameSize 10

void proStr(char *recvData)
{
    string sendName,acceptName,sendTime,info;
    string str = recvData;
    string pattern = "|";               //�ַ��������"|"��ʾ
    string strs = str + pattern;
    size_t pos = strs.find(pattern);
    int i=0;
    while(pos != strs.npos)
    {
        string temp = strs.substr(0, pos);
        
        //ȥ���ѷָ���ַ���,��ʣ�µ��ַ����н��зָ�
        strs = strs.substr(pos+1, strs.size());
        pos = strs.find(pattern);
        switch (i)
        {
        case 0:
            sendName = temp;
            break;
        case 1:
            acceptName = temp;
            break;
        case 2:
            sendTime = temp;
        case 3:
            info   = temp;
        default:
            break;
        }
        i++;
    }
    if(acceptName=="ALL")
        cout<<sendTime<<"   "<<sendName<<"---->"<<"ALL"<<": "<<info<<endl;
    else
        cout<<sendTime<<"   "<<sendName<<": "<<info<<endl;
}
DWORD WINAPI clientSend(LPVOID lparam) {
    SOCKET *socket = (SOCKET *) lparam;
    char *name = new char[nameSize];

    memset(name, 0, nameSize);
    cout<<"�������������: ";
    cin>>name;
    int len = send(*socket, name, strlen(name), 0);
    char sendData[1024];
    char acceptName[nameSize];
    char info[500];
    char sendTime[30];
    cout<<"������Ϣ��ʽ: "<<"������ ��Ϣ����                ";
    cout<<"����quit�˳�����"<<endl;
    while (true) {
        
        //��ȡ��ǰʱ��
        time_t timep;
        time (&timep);
        strftime(sendTime, sizeof(sendTime), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
        cin>>acceptName;
        //�ж��û��Ƿ��˳�
        if(strcmp("quit",acceptName)==0)
        {
            strcpy(sendData,name);strcat(sendData,"|");strcat(sendData,"ALL");strcat(sendData,"|");
            strcat(sendData,sendTime);strcat(sendData,"|");strcat(sendData,"quit");
            cout<<sendTime<<"   ���ѳɹ��˳�����"<<endl;
            int len = send(*socket,sendData,strlen(sendData),0);
            return 0;
        }
        cin.getline(info,500);
        
        strcpy(sendData,name);strcat(sendData,"|");strcat(sendData,acceptName);strcat(sendData,"|");
        strcat(sendData,sendTime);strcat(sendData,"|");strcat(sendData,info);
        cout<<sendTime<<"   "<<name<<"---->"<<acceptName<<": "<<info<<endl;
        int len = send(*socket,sendData,strlen(sendData),0);

        memset(sendData, 0, 1024);
    }
}

DWORD WINAPI clientRecv(LPVOID lparam) {
    SOCKET *socket = (SOCKET *) lparam;
    char recvData[1024];
    memset(recvData, 0, 1024);
    while (true) {
        int len = recv(*socket, recvData, 1023, 0);
        //���ַ������зָ��
        //cout<<recvData<<endl;
        if(len>0)
            proStr(recvData);
        memset(recvData, 0, 1024);
    }
}
int main()
{
    WSADATA wsaData;//WSAStartup�������ú󷵻ص�Windows Sockets���ݵ����ݽṹ 
    WSAStartup(MAKEWORD(2, 2), &wsaData);//����ʹ��socket2.2�汾
    //�����׽���
    SOCKET clientSocket;
    //��ַ����ΪAD_INET����������Ϊ��ʽ(SOCK_STREAM)��Э�����TCPs
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        //�׽��ִ���ʧ�ܣ�
        WSACleanup();
        return 0;
    }
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;      //IP��ʽ
    USHORT uPort = 8888;
    serverAddr.sin_port = htons(uPort);   //�󶨶˿ں�
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    if (connect(clientSocket, (SOCKADDR *) &serverAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) { //��������
        cout << "����ʧ��" << endl;
        system("pause");
        return 0;
    }
    else{cout<<"���ӳɹ�"<<endl;}




    HANDLE hthread[2];
    hthread[0] = CreateThread(NULL, 0, clientRecv, (LPVOID) &clientSocket, 0, NULL);
    hthread[1] = CreateThread(NULL, 0, clientSend, (LPVOID) &clientSocket, 0, NULL);

    while(true);    //ѭ������֤���򲻻������
    // CloseHandle(hthread[0]);
    // CloseHandle(hthread[1]);
    system("pause");
    closesocket(clientSocket);


    WSACleanup();

    return 0;
}