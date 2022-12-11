#include <ctime>
#include <cmath>
#include <iostream>
#include <cstring>
using namespace std;
#define INT_MAX 4294967296
unsigned int LINEAR_M = INT_MAX - 1;
unsigned int LINEAR_A = 16807;
int change(char a);
class BigInt //ʮ������
{
public:
    int num[512];
    BigInt();
    void copy(BigInt);
    void print();
    void set(unsigned int a);
    void set(int a);
    void set(string a);
    void set(string a, int x);
    void set(unsigned long long a);
    int getbit(int i);
};
BigInt add(BigInt a, BigInt b);//�����������
BigInt sub(BigInt a, BigInt b);//�����������
BigInt sub(BigInt a, BigInt b, int d);
int compare(BigInt a, BigInt b, int d);
int compare(BigInt a, BigInt b);
BigInt mul(BigInt a, BigInt b);//�����������
BigInt mod(BigInt a, BigInt b);
BigInt pow(BigInt a, BigInt b, BigInt n);
BigInt calculate_d(BigInt a, BigInt b);//����˷���Ԫ
BigInt div(BigInt a, BigInt b, BigInt &c);

unsigned int smallprime[10000];
void getsmallprime();
class prime
{
public:
    BigInt number;
    prime();
    prime(string a);
    prime(string a, int x);
    void getarbitrary();
    void check2000();    //����Ƿ��ܱ�2000���ڵ���������,���������2��ֱ������
    void millerrabin();  //����Ƿ�������
};

class RSA //����
{
public:
    BigInt n, phi;
    BigInt q;
    BigInt p;
    BigInt d, e;
    RSA(BigInt p, BigInt q, BigInt e);
};

class encrypt //����
{
public:
    BigInt n, e;
    BigInt c, m; //���ĺ�����
    encrypt(RSA a, BigInt m);
};

class decrypt //����
{
public:
    BigInt n, d;
    BigInt c, m; //���ĺ�����
    decrypt(RSA a, BigInt c);
};

prime::prime(string a)
{
    number.set(a);
}
prime::prime(string a, int x)
{
    number.set(a, x);
}

void BigInt::copy(BigInt a)
{
    for (int i = 0; i < 512; i++)
        num[i] = a.num[i];
}

BigInt::BigInt()
{
    memset(this, 0, sizeof(BigInt));
}

void BigInt::print()
{
    int i;
    for (i = 511; i > 0; i--)
        if (num[i])
            break;
    cout << "0x";
    for (; i >= 0; i--) cout << hex << num[i];
    cout << endl;
}

//ָ������
BigInt pow(BigInt a, BigInt b, BigInt n)
{
    BigInt buffer[512];
    buffer[0].copy(a);
    int x;
    for (x = 511; x > 0; x--)
        if (b.num[x])
            break;
    for (int i = 1; i <= x; i++)
    {
        BigInt tempbuf[5];
        tempbuf[0].copy(buffer[i - 1]);
        for (int j = 1; j < 5; j++)
        {
            //����ƽ��
            BigInt temp = mul(tempbuf[j - 1], tempbuf[j - 1]);
            //�ó��Ľ����n����ȡģ
            tempbuf[j] = mod(temp, n);
        }
        buffer[i].copy(tempbuf[4]);
    }
    BigInt product;
    product.num[0] = 1;
    for (int i = 511; i >= 0; i--)
    {
        BigInt temp;
        for (int j = 0; j < b.num[i]; j++)
        {
            temp = mul(product, buffer[i]);
            product = mod(temp, n);
        }
    }
    return product;
}

//����Ӧλ����ӣ�������ּӡ������˵��������λ
BigInt add(BigInt a, BigInt b)
{
    BigInt c;
    for (int i = 0; i < 512; i++) c.num[i] = a.num[i] + b.num[i];
    for (int i = 0; i < 511; i++)
    {
        c.num[i + 1] += c.num[i] / 16;
        c.num[i] %= 16;
    }
    return c;
}

//����Ӧλ����ӣ�������ּ��������˵�������ӻ���
BigInt sub(BigInt a, BigInt b)
{
    BigInt c;
    for (int i = 0; i < 512; i++) c.num[i] = a.num[i] - b.num[i];
    for (int i = 0; i < 511; i++)
    {
        if (c.num[i] < 0)
        {
            c.num[i] += 16;
            c.num[i + 1] -= 1;
        }
    }
    return c;
}

BigInt sub(BigInt a, BigInt b, int d)
{
    BigInt c;
    c.copy(a);
    //��ģ��������Ӧ��λ�������ơ���Ȼ��ģ������ȥ���ƺ��ģ��
    for (int i = d; i < 512; i++) c.num[i] -= b.num[i - d];
    for (int i = 0; i < 511; i++)
    {
        //������������ˣ��ٸ����ӻ���
        if (c.num[i] < 0)
        {
            c.num[i] += 16;
            c.num[i + 1] -= 1;
        }
    }
    return c;
}

//����������������˲�����num[512]��
BigInt mul(BigInt a, BigInt b)
{
    BigInt result;
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            result.num[i + j] += a.num[i] * b.num[j];
        }
    }
    //�Դ��ڽ�λ��������н�λ����
    for (int i = 0; i < 510; i++)
    {
        result.num[i + 1] += result.num[i] / 16;
        result.num[i] %= 16;
    }
    //��󷵻ؽ��
    return result;
}

BigInt div(BigInt a, BigInt b, BigInt &c)
{
    //�Ƚϳ����뱻����
    BigInt q;
    int t = compare(a, b);
    if (t == -1)
        return q;
    if (t == 0)
    {
        BigInt c;
        c.set(1);
        return c;
    }
    int x, y;
    for (x = 511; x >= 0; x--)
        if (a.num[x])
            break;
    for (y = 511; y >= 0; y--)
        if (b.num[y])
            break;
    int d = x - y;

    //ѭ������������Ȱ���λ������λ���ɹ���������Ӧ��λ�Ӹ�����
    c.copy(a);
    while (d >= 0)
    {
        while (c.num[x] > b.num[y])
        {
            c = sub(c, b, d);
            BigInt v1;
            v1.num[d] = 1;
            q = add(q, v1);
        }
        if (c.num[x] == b.num[y] && compare(c, b, d) == 1)
        {
            c = sub(c, b, d);
            BigInt v1;
            v1.num[d] = 1;
            q = add(q, v1);
        }
        d--;
        int t;
        while (compare(c, b) == 1 && c.num[x])
        {
            c = sub(c, b, d);
            BigInt v1;
            v1.num[d] = 1;
            q = add(q, v1);
        }
        if (c.num[x] == 0)
            x--;
        t = compare(c, b);
        if (t == -1)
            break;
        if (t == 0)
        {
            c = sub(c, b);
            BigInt v1;
            v1.num[d] = 1;
            q = add(q, v1);
            return q;
        }
    }
    return q;
}

BigInt mod(BigInt a, BigInt b) // a>b
{
    //���ȱȽ�a,b
    int t = compare(a, b);
    //���ü���ģ��ǰһ��С�ں�һ����ֱ�ӷ���ǰһ��
    if (t == -1)
        return a;
    //��������ȣ�����0
    if (t == 0)
    {
        BigInt zero;
        return zero;
    }
    //�ó�a��b�����λ
    int x, y;
    for (x = 511; x >= 0; x--)
        if (a.num[x])
            break;
    for (y = 511; y >= 0; y--)
        if (b.num[y])
            break;
    
    //����������֮������λ��
    int d = x - y;

    //��a����c
    BigInt c;
    c.copy(a);
    while (d >= 0)
    {
        //����λ�������Ƚ��ڶ��������Ƶ��͵�һ���λһ���ĵط���
        while (c.num[x] > b.num[y]) c = sub(c, b, d);
        if (c.num[x] == b.num[y] && compare(c, b, d) == 1) c = sub(c, b, d);
        d--;
        int t;
        while (compare(c, b) == 1 && c.num[x]) c = sub(c, b, d);
        if (c.num[x] == 0) x--;
        t = compare(c, b);
        if (t == -1) return c;
        if (t == 0)
        {
            BigInt s;
            return s;
        }
    }
    return c;
}

//�Ƚϴ������������±ȣ����ǰһ�����ں�һ������1��С�ڷ���-1�����ڷ���0
int compare(BigInt a, BigInt b)
{
    for (int i = 511; i >= 0; i--)
    {
        if (a.num[i] > b.num[i])
            return 1;
        if (a.num[i] < b.num[i])
            return -1;
    }
    return 0;
}

//����λ��֮��ͬ��
int compare(BigInt a, BigInt b, int d)
{
    for (int i = 511; i >= d; i--)
    {
        if (a.num[i] > b.num[i - d])
            return 1;
        if (a.num[i] < b.num[i - d])
            return -1;
    }
    return 0;
}

//ʹ��ŷ������㷨����˷���Ԫ
BigInt calculate_d(BigInt a, BigInt b)
{
    b = mod(b, a);
    BigInt q[100], t[100], r[100];
    r[0].copy(a);
    r[1].copy(b);
    t[0].set(0);
    t[1].set(1);
    int i;
    BigInt v1;
    v1.set(1);
    for (i = 2; i < 500; i++)
    {
        //���������������
        q[i - 1] = div(r[i - 2], r[i - 1], r[i]);
        //���
        BigInt temp = mul(q[i - 1], t[i - 1]);
        while (compare(temp, t[i - 2]) == 1) t[i - 2] = add(t[i - 2], r[0]);
        t[i] = sub(t[i - 2], temp);
        //�˵����Ϊ1����
        if (compare(r[i], v1) == 0) break;
    }
    return t[i];
}

void getsmallprime()
{
    //��4��ʼ���㣬2*2,2*3,2*4��������3*2��ֱ��10000����Щ�����Ӧ�����鸳1,ȥ�������еĺ���
    int a[10001];
    memset(a, 0, sizeof(a));
    for (int i = 2; i <= 100; i++)
    {
        for (int j = 2; i * j <= 10000; j++)
        {
            a[i * j] = 1;
        }
    }
    //����Щ�Ǻ���������Ҫ������������
    int index = 0;
    for (int i = 2; i <= 10000; i++)
    {
        if (!a[i])
            smallprime[index++] = i;
    }
}

//ʹ������ͬ������512λ����
void prime::getarbitrary()
{
    unsigned long long num[16];
    srand((unsigned)time(NULL));
    num[0] = rand() % (INT_MAX - 1) + 1;
    for (int i = 1; i < 16; i++)
    {
        num[i] = (num[i - 1] * LINEAR_A) % LINEAR_M;//����ͬ���㷨
    }
    //�����λ�����λ�����1
    num[15] |= (1 << 31) & 0x0000000011111111;
    num[0] |= 1;
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            number.num[i * 8 + j] = num[i] % 16;
            num[i] /= 16;
        }
    }
    int i;
}

void BigInt::set(unsigned int a)
{
    memset(this, 0, sizeof(BigInt));
    int i = 0;
    while (a)
    {
        num[i] = a % 16;
        a /= 16;
        i++;
    }
}

void BigInt::set(unsigned long long a)
{
    memset(this, 0, sizeof(BigInt));
    int i = 0;
    while (a)
    {
        num[i] = a % 16;
        a /= 16;
        i++;
    }
}

void BigInt::set(int a)
{
    memset(this, 0, sizeof(BigInt));
    if (a < 0)
        return;
    int i = 0;
    while (a)
    {
        num[i] = a % 16;
        a /= 16;
        i++;
    }
}

int BigInt::getbit(int i)
{
    if ((num[i / 4] & (1 << (i % 4))) == 0)
        return 0;
    return 1;
}

prime::prime()
{
    memset(this, 0, sizeof(prime));
    getarbitrary();//����512������
    check2000();    //����Ƿ��ܱ�2000���ڵ���������,���������2��ֱ������
    millerrabin();  //����Ƿ�������
}

void prime::check2000() //����Ƿ��ܱ�2000���ڵ���������,���������2��ֱ������
{
    if (smallprime[0] == 0)
        getsmallprime();
    BigInt v0; // 0
    BigInt v2;
    v2.set(2);
    while (1)
    {
        int flag = 1;
        for (int i = 0; i < 303; i++)
        {
            BigInt temp;
            temp.set(smallprime[i]);
            BigInt s = mod(number, temp);
            if (compare(s, v0) == 0)
            {
                flag = 0;
                break;
            }
        }
        if (flag)
        {
            break;
        }
        else
        {
            number = add(number, v2);
        }
    }
}

void prime::millerrabin()//�����ޱ��㷨��⣬����ֻ���5��
{
    int a[5] = {2, 3, 13, 17, 19};
    int flag = 0;
    BigInt n1;
    BigInt v1, v2;
    v1.set(1);
    v2.set(2);
    BigInt v0;
    v0.set(0);
    int high;
    while (!flag)
    {
        //��number-1����n1
        flag = 1;
        n1 = sub(number, v1);
        //�õ�num�е����λ
        for (high = 127; high >= 0; high--)
        {
            if (number.num[high])
                break;
        }
        BigInt temp, d;

        for (int k = 0; k < 5; k++)
        {
            temp.set(a[k]);
            d.set(1);//d=1
            for (int i = high * 4 + 3; i >= 0; i--)
            {
                BigInt x;
                x.copy(d);//x=d
                d = mod(mul(d, d), number);//����d��ƽ��mod n
                if (compare(d, v1) == 0&&compare(x, v1) && compare(x, n1))//���d=0,x��1,,x��n-1,���ҷ���ʧ��
                {
                    flag = 0;
                    break;
                }
                if (n1.getbit(i)) d = mod(mul(d, temp), number);//���b=1����d=d*a mod n
            }
            if (compare(d, v1)) flag = 0;//���d��1���򷵻�ʧ��
            if (!flag) break;//ʧ�ܾ��˳�ѭ����������2���������
        }
        //��������������������2�������
        if (!flag)
        {
            number = add(number, v2);
            check2000();
        }
    }
}

//����RSA�е�n����(n)=(p-1)*(q-1),d(e�Ԧ�(n)����Ԫ)
RSA::RSA(BigInt p, BigInt q, BigInt e)
{
    BigInt v1;
    v1.set(1);
    this->p = p;
    this->q = q;
    this->e = e;
    n = mul(p, q);//�˷�
    BigInt p1, q1;
    p1 = sub(p, v1);//p-1
    q1 = sub(q, v1);//q-1
    phi = mul(p1, q1);//����ŷ������
    d = calculate_d(phi, e);//ȡ����Ԫ
}

//����
encrypt::encrypt(RSA a, BigInt m)
{
    this->n = a.n;
    this->e = a.e;
    this->m = m;
    this->c = pow(m, e, n);
}

//����
decrypt::decrypt(RSA a, BigInt c)
{
    this->n = a.n;
    this->d = a.d;
    this->c = c;
    this->m = pow(c, d, n);
}

int change(char a)
{
    if (a >= '0' && a <= '9') return (int)(a - 48);
    if (a == 'a' || a == 'A') return 10;
    if (a == 'b' || a == 'B') return 11;
    if (a == 'c' || a == 'C') return 12;
    if (a == 'd' || a == 'D') return 13;
    if (a == 'e' || a == 'E') return 14;
    if (a == 'f' || a == 'F') return 15;
}

void BigInt::set(string a) // 16��������
{
    for (int i = a.length() - 1; i >= 0; i--)
        num[a.length() - i - 1] = change(a[i]);
}

void BigInt::set(string a, int x) //�ַ�������
{
    for (int i = a.length() - 1; i >= 0; i--)
    {
        num[2 * a.length() - 2 * i - 2] = (int)a[i] % 16;
        num[2 * a.length() - 2 * i - 1] = (int)a[i] / 16;
    }
}

int main()
{
    cout << "�ȴ���������" << endl;
    BigInt e;
    e.set(0x10001);
    prime p,q;
    cout<<"�������ɳɹ�"<<endl;
    cout << "��һ������p= ";p.number.print();
    cout << "�ڶ�������q= ";q.number.print();
    cout<<endl;
    RSA jiami(p.number, q.number, e);
    cout << "�����˻�n= "; jiami.n.print();
    cout << "��(n)= "; jiami.phi.print();
    cout << "�����e= "; jiami.e.print();
    cout << "e����Ԫd= "; jiami.d.print();
    cout<<endl;
    cout<<"���������Ϣ"<<endl;
    string message;
    cin>>message;
    BigInt m;
    m.set(message, 0);
    encrypt temp(jiami, m);
    cout << "ת������ m= "; temp.m.print();
    cout << "�������� c= "; temp.c.print();
    decrypt temp_(jiami, temp.c);
    cout << "���Ľ��� m\'= "; temp_.m.print();
}