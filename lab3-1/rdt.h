//用于定义可靠传输协议
#include<string.h>
using namespace std;
#define MAX_DATA_SIZE 10240


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
struct RDTHead
{
    unsigned int seq;//序列号，发送端
    unsigned int ack;//确认号码，发送端和接收端用来控制
    unsigned short checkSum;//校验和 16位
    unsigned dataSize;      //标识发送的数据的长度,边界判断与校验和
    char flag;              //ACK，FIN，SYN，END
    
    RDTHead()
    {
        this->seq=this->ack=0; //
        this->checkSum=this->dataSize=0;
    }
};

struct RDTPacket
{
    /* data */
    RDTHead head;
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

RDTPacket mkPacket(int seq, char *data, int len) {
    RDTPacket pkt;
    pkt.head.seq = seq;
    pkt.head.dataSize = len;
    memcpy(pkt.data, data, len);
    pkt.head.checkSum = CalcheckSum((u_short *) &pkt, sizeof(RDTPacket));
    return pkt;
}
RDTPacket mkPacket(int ack) {
    RDTPacket pkt;
    pkt.head.ack = ack;
    setACK(pkt.head.flag);
    pkt.head.checkSum = CalcheckSum((u_short *) &pkt, sizeof(RDTPacket));
    return pkt;
}

void extractPkt(char * Buffer, RDTPacket pkt)
{
    memcpy(Buffer, &pkt, sizeof(RDTPacket));
}