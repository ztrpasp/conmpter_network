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

double MAX_TIMEOUT = 1*CLOCKS_PER_SEC; //��ʱ�ش�ʱ�������Ǳ����ϲ�ͬ�˿������ݴ��䣬�ȳ�ʼ��Ϊ1s


//������������s
bool ConServer(SOCKET socket, SOCKADDR_IN serverAddr)
{
    //��һ�����֣�ֻ��Ҫ����Э��ͷ
    int addrLen = sizeof(serverAddr);
    RDTHead clientSYN;
    setSYN(clientSYN.flag);
    clientSYN.checkSum=CalcheckSum((unsigned short *)&clientSYN, sizeof(clientSYN));
    char buffer[sizeof(clientSYN)];
    memset(buffer, 0, sizeof(clientSYN));
    memcpy(buffer, &clientSYN, sizeof(clientSYN));
    sendto(socket, buffer, sizeof(clientSYN), 0, (SOCKADDR *) &serverAddr, addrLen);
    cout << "��һ������" << endl;

    //�ڶ�������
    RDTHead serverSYN_ACK;
    clock_t start =clock();

    u_long mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);//����Ϊ������

    while (recvfrom(socket, buffer, sizeof(serverSYN_ACK), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout<<"��һ�����ֳ�ʱ�ش�"<<endl;
            memcpy(buffer, &clientSYN, sizeof(clientSYN));
            sendto(socket, buffer, sizeof(clientSYN), 0, (SOCKADDR *) &serverAddr, addrLen);
            start = clock();
        }
    }

    
    memcpy(&serverSYN_ACK, buffer, sizeof(serverSYN_ACK));
    if (isSYN_ACK(serverSYN_ACK.flag)&& (CalcheckSum((u_short *) &serverSYN_ACK, sizeof(RDTHead)) == 0)) {
        cout << "�ڶ������ֳɹ�" << endl;
    } else {
        cout << "�ڶ�������ʧ��" << endl;
        return false;
    }

    //����������
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
        //����������ȷ�ϰ���ʧ
        memcpy(buffer, &clientACK, sizeof(RDTHead));
        sendto(socket, buffer, sizeof(RDTHead), 0, (sockaddr *) &serverAddr, addrLen);
        cout<<"���������ֳ�ʱ�ش�"<<endl;
        start = clock();
    }


    cout<<"���������ֳɹ�����"<<endl;
    u_long imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//����
    return true;
}




void send(char *fileBuffer, size_t filelen, SOCKET &socket, SOCKADDR_IN &addr) {

    u_long imode = 1;
    ioctlsocket(socket, FIONBIO, &imode); //�Ƚ��������ģʽ
    
    int packetNum = int(filelen / MAX_DATA_SIZE); int remain = filelen % MAX_DATA_SIZE ? 1 : 0;
    packetNum += remain;
    int num = 0;  //���ݰ�������                      
    int stage = 0; //�����Զ�״̬��
    int addrLen = sizeof(addr);

    char *dataBuffer = new char[MAX_DATA_SIZE], *pktBuffer = new char[sizeof(RDTPacket)];
    RDTPacket sendPkt, rcvPkt;   

    cout <<"�ܹ���Ҫ����"<< packetNum << "�����ݰ�" <<endl;

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
                cout<<"�ļ��������"<<endl;
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
                start = clock();//��ʱ
                stage = 1;
                break;
            case 1:
                //��ʱ�����
                while (recvfrom(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, &addrLen) <= 0) {
                    if (clock() - start >= MAX_TIMEOUT) {
                        sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                        cout << num << "�����ݰ���ʱ�ش�" << endl;
                        start = clock();
                    }
                }
                memcpy(&rcvPkt, pktBuffer, sizeof(RDTPacket));
                
                //�յ��ظ��İ�����У��ʹ���
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
                //��ʱ���
                while (recvfrom(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, &addrLen) <= 0) {
                    if (clock() - start >= MAX_TIMEOUT) {
                        sendto(socket, pktBuffer, sizeof(RDTPacket), 0, (SOCKADDR *) &addr, addrLen);
                        cout << num << "�����ݰ���ʱ�ش�" << endl;
                        start = clock();
                    }
                }
                memcpy(&rcvPkt, pktBuffer, sizeof(RDTPacket));
                //�յ��ظ��İ�����У��ʹ���
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
    cout<<"�ͻ��˷����һ�λ��ֽ���FIN-WAIT-1״̬"<<endl;
    unsigned long imode = 1;
    ioctlsocket(clientSocket, FIONBIO, &imode); //��Ϊ������ģʽ

    clock_t start = clock();
    while (recvfrom(clientSocket, buffer, sizeof(RDTHead), 0, (sockaddr *) &serverAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout<<"��һ�λ��ֳ�ʱ�ش�"<<endl;
            memcpy(buffer, &clientFIN, sizeof(RDTHead));
            sendto(clientSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &serverAddr, addrLen);
            start = clock();
        }
    }

    RDTHead serverACK;
    memcpy(&serverACK, buffer, sizeof(RDTHead));

    if ((isACK(serverACK.flag)) && (CalcheckSum((u_short *) buffer, sizeof(RDTHead) == 0))) {
        cout << "�ͻ��˽���FIN-WAIT-2״̬" << endl;
    } else {
        cout << "����" << endl;
        return false;
    }

    imode = 0;
    ioctlsocket(clientSocket, FIONBIO, &imode);//����

    recvfrom(clientSocket, buffer, sizeof(RDTHead), 0, (SOCKADDR *) &serverAddr, &addrLen);
    RDTHead serverFIN;
    memcpy(&serverFIN, buffer, sizeof(RDTHead));
    if ((isFIN_ACK(serverFIN.flag)) && (CalcheckSum((u_short *) buffer, sizeof(RDTHead) == 0))) {
        cout << "�����λ���" << endl;
    } else {
        cout << "����" << endl;
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
        //ȷ�ϰ���ʧ
        memcpy(buffer, &clientACK, sizeof(RDTHead));
        sendto(clientSocket, buffer, sizeof(RDTHead), 0, (sockaddr *) &serverAddr, addrLen);
        start = clock();
    }

    cout << "���ӳɹ��ر�" << endl;
    closesocket(clientSocket);
    return true;
}

int main()
{
    WSADATA wsaData;//WSAStartup�������ú󷵻ص�Windows Sockets���ݵ����ݽṹ 
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    SOCKET clientSocket; //�����׽���
    //��ַ����ΪAD_INET����������Ϊ���ݱ���Э�����
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        //�׽��ִ���ʧ�ܣ�
        cout<<"�׽��ִ���ʧ��"<<endl; WSACleanup();
        return 0;
    }
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;      //IP��ʽ
    USHORT uPort = 8887;                  //д���˿ں�
    serverAddr.sin_port = htons(uPort);   //�󶨶˿ں�
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    if (!ConServer(clientSocket, serverAddr)) {
        cout << "����ʧ��" << endl;
        return 0;
    }
    char fileName[150]="D:\\cpp_vscode\\conmpter_network\\lab3-1\\�����ļ�\\"; char path[50];
    cout << "��������Ҫ������ļ���" << endl;
    cin >> path;
    
    strcat(fileName,path);
    ifstream myfile(fileName, ifstream::binary);
    if (!myfile.is_open())
    {
        cout << "·������" << endl;
        return 0;
    }

    
    // ����һ���洢�ļ�(��)��Ϣ�Ľṹ�壬�������ļ���С�ʹ���ʱ�䡢����ʱ�䡢�޸�ʱ���
	struct stat statbuf;
	// �ṩ�ļ����ַ���������ļ����Խṹ��
	stat(fileName, &statbuf);
	// ��ȡ�ļ���С
	size_t fileLen = statbuf.st_size;

    char *fileBuffer = new char[fileLen];
    myfile.read(fileBuffer, fileLen);
    myfile.close();
    cout << "��ʼ���д���, �ļ���СΪ:  "<<fileLen<< endl;

    send(fileBuffer, fileLen, clientSocket, serverAddr);

    if (!DisConServer(clientSocket, serverAddr)) {
        cout << "�ر�����ʧ��" << endl;
        return 0;
    }
    cout<<"�ɹ��ر�����"<<endl;

    system("pause");
    return 0;
}