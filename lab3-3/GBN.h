//用于定义可靠传输协议
#include<string.h>
#include<winsock.h>
using namespace std;

#define MAX_DATA_SIZE 5120
#define windowSize 8

#define MAX_SEQ  4294967295
unsigned int base=1;
unsigned int nextseqnum=1;



float cwnd = 1;
unsigned int ssthresh = 32 ;
unsigned int dupACKCount = 0;



#define SYN 0x1
#define ACK 0x2
#define FIN 0x4
#define END 0x8

void setSYN(char &flag){ flag |= SYN; }
void setACK(char &flag){ flag |= ACK; }
void setSYN_ACK(char &flag){ flag |= SYN; flag |= ACK; }
void setFIN(char &flag){ flag |=  FIN; }
void setFIN_ACK(char &flag){ flag |= FIN; flag |= ACK; }
void setEND(char &flag){ flag |= END;  }

bool isSYN_ACK(char flag){ return (flag & SYN)&&(flag & ACK); }
bool isSYN(char flag){ return (flag & SYN); }
bool isACK(char flag){ return (flag & ACK); }
bool isFIN_ACK(char flag){ return (flag & FIN)&&(flag & ACK); }
bool isFIN(char flag){ return (flag & FIN); }
bool isEND(char flag){ return (flag & END); }
struct Head
{
    unsigned int seq;//序列号，发送端
    unsigned int ack;//确认号码，发送端和接收端用来控制
    unsigned short checkSum;//校验和 16位
    unsigned int dataSize;      //标识发送的数据的长度,边界判断与校验和
    char flag;              //ACK，FIN，SYN，END
    
    Head()
    {
        this->seq=this->ack=0; //
        this->checkSum=this->dataSize=0;
    }
};

struct Packet
{
    /* data */
    Head head;
    char data[MAX_DATA_SIZE];
};

//计算校验和
unsigned short CalcheckSum(unsigned short *packet, unsigned int dataSize)
{
    unsigned long sum = 0;
    int count = (dataSize + 1) / 2;

    unsigned short *temp = new unsigned short [count];
    memset(temp, 0, 2 * count);
    memcpy(temp, packet, dataSize);

    while (count--) {
        sum += *temp++;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++;
        }
    }
    return ~(sum & 0xFFFF);
}

Packet mkPacket(int seq, char *data, int len) {
    Packet pkt;
    pkt.head.seq = seq;
    pkt.head.dataSize = len;
    memcpy(pkt.data, data, len);
    pkt.head.checkSum = CalcheckSum((u_short *) &pkt, sizeof(Packet));
    return pkt;
}
Packet mkPacket(int ack) {
    Packet pkt;
    pkt.head.ack = ack;
    setACK(pkt.head.flag);
    pkt.head.checkSum = CalcheckSum((u_short *) &pkt, sizeof(Packet));
    return pkt;
}

void extractPkt(char * Buffer, Packet *pkt)
{
    memcpy(Buffer, pkt, sizeof(Packet));
}

int sendHead(char *buffer, Head *head, SOCKET socket,SOCKADDR_IN addr)
{
    int addrLen = sizeof(addr);
    memcpy(buffer, head, sizeof(Head));
    return sendto(socket, buffer, sizeof(Head), 0, (SOCKADDR *) &addr, addrLen);
}

int  sendPacket(char *buffer, Packet *packet, SOCKET socket,SOCKADDR_IN addr)
{
    int addrLen = sizeof(addr);
    memcpy(buffer, packet, sizeof(Packet));
    return sendto(socket, buffer, sizeof(Packet), 0, (SOCKADDR *) &addr, addrLen);
}