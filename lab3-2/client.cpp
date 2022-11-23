#include<iostream>
#include<stdio.h>
#include<windows.h>
#include<time.h>
#include<winsock.h>
#include<string.h>
#include <fstream>
#include <sys/stat.h>
#include "GBN.h"
#pragma comment(lib, "ws2_32.lib")
using namespace std;

double MAX_TIMEOUT = 1*CLOCKS_PER_SEC; //超时重传时间由于是本机上不同端口上数据传输，先初始化为1s
double DELAY_ACK = 0.5*CLOCKS_PER_SEC; //延迟ACK定时器
 

//三次握手建立连接，只需要发送协议头部分。
bool ConServer(SOCKET socket, SOCKADDR_IN serverAddr)
{
    //第一次握手
    int addrLen = sizeof(serverAddr);
    Head clientSYN;
    setSYN(clientSYN.flag);
    
    clientSYN.checkSum=CalcheckSum((unsigned short *)&clientSYN, sizeof(clientSYN));
    char buffer[sizeof(clientSYN)];
    memset(buffer, 0, sizeof(clientSYN));
    sendHead(buffer,&clientSYN,socket,serverAddr);
    cout << "第一次握手,进入SYN_SEND状态" << endl;
    //第二次握手
    Head serverSYN_ACK;
    u_long mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);//设置为非阻塞
    clock_t start =clock();
    while (recvfrom(socket, buffer, sizeof(serverSYN_ACK), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout<<"第一次握手超时重传"<<endl;
            memcpy(buffer, &clientSYN, sizeof(clientSYN));
            sendto(socket, buffer, sizeof(clientSYN), 0, (SOCKADDR *) &serverAddr, addrLen);
            start = clock();
        }
    }

    
    memcpy(&serverSYN_ACK, buffer, sizeof(serverSYN_ACK));
    if (isSYN_ACK(serverSYN_ACK.flag)&& (CalcheckSum((u_short *) &serverSYN_ACK, sizeof(Head)) == 0)) {
        cout << "第二次握手成功" << endl;
    } else {
        cout << "第二次握手失败" << endl;
        return false;
    }

    //第三次握手
    Head clientACK;
    setACK(clientACK.flag);
    clientACK.checkSum=CalcheckSum((unsigned short *)&clientACK, sizeof(clientACK));
    memcpy(buffer, &clientACK, sizeof(clientACK));
    if(sendto(socket, buffer, sizeof(clientACK), 0, (SOCKADDR *) &serverAddr, addrLen)==-1)
    {
        return false;
    }

    cout<<"第三次握手进入TIME-WAIT状态"<<endl;
    start = clock();
    while (clock() - start <= 2 * MAX_TIMEOUT) {
        if (recvfrom(socket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0)
            continue;
        //第三次握手确认包丢失
        memcpy(buffer, &clientACK, sizeof(Head));
        sendto(socket, buffer, sizeof(Head), 0, (sockaddr *) &serverAddr, addrLen);
        cout<<"第三次握手超时重传"<<endl;
        start = clock();
    }


    cout<<"第三次握手进入Established状态"<<endl;
    u_long imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//阻塞
    return true;
}


void sendWindow(char *fileBuffer, int filelen,int &packetNum, int &num, SOCKET &socket,SOCKADDR_IN &addr,clock_t &start)
{
    char *dataBuffer = new char[MAX_DATA_SIZE], *pktBuffer = new char[sizeof(Packet)];
    int dataSize;
    int addrLen = sizeof(addr); 
    while(num<packetNum)
    {
        if(nextseqnum<base+windowSize)
        {
            dataSize = MAX_DATA_SIZE;
            if((num+1)*MAX_DATA_SIZE>filelen)//
            {
                dataSize = filelen-num*MAX_DATA_SIZE;
            }
            memcpy(dataBuffer, fileBuffer + num * MAX_DATA_SIZE, dataSize);
            Packet sendPkt = mkPacket(nextseqnum,dataBuffer,dataSize);
            memcpy(pktBuffer, &sendPkt, sizeof(Packet));
            sendto(socket, pktBuffer, sizeof(Packet), 0, (SOCKADDR *) &addr, addrLen);

            if (base == nextseqnum) {
                start = clock();
            }
            nextseqnum++;
            num++;
            cout << num << "号数据包已经发送" << endl;
        }
        else break;
    }
    
}

void send(char *fileBuffer, size_t filelen, SOCKET &socket, SOCKADDR_IN &addr) {

    u_long imode = 1;
    ioctlsocket(socket, FIONBIO, &imode); //先进入非阻塞模式
    
    int packetNum = int(filelen / MAX_DATA_SIZE); 
    int remain = filelen % MAX_DATA_SIZE ? 1 : 0;
    packetNum += remain;
    cout <<"总共需要传输"<< packetNum << "个数据包" <<endl;

    int num = 0;//数据包的索引                      
    int addrLen = sizeof(addr); 
    clock_t start;

    char *dataBuffer = new char[MAX_DATA_SIZE], *pktBuffer = new char[sizeof(Packet)];
    Packet rcvPkt;    
    int acceptACK = -1;
    while(true)
    {

        sendWindow(fileBuffer,filelen,packetNum,num,socket,addr,start);
        while (recvfrom(socket, pktBuffer, sizeof(Packet), 0, (SOCKADDR *) &addr, &addrLen) <= 0 ) {
            if (clock() - start >= MAX_TIMEOUT) {
                cout<<"超时重传"<<num-(nextseqnum-base)+1<<"到"<<num<<endl;
                int tmp = num-(nextseqnum-base);
                nextseqnum = base;
                start = clock();
                sendWindow(fileBuffer,filelen,packetNum,tmp,socket,addr,start);
            }
        }

        memcpy(&rcvPkt, pktBuffer, sizeof(Packet));

        if (CalcheckSum((u_short *) &rcvPkt, sizeof(Packet)) != 0 )
            continue;   //do nothing 

        //cout << "base:  " << base << "nextSeq: " << nextseqnum << "endWindow:   " << base + windowSize << endl;
        
        if(acceptACK == rcvPkt.head.ack)//忽略相同的ACK
        {
            cout<<"收到重复的ACK"<<acceptACK<<endl;
        }
        else
        {
            cout<<"收到确认 "<<rcvPkt.head.ack<<"       ";
            base = rcvPkt.head.ack+1;
            start  = clock();
            acceptACK=rcvPkt.head.ack;
        }
        cout << "base:  " << base << "nextSeq: " << nextseqnum << "endWindow:   " << base + windowSize << endl;

        if (rcvPkt.head.ack == packetNum) {
            Head endPacket;
            setEND(endPacket.flag);
            endPacket.checkSum = CalcheckSum((u_short *) &endPacket, sizeof(endPacket));
            memcpy(pktBuffer, &endPacket, sizeof(endPacket));
            sendto(socket, pktBuffer, sizeof(endPacket), 0, (SOCKADDR *) &addr, addrLen);

            while (recvfrom(socket, pktBuffer, sizeof(endPacket), 0, (SOCKADDR *) &addr, &addrLen) <= 0) {
                if (clock() - start >= MAX_TIMEOUT) {
                    start = clock();
                    goto resend;
                }
            }

            if (((Head *) (pktBuffer))->flag & ACK &&
                CalcheckSum((u_short *) pktBuffer, sizeof(Head)) == 0) {
                cout << "文件传输完成" << endl;
                return;
            }
            resend:
            continue;
        }
    }
    
}




bool DisConServer(SOCKET clientSocket, SOCKADDR_IN serverAddr) {

    int addrLen = sizeof(serverAddr);
    char buffer[sizeof(Head)];
    Head clientFIN;
    setFIN_ACK(clientFIN.flag);
    clientFIN.checkSum = CalcheckSum((u_short *) &clientFIN, sizeof(Head));


    memcpy(buffer, &clientFIN, sizeof(Head));
    sendto(clientSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, addrLen);
    cout<<"客户端发起第一次挥手进入FIN-WAIT-1状态"<<endl;
    unsigned long imode = 1;
    ioctlsocket(clientSocket, FIONBIO, &imode); //改为非阻塞模式

    clock_t start = clock();
    while (recvfrom(clientSocket, buffer, sizeof(Head), 0, (sockaddr *) &serverAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout<<"第一次挥手超时重传"<<endl;
            memcpy(buffer, &clientFIN, sizeof(Head));
            sendto(clientSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, addrLen);
            start = clock();
        }
    }

    Head serverACK;
    memcpy(&serverACK, buffer, sizeof(Head));

    if ((isACK(serverACK.flag)) && (CalcheckSum((u_short *) buffer, sizeof(Head) == 0))) {
        cout << "客户端进入FIN-WAIT-2状态" << endl;
    } else {
        cout << "错误" << endl;
        return false;
    }

    imode = 0;
    ioctlsocket(clientSocket, FIONBIO, &imode);//阻塞

    recvfrom(clientSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, &addrLen);
    Head serverFIN;
    memcpy(&serverFIN, buffer, sizeof(Head));
    if ((isFIN_ACK(serverFIN.flag)) && (CalcheckSum((u_short *) buffer, sizeof(Head) == 0))) {
        cout << "第三次挥手" << endl;
    } else {
        cout << "错误" << endl;
        return false;
    }

    imode = 1;
    ioctlsocket(clientSocket, FIONBIO, &imode);

    Head clientACK;
    setACK(clientACK.flag);

    sendto(clientSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, addrLen);
    start = clock();
    cout<<"第四次挥手进入TIME-WAIT状态"<<endl;
    while (clock() - start <= 2 * MAX_TIMEOUT) {
        if (recvfrom(clientSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0)
            continue;
        //确认包丢失
        cout<<"第四次挥手超时重传"<<endl;
        memcpy(buffer, &clientACK, sizeof(Head));
        sendto(clientSocket, buffer, sizeof(Head), 0, (sockaddr *) &serverAddr, addrLen);
        start = clock();
    }

    cout << "第四次挥手进入closed状态" << endl;
    closesocket(clientSocket);
    return true;
}

int main()
{
    WSADATA wsaData;//WSAStartup函数调用后返回的Windows Sockets数据的数据结构 
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    SOCKET clientSocket; //创建套接字
    //地址类型为AD_INET，服务类型为数据报，协议采用
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        //套接字创建失败；
        cout<<"套接字创建失败"<<endl; WSACleanup();
        return 0;
    }
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;      //IP格式
    USHORT uPort = 8887;                  //写死端口号
    serverAddr.sin_port = htons(uPort);   //绑定端口号
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    if (!ConServer(clientSocket, serverAddr)) {
        cout << "连接失败" << endl;
        return 0;
    }
    char fileName[150]="D:\\cpp_vscode\\conmpter_network\\lab3-1\\测试文件\\"; char path[50];
    ifstream myfile;
    while(true)
    {
        cout << "请输入需要传输的文件名" << endl;
        cin >> path;
    
        strcat(fileName,path);
        myfile.open(fileName, ifstream::binary);
        if (!myfile.is_open())
        {
            cout << "路径错误,请重新确认路径" << endl;
        }
        else break;
    }
    
    
    // 这是一个存储文件(夹)信息的结构体，其中有文件大小和创建时间、访问时间、修改时间等
	struct stat statbuf;
	// 提供文件名字符串，获得文件属性结构体
	stat(fileName, &statbuf);
	// 获取文件大小
	size_t fileLen = statbuf.st_size;

    char *fileBuffer = new char[fileLen];
    myfile.read(fileBuffer, fileLen);
    myfile.close();
    cout << "开始进行传输, 文件大小为:  "<<fileLen<< endl;

    send(fileBuffer, fileLen, clientSocket, serverAddr);

    if (!DisConServer(clientSocket, serverAddr)) {
        cout << "关闭连接失败" << endl;
        return 0;
    }
    cout<<"成功关闭连接"<<endl;

    system("pause");
    return 0;
}