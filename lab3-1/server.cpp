#include<iostream>
#include<stdio.h>
#include<windows.h>
#include<time.h>
#include<winsock.h>
#include<string.h>
#include <fstream>
#include "rdt.h"
#pragma comment(lib, "ws2_32.lib")

#define MAX_FILE_SIZE 15000000
double MAX_TIMEOUT = 1*CLOCKS_PER_SEC;

bool AcClient(SOCKET socket, SOCKADDR_IN addr)
{
    //第一次握手
    char buffer[sizeof(RDTHead)];
    int len = sizeof(addr);
    recvfrom(socket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &addr, &len);
    RDTHead clientSYN;
    memcpy(&clientSYN, buffer, sizeof(clientSYN));
    if (isSYN(clientSYN.flag) && (CalcheckSum((u_short *)&clientSYN, sizeof(RDTHead)) == 0))
        cout << "第一次握手成功" << endl;

    //第二次握手
    RDTHead serverSYN_ACK;
    setSYN_ACK(serverSYN_ACK.flag);
    serverSYN_ACK.checkSum = CalcheckSum((u_short *) &serverSYN_ACK, sizeof(RDTHead));
    memcpy(buffer, &serverSYN_ACK, sizeof(RDTHead));
    sendto(socket, buffer, sizeof(RDTHead), 0, (sockaddr *) &addr, len);
    cout<<"第二次握手"<<endl;

    u_long imode = 1;
    ioctlsocket(socket, FIONBIO, &imode);//非阻塞
    clock_t start = clock(); //开始计时
    while (recvfrom(socket, buffer, sizeof(RDTHead), 0, (sockaddr *) &addr, &len) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout << "第二次握手超时重传" << endl;
            sendto(socket, buffer, sizeof(buffer), 0, (sockaddr *) &addr, len);
            start = clock();
        }
    }

    RDTHead clientACK;
    memcpy(&clientACK, &buffer, sizeof(RDTHead));
    if (isACK(clientACK.flag) && (CalcheckSum((u_short *) buffer, sizeof(RDTHead)) == 0)) {
        cout << "第三次握手成功" << endl;
    } else {
        cout << "第三次握手失败" << endl;
        return false;
    }

    imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//换为阻塞模式
    return true;
}

bool recv(char *fileBuffer, SOCKET &socket, SOCKADDR_IN &addr, unsigned long &filelen) {
    int stage = 0; //状态
    int num = 0;   //数据包个数
    int dataSize;   //数据包数据段长度
    int addrLen = sizeof(addr);
    char *pktBuffer = new char[sizeof(RDTPacket)];
    RDTPacket rcvPkt, sendPkt;
    RDTHead overHead;
    while (true) {
        memset(pktBuffer, 0, sizeof(RDTPacket));
        switch (stage) {
            case 0:
                //先确认是不是发送的结束包
                recvfrom(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, &addrLen);
                memcpy(&overHead, pktBuffer, sizeof(RDTHead));

                if (isEND(overHead.flag)) {
                    cout << "传输完毕" << endl;
                    RDTHead endPacket;
                    setACK(endPacket.flag);
                    endPacket.checkSum = CalcheckSum((u_short *) &endPacket, sizeof(RDTHead));
                    memcpy(pktBuffer, &endPacket, sizeof(RDTHead));
                    sendto(socket, pktBuffer, sizeof(RDTHead), 0, (SOCKADDR *) &addr, addrLen);
                    return true;
                }

                memcpy(&rcvPkt, pktBuffer, sizeof(RDTPacket));

                //校验位不正确或收到重复的包
                if (rcvPkt.head.seq == 1 || CalcheckSum((u_short *) &rcvPkt, sizeof(RDTPacket)) != 0) {
                    sendPkt = mkPacket(1);
                    memcpy(pktBuffer, &sendPkt, sizeof(RDTPacket));
                    sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                    stage = 0;
                    cout << num  << "号数据包重复或损坏, 抛弃" << endl;
                    break;
                }


                //收到正确的数据包
                if (rcvPkt.head.seq == 0 || CalcheckSum((u_short *) &rcvPkt, sizeof(RDTPacket)) == 0)
                {
                    dataSize = rcvPkt.head.dataSize;
                    memcpy(fileBuffer + filelen, rcvPkt.data, dataSize);
                    filelen += dataSize;
                    //发送确认包
                    sendPkt = mkPacket(0);
                    memcpy(pktBuffer, &sendPkt, sizeof(RDTPacket));
                    sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                    stage = 1;
                    num++;
                    break;
                }
            
                
                break;
            case 1:
                //先确认是不是发送的结束包
                recvfrom(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, &addrLen);
                memcpy(&overHead, pktBuffer, sizeof(RDTHead));

                if (isEND(overHead.flag)) {
                    cout << "传输完毕" << endl;
                    RDTHead endPacket;
                    setACK(endPacket.flag);
                    endPacket.checkSum = CalcheckSum((u_short *) &endPacket, sizeof(RDTHead));
                    memcpy(pktBuffer, &endPacket, sizeof(RDTHead));
                    sendto(socket, pktBuffer, sizeof(RDTHead), 0, (SOCKADDR *) &addr, addrLen);
                    return true;
                }

                memcpy(&rcvPkt, pktBuffer, sizeof(RDTPacket));

                if (rcvPkt.head.seq == 0 || CalcheckSum((u_short *) &rcvPkt, sizeof(RDTPacket)) != 0) {
                    sendPkt = mkPacket(0);
                    memcpy(pktBuffer, &sendPkt, sizeof(RDTPacket));
                    sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                    stage = 1;
                    cout << num<< "号数据包重复或损坏, 抛弃" << endl;
                    break;
                }

                //正确接收的情况
                if (rcvPkt.head.seq == 1 || CalcheckSum((u_short *) &rcvPkt, sizeof(RDTPacket)) == 0) {
                    dataSize = rcvPkt.head.dataSize;
                    memcpy(fileBuffer + filelen, rcvPkt.data, dataSize);
                    filelen += dataSize;
                    //发送确认包
                    sendPkt = mkPacket(1);
                    memcpy(pktBuffer, &sendPkt, sizeof(RDTPacket));
                    sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                    stage = 0;
                    num++;
                    break;
                }
                break;
        }
    }
}
bool DisConClient(SOCKET &serverSocket, SOCKADDR_IN &clientAddr) {
    int addrLen = sizeof(clientAddr);
    char *buffer = new char[sizeof(RDTHead)];

    recvfrom(serverSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &clientAddr, &addrLen);

    RDTHead clientFIN;
    memcpy(&clientFIN,buffer,sizeof(RDTHead));

    if ((isFIN_ACK) && (CalcheckSum((u_short *) buffer, sizeof(RDTHead) == 0))) {
        cout << "客户端请求断开" << endl;
    } else {
        cout << "错误" << endl;
        return false;
    }

    RDTHead serverACK;
    setACK(serverACK.flag);
    serverACK.checkSum = CalcheckSum((u_short *) &serverACK, sizeof(RDTHead));
    memcpy(buffer, &serverACK, sizeof(RDTHead));
    sendto(serverSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &clientAddr, addrLen);

    RDTHead serverFIN;
    setFIN_ACK(serverFIN.flag);
    serverFIN.checkSum = CalcheckSum((u_short *) &serverFIN, sizeof(RDTHead));
    memcpy(buffer, &serverFIN, sizeof(RDTHead));
    sendto(serverSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &clientAddr, addrLen);

    u_long imode = 1;
    ioctlsocket(serverSocket, FIONBIO, &imode);
    clock_t start = clock();
    while (recvfrom(serverSocket, buffer, sizeof(RDTHead), 0, (sockaddr *) &clientAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            memcpy(buffer, &serverACK, sizeof(RDTHead));
            sendto(serverSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &clientAddr, addrLen);
            memcpy(buffer, &serverFIN, sizeof(RDTHead));
            sendto(serverSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &clientAddr, addrLen);
            start = clock();
        }
    }

    RDTHead clientACK;
    memcpy(&clientACK,buffer,sizeof(RDTHead));
    if ((isACK(clientACK.flag)) && (CalcheckSum((u_short *) buffer, sizeof(RDTHead) == 0))) {
        cout << "成功关闭连接" << endl;
    } else {
        cout << "错误" << endl;
        return false;
    }
    closesocket(serverSocket);
    return true;
}
int main()
{
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;      //IP格式
    USHORT uPort = 8888;                  //写死端口号
    serverAddr.sin_port = htons(uPort);   //绑定端口号
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    bind(serverSocket, (SOCKADDR *) &serverAddr, sizeof(SOCKADDR));

    SOCKADDR_IN clientAddr;

    //三次握手建立连接
    if (!AcClient(serverSocket, clientAddr)) {
        cout << "连接失败" << endl;
        return 0;
    }

    char *fileBuffer = new char[MAX_FILE_SIZE];
    unsigned long filelen=0;
    recv(fileBuffer, serverSocket, clientAddr,filelen);

    //写入复制文件
    string filename = "D:\\cpp_vscode\\conmpter_network\\lab3-1\\test.jpg";
    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open()) {
        cout << "路径错误" << endl;
        return 0;
    }
    outfile.write(fileBuffer, filelen);
    outfile.close();

    //四次挥手断开连接
    if (!DisConClient(serverSocket, clientAddr)) {
        cout << "关闭连接失败" << endl;
        return 0;
    }



    system("pause");
    return 0;
}