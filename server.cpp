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
int num=0;//连接数
int i=0;//
//获取时间
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
    int id;//id号
    int valid;//是否有效
    char *name;//姓名
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
    cout<<sendName<<" 在 "<<sendTime<<" 向 "<<acceptName<<" 发送了一条消息"<<endl;
    if(info=="quit")
    {
        cout<<sendTime<<"   "<<sendName<<"退出聊天"<<endl;
        clients[name_id[sendName]].valid=0;
    }
    if(acceptName=="ALL")
    {
        //实现群发功能
        for (auto iter = name_id.begin(); iter != name_id.end(); ++iter) {//群发
			if (iter->second == name_id[sendName]||clients[iter->second].valid==0)continue;
			send(clients[iter->second].scoket, recvData, strlen(recvData), 0);
		}
    }
    else
    {
        map<string,int>::iterator iter = name_id.find(acceptName);
        //群发与转发消息，等待实现
        if(iter != name_id.end()&&clients[iter->second].valid==1) 
            send(clients[iter->second].scoket,recvData,strlen(recvData),0);
        else    //当找不到用户时，返回报错信息，等待实现
        cout<<sendTime<<"   "<<"找不到用户"<<endl;
        //当用户valid为零时，等待实现
    }
}


DWORD WINAPI clien_run(LPVOID lparam) {
    myClient * client = (myClient*) lparam;
    //接受name并将与id的映射存储在map数据结构中
    char name [nameSize];
    memset(name,0,nameSize);
    int len = recv(client->scoket, name, nameSize-1, 0); //接收用户名
    if(len==0)
    {
         cout<<"连接已经关闭"<<endl;
    }
    name[len] = '\0';
    client->name = name;
    
    cout<<onlineTime<<"    "<<client->name<<"上线"<<endl;
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
            proStr(recvData);//实现消息分析、转发、群发等功能；
        }
        memset(recvData,0,1024);
    }
}
int main()
{
    HANDLE timeThread = CreateThread(NULL,0,updateTime,LPVOID(),0,NULL);
    WSADATA wsaData;//WSAStartup函数调用后返回的Windows Sockets数据的数据结构 
    WSAStartup(MAKEWORD(2, 2), &wsaData);//声明使用socket2.2版本
    //建立服务器套接字
    SOCKET serverSocket;
    //地址类型为AD_INET，服务类型为流式(SOCK_STREAM)，协议采用TCPs
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
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
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)  //建立连接
    {
        cout<<"绑定失败"<<endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    listen(serverSocket, maxClient);
    name_id["ALL"]=INT32_MAX;       //设计所有人的map映射
    while(num<maxClient)
    {   
        SOCKADDR_IN clientAddr;  
        int clientAddrlen = sizeof(clientAddr);
        clients[num].scoket = accept(serverSocket, (SOCKADDR*)&clientAddr,&clientAddrlen);
        clients[num].id=num;
        clients[num].valid=1;
        if (clients[num].scoket == SOCKET_ERROR) 
        {
            cout << "连接失败" << endl;
            return 0;
        }
        HANDLE hThread;
        hThread = CreateThread(NULL, 0, clien_run, (LPVOID)&clients[num], 0, NULL);
        num++;
    }
    while(true);//不关闭主进程
    closesocket(serverSocket);
    WSACleanup();
    //关
    return 0;
}