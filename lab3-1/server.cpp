#include<iostream>
#include<stdio.h>
#include<windows.h>
#include<time.h>
#include<winsock.h>
#include<string.h>
#include "rdt.h"
#pragma comment(lib, "ws2_32.lib")

double MAX_TIMEOUT = 1*CLOCKS_PER_SEC;

bool AcClient(SOCKET socket, SOCKADDR_IN addr)
{
    //��һ������
    char buffer[sizeof(RDTHead)];
    int len = sizeof(addr);
    recvfrom(socket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &addr, &len);
    RDTHead clientSYN;
    memcpy(&clientSYN, buffer, sizeof(clientSYN));
    if (isSYN(clientSYN.flag) && (CalcheckSum((u_short *)&clientSYN, sizeof(RDTHead)) == 0))
        cout << "��һ�����ֳɹ�" << endl;

    //�ڶ�������
    RDTHead serverSYN_ACK;
    setSYN_ACK(serverSYN_ACK.flag);
    serverSYN_ACK.checkSum = CalcheckSum((u_short *) &serverSYN_ACK, sizeof(RDTHead));
    memcpy(buffer, &serverSYN_ACK, sizeof(RDTHead));
    sendto(socket, buffer, sizeof(RDTHead), 0, (sockaddr *) &addr, len);
    cout<<"�ڶ�������"<<endl;

    u_long imode = 1;
    ioctlsocket(socket, FIONBIO, &imode);//������
    clock_t start = clock(); //��ʼ��ʱ
    while (recvfrom(socket, buffer, sizeof(RDTHead), 0, (sockaddr *) &addr, &len) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout << "�ڶ������ֳ�ʱ�ش�" << endl;
            sendto(socket, buffer, sizeof(buffer), 0, (sockaddr *) &addr, len);
            start = clock();
        }
    }

    RDTHead clientACK;
    memcpy(&clientACK, &buffer, sizeof(RDTHead));
    if (isACK(clientACK.flag) && (CalcheckSum((u_short *) buffer, sizeof(RDTHead)) == 0)) {
        cout << "���������ֳɹ�" << endl;
    } else {
        cout << "����������ʧ��" << endl;
        return false;
    }

    imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//��Ϊ����ģʽ
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
    system("pause");
    return 0;
}