#include<iostream>
#include<stdio.h>
#include<windows.h>
#include<time.h>
#include<winsock.h>
#include<string.h>
#include<map>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
#define maxClient 10
#define nameSize 10
int num=0;//������
int i=0;//
//��ȡʱ��
char onlineTime[20];
DWORD WINAPI updateTime(LPVOID lparam) {
    while(true)
    {
        Sleep(500);
        time_t timep;
        time (&timep);
        strftime(onlineTime, sizeof(onlineTime), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
    }
    return 0;
}
class myClient
{
    public:
    int id;//id��
    int valid;//�Ƿ���Ч
    char *name;//����
    SOCKET scoket;//socket
    myClient();
};
myClient::myClient()
{
    int id = 0;
}
myClient clients[maxClient];
map<string,int> name_id;

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
    cout<<sendName<<" �� "<<sendTime<<" �� "<<acceptName<<" ������һ����Ϣ"<<endl;
    if(info=="quit")
    {
        cout<<sendTime<<"   "<<sendName<<"�˳�����"<<endl;
        clients[name_id[sendName]].valid=0;
    }
    if(acceptName=="ALL")
    {
        //ʵ��Ⱥ������
        for (auto iter = name_id.begin(); iter != name_id.end(); ++iter) {//Ⱥ��
			if (iter->second == name_id[sendName]||clients[iter->second].valid==0)continue;
			send(clients[iter->second].scoket, recvData, strlen(recvData), 0);
		}
    }
    else
    {
        map<string,int>::iterator iter = name_id.find(acceptName);
        //Ⱥ����ת����Ϣ���ȴ�ʵ��
        if(iter != name_id.end()&&clients[iter->second].valid==1) 
            send(clients[iter->second].scoket,recvData,strlen(recvData),0);
        else    //���Ҳ����û�ʱ�����ر�����Ϣ���ȴ�ʵ��
        cout<<sendTime<<"   "<<"�Ҳ����û�"<<endl;
        //���û�validΪ��ʱ���ȴ�ʵ��
    }
}


DWORD WINAPI clien_run(LPVOID lparam) {
    myClient * client = (myClient*) lparam;
    //����name������id��ӳ��洢��map���ݽṹ��
    char name [nameSize];
    memset(name,0,nameSize);
    int len = recv(client->scoket, name, nameSize-1, 0); //�����û���
    if(len==0)
    {
         cout<<"�����Ѿ��ر�"<<endl;
    }
    name[len] = '\0';
    client->name = name;
    
    cout<<onlineTime<<"    "<<client->name<<"����"<<endl;
    name_id[name]=client->id;

    char recvData[1024];
    char sendData[1024];
    while(true)
    {
        int len = recv(client->scoket, recvData, 1023, 0);
        recvData[len]='\0';
        if(len>0)
        {
            //cout<<recvData<<endl;
            proStr(recvData);//ʵ����Ϣ������ת����Ⱥ���ȹ��ܣ�
        }
        memset(recvData,0,1024);
    }
}
int main()
{
    HANDLE timeThread = CreateThread(NULL,0,updateTime,LPVOID(),0,NULL);
    WSADATA wsaData;//WSAStartup�������ú󷵻ص�Windows Sockets���ݵ����ݽṹ 
    WSAStartup(MAKEWORD(2, 2), &wsaData);//����ʹ��socket2.2�汾
    //�����������׽���
    SOCKET serverSocket;
    //��ַ����ΪAD_INET����������Ϊ��ʽ(SOCK_STREAM)��Э�����TCPs
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
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
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)  //��������
    {
        cout<<"��ʧ��"<<endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    listen(serverSocket, maxClient);
    name_id["ALL"]=INT32_MAX;       //��������˵�mapӳ��
    while(num<maxClient)
    {   
        SOCKADDR_IN clientAddr;  
        int clientAddrlen = sizeof(clientAddr);
        clients[num].scoket = accept(serverSocket, (SOCKADDR*)&clientAddr,&clientAddrlen);
        clients[num].id=num;
        clients[num].valid=1;
        if (clients[num].scoket == SOCKET_ERROR) 
        {
            cout << "����ʧ��" << endl;
            return 0;
        }
        HANDLE hThread;
        hThread = CreateThread(NULL, 0, clien_run, (LPVOID)&clients[num], 0, NULL);
        num++;
    }
    while(true);//���ر�������
    closesocket(serverSocket);
    WSACleanup();
    //��
    return 0;
}