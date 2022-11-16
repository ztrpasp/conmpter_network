#include<iostream>
#include<stdio.h>
#include<windows.h>
#include<time.h>
#include<winsock.h>
#include<string.h>
#include <fstream>
#include <sys/stat.h>
#include "rdt.h"
#pragma comment(lib, "ws2_32.lib")
using namespace std;

double MAX_TIMEOUT = 1*CLOCKS_PER_SEC; //超时重传时间由于是本机上不同端口上数据传输，先初始化为1s


//连接三次握手s
bool ConServer(SOCKET socket, SOCKADDR_IN serverAddr)
{
    //第一次握手，只需要发送协议头
    int addrLen = sizeof(serverAddr);
    RDTHead clientSYN;
    setSYN(clientSYN.flag);
    clientSYN.checkSum=CalcheckSum((unsigned short *)&clientSYN, sizeof(clientSYN));
    char buffer[sizeof(clientSYN)];
    memset(buffer, 0, sizeof(clientSYN));
    memcpy(buffer, &clientSYN, sizeof(clientSYN));
    sendto(socket, buffer, sizeof(clientSYN), 0, (SOCKADDR *) &serverAddr, addrLen);
    cout << "第一次握手" << endl;

    //第二次握手
    RDTHead serverSYN_ACK;
    clock_t start =clock();

    u_long mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);//设置为非阻塞

    while (recvfrom(socket, buffer, sizeof(serverSYN_ACK), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout<<"第一次握手超时重传"<<endl;
            memcpy(buffer, &clientSYN, sizeof(clientSYN));
            sendto(socket, buffer, sizeof(clientSYN), 0, (SOCKADDR *) &serverAddr, addrLen);
            start = clock();
        }
    }

    
    memcpy(&serverSYN_ACK, buffer, sizeof(serverSYN_ACK));
    if (isSYN_ACK(serverSYN_ACK.flag)&& (CalcheckSum((u_short *) &serverSYN_ACK, sizeof(RDTHead)) == 0)) {
        cout << "第二次握手成功" << endl;
    } else {
        cout << "第二次握手失败" << endl;
        return false;
    }

    //第三次握手
    RDTHead clientACK;
    setACK(clientACK.flag);
    clientACK.checkSum=CalcheckSum((unsigned short *)&clientACK, sizeof(clientACK));
    memcpy(buffer, &clientACK, sizeof(clientACK));
    if(sendto(socket, buffer, sizeof(clientACK), 0, (SOCKADDR *) &serverAddr, addrLen)==-1)
    {
        return false;
    }


    start = clock();
    while (clock() - start <= 2 * MAX_TIMEOUT) {
        if (recvfrom(socket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0)
            continue;
        //第三次握手确认包丢失
        memcpy(buffer, &clientACK, sizeof(RDTHead));
        sendto(socket, buffer, sizeof(RDTHead), 0, (sockaddr *) &serverAddr, addrLen);
        cout<<"第三次握手超时重传"<<endl;
        start = clock();
    }


    cout<<"第三次握手成功连接"<<endl;
    u_long imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//阻塞
    return true;
}




void send(char *fileBuffer, size_t filelen, SOCKET &socket, SOCKADDR_IN &addr) {

    u_long imode = 1;
    ioctlsocket(socket, FIONBIO, &imode); //先进入非阻塞模式
    
    int packetNum = int(filelen / MAX_DATA_SIZE); int remain = filelen % MAX_DATA_SIZE ? 1 : 0;
    packetNum += remain;
    int num = 0;  //数据包的索引                      
    int stage = 0; //有限自动状态机
    int addrLen = sizeof(addr);

    char *dataBuffer = new char[MAX_DATA_SIZE], *pktBuffer = new char[sizeof(RDTPacket)];
    RDTPacket sendPkt, rcvPkt;   

    cout <<"总共需要传输"<< packetNum << "个数据包" <<endl;

    clock_t start;
    while (true) {
        int dataSize;
        if (num == packetNum) {
            RDTHead endHead;
            setEND(endHead.flag);
            endHead.checkSum = CalcheckSum((u_short *) &endHead, sizeof(RDTHead));
            memcpy(pktBuffer, &endHead, sizeof(RDTHead));
            sendto(socket, pktBuffer, sizeof(RDTHead), 0, (SOCKADDR *) &addr, addrLen);

            while (recvfrom(socket, pktBuffer, sizeof(RDTHead), 0, (SOCKADDR *) &addr, &addrLen) <= 0) {
                if (clock() - start >= MAX_TIMEOUT) {
                    memcpy(pktBuffer, &endHead, sizeof(RDTHead));
                    sendto(socket, pktBuffer, sizeof(RDTHead), 0, (SOCKADDR *) &addr, addrLen);
                    start = clock();
                }
            }

            RDTHead serverACK;
            memcpy(&serverACK,pktBuffer,sizeof(RDTHead));

            if(isACK(serverACK.flag))
                cout<<"文件传输完成"<<endl;
            return;
        }
        switch (stage) {
            case 0:
                dataSize = MAX_DATA_SIZE;
                if((num+1)*MAX_DATA_SIZE>filelen)//
                {
                    dataSize = filelen-num*MAX_DATA_SIZE;
                }
                memcpy(dataBuffer, fileBuffer + num * MAX_DATA_SIZE, dataSize);
                sendPkt = mkPacket(0, dataBuffer, dataSize);

                memcpy(pktBuffer, &sendPkt, sizeof(RDTPacket));
                sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                start = clock();//计时
                stage = 1;
                break;
            case 1:
                //超时的情况
                while (recvfrom(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, &addrLen) <= 0) {
                    if (clock() - start >= MAX_TIMEOUT) {
                        sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                        cout << num << "号数据包超时重传" << endl;
                        start = clock();
                    }
                }
                memcpy(&rcvPkt, pktBuffer, sizeof(RDTPacket));
                
                //收到重复的包或者校验和错误
                if (rcvPkt.head.ack == 1 || CalcheckSum((u_short *) &rcvPkt, sizeof(RDTPacket)) != 0) {
                    stage = 1;
                    break;
                }
                
                if (rcvPkt.head.ack == 0 || CalcheckSum((u_short *) &rcvPkt, sizeof(RDTPacket)) == 0) {
                    stage = 2;
                    num++;
                    break;
                }
                break;
                
            case 2:
                dataSize = MAX_DATA_SIZE;
                if((num+1)*MAX_DATA_SIZE>filelen)//
                {
                    dataSize = filelen-num*MAX_DATA_SIZE;
                }
                memcpy(dataBuffer, fileBuffer + num * MAX_DATA_SIZE, dataSize);

                sendPkt = mkPacket(1, dataBuffer, dataSize);
                memcpy(pktBuffer, &sendPkt, sizeof(RDTPacket));
                sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                start = clock();
                stage = 3;
                break;
            case 3:
                //超时情况
                while (recvfrom(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, &addrLen) <= 0) {
                    if (clock() - start >= MAX_TIMEOUT) {
                        sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                        cout << num << "号数据包超时重传" << endl;
                        start = clock();
                    }
                }
                memcpy(&rcvPkt, pktBuffer, sizeof(RDTPacket));
                //收到重复的包或者校验和错误
                if (rcvPkt.head.ack == 0 || CalcheckSum((u_short *) &rcvPkt, sizeof(RDTPacket)) != 0) {
                    stage = 3;
                    break;
                }
                if (rcvPkt.head.ack == 0 || CalcheckSum((u_short *) &rcvPkt, sizeof(RDTPacket)) == 0) {
                    stage = 0;
                    num++;
                    break;
                }
                break;
        }
    }
}




bool DisConServer(SOCKET clientSocket, SOCKADDR_IN serverAddr) {

    int addrLen = sizeof(serverAddr);
    char buffer[sizeof(RDTHead)];
    RDTHead clientFIN;
    setFIN_ACK(clientFIN.flag);
    clientFIN.checkSum = CalcheckSum((u_short *) &clientFIN, sizeof(RDTHead));


    memcpy(buffer, &clientFIN, sizeof(RDTHead));
    sendto(clientSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &serverAddr, addrLen);
    cout<<"客户端发起第一次挥手进入FIN-WAIT-1状态"<<endl;
    unsigned long imode = 1;
    ioctlsocket(clientSocket, FIONBIO, &imode); //改为非阻塞模式

    clock_t start = clock();
    while (recvfrom(clientSocket, buffer, sizeof(RDTHead), 0, (sockaddr *) &serverAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout<<"第一次挥手超时重传"<<endl;
            memcpy(buffer, &clientFIN, sizeof(RDTHead));
            sendto(clientSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &serverAddr, addrLen);
            start = clock();
        }
    }

    RDTHead serverACK;
    memcpy(&serverACK, buffer, sizeof(RDTHead));

    if ((isACK(serverACK.flag)) && (CalcheckSum((u_short *) buffer, sizeof(RDTHead) == 0))) {
        cout << "客户端进入FIN-WAIT-2状态" << endl;
    } else {
        cout << "错误" << endl;
        return false;
    }

    imode = 0;
    ioctlsocket(clientSocket, FIONBIO, &imode);//阻塞

    recvfrom(clientSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &serverAddr, &addrLen);
    RDTHead serverFIN;
    memcpy(&serverFIN, buffer, sizeof(RDTHead));
    if ((isFIN_ACK(serverFIN.flag)) && (CalcheckSum((u_short *) buffer, sizeof(RDTHead) == 0))) {
        cout << "第三次挥手" << endl;
    } else {
        cout << "错误" << endl;
        return false;
    }

    imode = 1;
    ioctlsocket(clientSocket, FIONBIO, &imode);

    RDTHead clientACK;
    setACK(clientACK.flag);

    sendto(clientSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &serverAddr, addrLen);
    start = clock();
    while (clock() - start <= 2 * MAX_TIMEOUT) {
        if (recvfrom(clientSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0)
            continue;
        //确认包丢失
        memcpy(buffer, &clientACK, sizeof(RDTHead));
        sendto(clientSocket, buffer, sizeof(RDTHead), 0, (sockaddr *) &serverAddr, addrLen);
        start = clock();
    }

    cout << "连接成功关闭" << endl;
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
    cout << "请输入需要传输的文件名" << endl;
    cin >> path;
    
    strcat(fileName,path);
    ifstream myfile(fileName, ifstream::binary);
    if (!myfile.is_open())
    {
        cout << "路径错误" << endl;
        return 0;
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