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
    cout<<"第三次握手成功连接"<<endl;
    u_long imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//阻塞
    return true;
}




void sendFSM(char *fileBuffer, size_t filelen, SOCKET &socket, SOCKADDR_IN &addr) {

    u_long imode = 1;
    ioctlsocket(socket, FIONBIO, &imode); //先进入非阻塞模式
    
    int packetNum = int(filelen / MAX_DATA_SIZE); int remain = filelen % MAX_DATA_SIZE ? 1 : 0;
    packetNum += remain;
    int num = 0;  //数据包的索引                      
    int stage = 0; //有限自动状态机
    int addrLen = sizeof(addr);

    char *dataBuffer = new char[MAX_DATA_SIZE], *pktBuffer = new char[sizeof(RDTPacket)];
    cout <<"需要传输"<< packetNum << "个数据包" <<endl;
    RDTPacket sendPkt, rcvPkt;   

    clock_t start;
    while (true) {
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
                int dataSize;
                if((num+1)*MAX_DATA_SIZE>filelen)//
                {
                    dataSize = filelen-num*MAX_DATA_SIZE;
                    memcpy(dataBuffer, fileBuffer + num * MAX_DATA_SIZE, dataSize);
                }
                else 
                {
                    memcpy(dataBuffer, fileBuffer + num * MAX_DATA_SIZE, MAX_DATA_SIZE);
                    dataSize = MAX_DATA_SIZE;
                }
                sendPkt = mkPacket(0, dataBuffer, dataSize);

                memcpy(pktBuffer, &sendPkt, sizeof(RDTPacket));
                sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                start = clock();//计时
                stage = 1;
                break;
            case 1:
                //time_out
                while (recvfrom(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR *) &addr, &addrLen) <= 0) {
                    if (clock() - start >= MAX_TIME) {
                        sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR *) &addr, addrLen);
                        cout << index << "号数据包超时重传" << endl;
                        start = clock();
                    }
                }
                memcpy(&pkt, pkt_buffer, sizeof(packet));
                if (pkt.head.ack == 1 || checkPacketSum((u_short *) &pkt, sizeof(packet)) != 0) {
                    stage = 1;
                    break;
                }
                stage = 2;
                //cout<<index<<"号数据包传输成功，传输了"<<packetDataLen<<"Bytes数据"<<endl;
                index++;
                break;
            case 2:
                memcpy(data_buffer, fileBuffer + index * MAX_DATA_SIZE, packetDataLen);
                sendPkt = makePacket(1, data_buffer, packetDataLen);
                memcpy(pkt_buffer, &sendPkt, sizeof(packet));
                sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR *) &addr, addrLen);

                start = clock();//计时
                stage = 3;
                break;
            case 3:
                //time_out
                while (recvfrom(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR *) &addr, &addrLen) <= 0) {
                    if (clock() - start >= MAX_TIME) {
                        sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR *) &addr, addrLen);
                        cout << index << "号数据包超时重传" << endl;
                        start = clock();
                    }
                }
                memcpy(&pkt, pkt_buffer, sizeof(packet));
                if (pkt.head.ack == 0 || checkPacketSum((u_short *) &pkt, sizeof(packet)) != 0) {
                    stage = 3;
                    break;
                }
                stage = 0;
                //cout<<index<<"号数据包传输成功，传输了"<<packetDataLen<<"Bytes数据"<<endl;
                index++;
                break;
            default:
                cout << "error" << endl;
                return;
        }
    }
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
    USHORT uPort = 8888;                  //写死端口号
    serverAddr.sin_port = htons(uPort);   //绑定端口号
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    // if (!ConServer(clientSocket, serverAddr)) {
    //     cout << "连接失败" << endl;
    //     return 0;
    // }
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

    //sendFSM(fileBuffer, fileLen, clientSocket, serverAddr);


    system("pause");
    return 0;
}