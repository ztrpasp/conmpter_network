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
    string pattern = "|";               //字符串间隔用"|"表示
    string strs = str + pattern;
    size_t pos = strs.find(pattern);
    int i=0;
    while(pos != strs.npos)
    {
        string temp = strs.substr(0, pos);
        
        //去掉已分割的字符串,在剩下的字符串中进行分割
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
    cout<<"请输入你的名字: ";
    cin>>name;
    int len = send(*socket, name, strlen(name), 0);
    char sendData[1024];
    char acceptName[nameSize];
    char info[500];
    char sendTime[30];
    cout<<"发送消息形式: "<<"收信人 消息内容                ";
    cout<<"键入quit退出聊天"<<endl;
    while (true) {
        
        //获取当前时间
        time_t timep;
        time (&timep);
        strftime(sendTime, sizeof(sendTime), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
        cin>>acceptName;
        //判断用户是否退出
        if(strcmp("quit",acceptName)==0)
        {
            strcpy(sendData,name);strcat(sendData,"|");strcat(sendData,"ALL");strcat(sendData,"|");
            strcat(sendData,sendTime);strcat(sendData,"|");strcat(sendData,"quit");
            cout<<sendTime<<"   您已成功退出聊天"<<endl;
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
        //对字符串进行分割处理
        //cout<<recvData<<endl;
        if(len>0)
            proStr(recvData);
        memset(recvData, 0, 1024);
    }
}
int main()
{
    WSADATA wsaData;//WSAStartup函数调用后返回的Windows Sockets数据的数据结构 
    WSAStartup(MAKEWORD(2, 2), &wsaData);//声明使用socket2.2版本
    //创建套接字
    SOCKET clientSocket;
    //地址类型为AD_INET，服务类型为流式(SOCK_STREAM)，协议采用TCPs
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        //套接字创建失败；
        WSACleanup();
        return 0;
    }
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;      //IP格式
    USHORT uPort = 8888;
    serverAddr.sin_port = htons(uPort);   //绑定端口号
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    if (connect(clientSocket, (SOCKADDR *) &serverAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) { //建立连接
        cout << "连接失败" << endl;
        system("pause");
        return 0;
    }
    else{cout<<"连接成功"<<endl;}




    HANDLE hthread[2];
    hthread[0] = CreateThread(NULL, 0, clientRecv, (LPVOID) &clientSocket, 0, NULL);
    hthread[1] = CreateThread(NULL, 0, clientSend, (LPVOID) &clientSocket, 0, NULL);

    while(true);    //循环，保证程序不会结束。
    // CloseHandle(hthread[0]);
    // CloseHandle(hthread[1]);
    system("pause");
    closesocket(clientSocket);


    WSACleanup();

    return 0;
}