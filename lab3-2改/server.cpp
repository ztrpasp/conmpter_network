#include<iostream>
#include<stdio.h>
#include<windows.h>
#include<time.h>
#include<winsock.h>
#include<string.h>
#include <fstream>
#include "GBN.h"
#pragma comment(lib, "ws2_32.lib")

#define MAX_FILE_SIZE 15000000
double MAX_TIMEOUT = 1*CLOCKS_PER_SEC;

bool AcClient(SOCKET socket, SOCKADDR_IN addr)
{
    //��һ������
    char buffer[sizeof(Head)];
    int len = sizeof(addr);
    recvfrom(socket, buffer, sizeof(Head), 0, (SOCKADDR *) &addr, &len);
    Head clientSYN;
    memcpy(&clientSYN, buffer, sizeof(clientSYN));
    if (isSYN(clientSYN.flag) && (CalcheckSum((u_short *)&clientSYN, sizeof(Head)) == 0))
        cout << "��һ�����ֳɹ�" << endl;

    //�ڶ�������
    Head serverSYN_ACK;
    setSYN_ACK(serverSYN_ACK.flag);
    serverSYN_ACK.checkSum = CalcheckSum((u_short *) &serverSYN_ACK, sizeof(Head));
    memcpy(buffer, &serverSYN_ACK, sizeof(Head));
    sendto(socket, buffer, sizeof(Head), 0, (sockaddr *) &addr, len);
    cout<<"�ڶ������ֽ���SYN_RECV״̬"<<endl;

    u_long imode = 1;
    ioctlsocket(socket, FIONBIO, &imode);//������
    clock_t start = clock(); //��ʼ��ʱ
    while (recvfrom(socket, buffer, sizeof(Head), 0, (sockaddr *) &addr, &len) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout << "�ڶ������ֳ�ʱ�ش�" << endl;
            sendto(socket, buffer, sizeof(buffer), 0, (sockaddr *) &addr, len);
            start = clock();
        }
    }

    Head clientACK;
    memcpy(&clientACK, &buffer, sizeof(Head));
    if (isACK(clientACK.flag) && (CalcheckSum((u_short *) buffer, sizeof(Head)) == 0)) {
        cout << "���������ֽ���Established״̬" << endl;
    } 
    else 
    {
        cout << "����������ʧ��" << endl;
        return false;
    }

    imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//��Ϊ����ģʽ
    return true;
}

bool recv(char *fileBuffer, SOCKET &socket, SOCKADDR_IN &addr, unsigned long &filelen) {
    filelen = 0;
    int addrLen = sizeof(addr);
    u_int expectedSeq = 1;
    int dataLen;

    char *pktBuffer = new char[sizeof(Packet)];
    Packet recvPkt, sendPkt= mkPacket(-1);

    while (true) {
        memset(pktBuffer, 0, sizeof(Packet));
        recvfrom(socket, pktBuffer, sizeof(Packet), 0, (SOCKADDR *) &addr, &addrLen);
        memcpy(&recvPkt,pktBuffer, sizeof(Packet));
        cout<<"recv"<<endl;
        if (isEND(recvPkt.head.flag) && CalcheckSum((u_short*)&recvPkt, sizeof(Head))==0) {
            cout << "�������" << endl;
            Head endPacket;
            setACK(endPacket.flag);
            endPacket.checkSum = CalcheckSum((u_short *) &endPacket, sizeof(Head));
            memcpy(pktBuffer, &endPacket, sizeof(Head));
            sendto(socket, pktBuffer, sizeof(Head), 0, (SOCKADDR *) &addr, addrLen);
            return true;
        }

        if(recvPkt.head.seq==expectedSeq && CalcheckSum((u_short*)&recvPkt, sizeof(Packet))==0){
            //correctly receive the expected seq
            dataLen = recvPkt.head.dataSize;
            memcpy(fileBuffer + filelen, recvPkt.data, dataLen);
            filelen += dataLen;

            //give back ack=seq
            sendPkt = mkPacket(expectedSeq);
            memcpy(pktBuffer, &sendPkt, sizeof(Packet));
            sendto(socket, pktBuffer, sizeof(Packet), 0, (SOCKADDR *) &addr, addrLen);
            cout<<"����ȷ��"<<expectedSeq;
            expectedSeq++;
            //cout<<"recv"<<endl;
            continue;
        }
        cout<<"wait head:"<<expectedSeq<<endl;
        cout<<"recv head:"<<recvPkt.head.seq<<endl;
        memcpy(pktBuffer, &sendPkt, sizeof(Packet));
        sendto(socket, pktBuffer, sizeof(Packet), 0, (SOCKADDR *) &addr, addrLen);
    }
}
bool DisConClient(SOCKET &serverSocket, SOCKADDR_IN &clientAddr) {
    int addrLen = sizeof(clientAddr);
    char *buffer = new char[sizeof(Head)];

    recvfrom(serverSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &clientAddr, &addrLen);

    Head clientFIN;
    memcpy(&clientFIN,buffer,sizeof(Head));

    if ((isFIN_ACK) && (CalcheckSum((u_short *) buffer, sizeof(Head) == 0))) {
        cout << "��һ�λ��ֿͻ�������Ͽ�" << endl;
    } else {
        cout << "����" << endl;
        return false;
    }

    Head serverACK;
    setACK(serverACK.flag);
    serverACK.checkSum = CalcheckSum((u_short *) &serverACK, sizeof(Head));
    memcpy(buffer, &serverACK, sizeof(Head));
    sendto(serverSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &clientAddr, addrLen);
    cout<<"�ڶ��λ��ֽ���CLOSE-WAIT״̬"<<endl;

    Head serverFIN;
    setFIN_ACK(serverFIN.flag);
    serverFIN.checkSum = CalcheckSum((u_short *) &serverFIN, sizeof(Head));
    memcpy(buffer, &serverFIN, sizeof(Head));
    sendto(serverSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &clientAddr, addrLen);
    cout<<"�����λ��ֽ���LAST-ACK״̬"<<endl;
    u_long imode = 1;
    ioctlsocket(serverSocket, FIONBIO, &imode);
    clock_t start = clock();
    while (recvfrom(serverSocket, buffer, sizeof(Head), 0, (sockaddr *) &clientAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            memcpy(buffer, &serverACK, sizeof(Head));
            sendto(serverSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &clientAddr, addrLen);
            memcpy(buffer, &serverFIN, sizeof(Head));
            sendto(serverSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &clientAddr, addrLen);
            start = clock();
        }
    }

    Head clientACK;
    memcpy(&clientACK,buffer,sizeof(Head));
    if ((isACK(clientACK.flag)) && (CalcheckSum((u_short *) buffer, sizeof(Head) == 0))) {
        cout << "���Ĵλ��ֽ���closed״̬" << endl;
    } else {
        cout << "����" << endl;
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
    serverAddr.sin_family = AF_INET;      //IP��ʽ
    USHORT uPort = 8888;                  //д���˿ں�
    serverAddr.sin_port = htons(uPort);   //�󶨶˿ں�
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    bind(serverSocket, (SOCKADDR *) &serverAddr, sizeof(SOCKADDR));

    SOCKADDR_IN clientAddr;

    //�������ֽ�������
    if (!AcClient(serverSocket, clientAddr)) {
        cout << "����ʧ��" << endl;
        return 0;
    }

    char *fileBuffer = new char[MAX_FILE_SIZE];
    unsigned long filelen=0;
    cout<<"��ʼ�����ļ�"<<endl;
    recv(fileBuffer, serverSocket, clientAddr,filelen);

    //д�븴���ļ�
    string filename = "D:\\cpp_vscode\\conmpter_network\\lab3-2��\\test.jpg";
    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open()) {
        cout << "·������" << endl;
        return 0;
    }
    outfile.write(fileBuffer, filelen);
    outfile.close();

    //�Ĵλ��ֶϿ�����
    if (!DisConClient(serverSocket, clientAddr)) {
        cout << "�ر�����ʧ��" << endl;
        return 0;
    }



    system("pause");
    return 0;
}