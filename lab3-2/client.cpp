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

double MAX_TIMEOUT = 1*CLOCKS_PER_SEC; //��ʱ�ش�ʱ�������Ǳ����ϲ�ͬ�˿������ݴ��䣬�ȳ�ʼ��Ϊ1s
double DELAY_ACK = 0.5*CLOCKS_PER_SEC; //�ӳ�ACK��ʱ��
 

//�������ֽ������ӣ�ֻ��Ҫ����Э��ͷ���֡�
bool ConServer(SOCKET socket, SOCKADDR_IN serverAddr)
{
    //��һ������
    int addrLen = sizeof(serverAddr);
    Head clientSYN;
    setSYN(clientSYN.flag);
    
    clientSYN.checkSum=CalcheckSum((unsigned short *)&clientSYN, sizeof(clientSYN));
    char buffer[sizeof(clientSYN)];
    memset(buffer, 0, sizeof(clientSYN));
    sendHead(buffer,&clientSYN,socket,serverAddr);
    cout << "��һ������,����SYN_SEND״̬" << endl;
    //�ڶ�������
    Head serverSYN_ACK;
    u_long mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);//����Ϊ������
    clock_t start =clock();
    while (recvfrom(socket, buffer, sizeof(serverSYN_ACK), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout<<"��һ�����ֳ�ʱ�ش�"<<endl;
            memcpy(buffer, &clientSYN, sizeof(clientSYN));
            sendto(socket, buffer, sizeof(clientSYN), 0, (SOCKADDR *) &serverAddr, addrLen);
            start = clock();
        }
    }

    
    memcpy(&serverSYN_ACK, buffer, sizeof(serverSYN_ACK));
    if (isSYN_ACK(serverSYN_ACK.flag)&& (CalcheckSum((u_short *) &serverSYN_ACK, sizeof(Head)) == 0)) {
        cout << "�ڶ������ֳɹ�" << endl;
    } else {
        cout << "�ڶ�������ʧ��" << endl;
        return false;
    }

    //����������
    Head clientACK;
    setACK(clientACK.flag);
    clientACK.checkSum=CalcheckSum((unsigned short *)&clientACK, sizeof(clientACK));
    memcpy(buffer, &clientACK, sizeof(clientACK));
    if(sendto(socket, buffer, sizeof(clientACK), 0, (SOCKADDR *) &serverAddr, addrLen)==-1)
    {
        return false;
    }

    cout<<"���������ֽ���TIME-WAIT״̬"<<endl;
    start = clock();
    while (clock() - start <= 2 * MAX_TIMEOUT) {
        if (recvfrom(socket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0)
            continue;
        //����������ȷ�ϰ���ʧ
        memcpy(buffer, &clientACK, sizeof(Head));
        sendto(socket, buffer, sizeof(Head), 0, (sockaddr *) &serverAddr, addrLen);
        cout<<"���������ֳ�ʱ�ش�"<<endl;
        start = clock();
    }


    cout<<"���������ֽ���Established״̬"<<endl;
    u_long imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//����
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
            cout << num << "�����ݰ��Ѿ�����" << endl;
        }
        else break;
    }
    
}

void send(char *fileBuffer, size_t filelen, SOCKET &socket, SOCKADDR_IN &addr) {

    u_long imode = 1;
    ioctlsocket(socket, FIONBIO, &imode); //�Ƚ��������ģʽ
    
    int packetNum = int(filelen / MAX_DATA_SIZE); 
    int remain = filelen % MAX_DATA_SIZE ? 1 : 0;
    packetNum += remain;
    cout <<"�ܹ���Ҫ����"<< packetNum << "�����ݰ�" <<endl;

    int num = 0;//���ݰ�������                      
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
                cout<<"��ʱ�ش�"<<num-(nextseqnum-base)+1<<"��"<<num<<endl;
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
        
        if(acceptACK == rcvPkt.head.ack)//������ͬ��ACK
        {
            cout<<"�յ��ظ���ACK"<<acceptACK<<endl;
        }
        else
        {
            cout<<"�յ�ȷ�� "<<rcvPkt.head.ack<<"       ";
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
                cout << "�ļ��������" << endl;
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
    cout<<"�ͻ��˷����һ�λ��ֽ���FIN-WAIT-1״̬"<<endl;
    unsigned long imode = 1;
    ioctlsocket(clientSocket, FIONBIO, &imode); //��Ϊ������ģʽ

    clock_t start = clock();
    while (recvfrom(clientSocket, buffer, sizeof(Head), 0, (sockaddr *) &serverAddr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIMEOUT) {
            cout<<"��һ�λ��ֳ�ʱ�ش�"<<endl;
            memcpy(buffer, &clientFIN, sizeof(Head));
            sendto(clientSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, addrLen);
            start = clock();
        }
    }

    Head serverACK;
    memcpy(&serverACK, buffer, sizeof(Head));

    if ((isACK(serverACK.flag)) && (CalcheckSum((u_short *) buffer, sizeof(Head) == 0))) {
        cout << "�ͻ��˽���FIN-WAIT-2״̬" << endl;
    } else {
        cout << "����" << endl;
        return false;
    }

    imode = 0;
    ioctlsocket(clientSocket, FIONBIO, &imode);//����

    recvfrom(clientSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, &addrLen);
    Head serverFIN;
    memcpy(&serverFIN, buffer, sizeof(Head));
    if ((isFIN_ACK(serverFIN.flag)) && (CalcheckSum((u_short *) buffer, sizeof(Head) == 0))) {
        cout << "�����λ���" << endl;
    } else {
        cout << "����" << endl;
        return false;
    }

    imode = 1;
    ioctlsocket(clientSocket, FIONBIO, &imode);

    Head clientACK;
    setACK(clientACK.flag);

    sendto(clientSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, addrLen);
    start = clock();
    cout<<"���Ĵλ��ֽ���TIME-WAIT״̬"<<endl;
    while (clock() - start <= 2 * MAX_TIMEOUT) {
        if (recvfrom(clientSocket, buffer, sizeof(Head), 0, (SOCKADDR *) &serverAddr, &addrLen) <= 0)
            continue;
        //ȷ�ϰ���ʧ
        cout<<"���Ĵλ��ֳ�ʱ�ش�"<<endl;
        memcpy(buffer, &clientACK, sizeof(Head));
        sendto(clientSocket, buffer, sizeof(Head), 0, (sockaddr *) &serverAddr, addrLen);
        start = clock();
    }

    cout << "���Ĵλ��ֽ���closed״̬" << endl;
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
    ifstream myfile;
    while(true)
    {
        cout << "��������Ҫ������ļ���" << endl;
        cin >> path;
    
        strcat(fileName,path);
        myfile.open(fileName, ifstream::binary);
        if (!myfile.is_open())
        {
            cout << "·������,������ȷ��·��" << endl;
        }
        else break;
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