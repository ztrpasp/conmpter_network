#include<iostream>
#include<stdio.h>
#include<time.h>
using namespace std;
int main()
{
    char  name [50];
    cin>>name;
    cout<<name;
    system("pause");
    return 0;
    // time_t nowtime;
	// //���ȴ���һ��time_t ���͵ı���nowtime
	// struct tm* ptminfo;
	// //Ȼ�󴴽�һ����ʱ��ṹ��ָ�� p 
	// time(&nowtime);
	// //ʹ�øú����Ϳɵõ���ǰϵͳʱ�䣬ʹ�øú�����Ҫ������time_t���ͱ���nowtime�ĵ�ֵַ��
	// ptminfo = localtime(&nowtime);
	// //���ڴ�ʱ����nowtime�е�ϵͳʱ��ֵΪ����ʱ�䣬������Ҫ���ñ���ʱ�亯��p=localtime��time_t* nowtime����nowtime�����е�����ʱ��ת��Ϊ����ʱ�䣬���뵽ָ��Ϊp��ʱ��ṹ���С����ĵĻ������Բ���ע�������ֶ��ġ�
	// printf("current: %02d-%02d-%02d %02d:%02d:%02d\n",
    //         ptminfo->tm_year + 1900, ptminfo->tm_mon + 1, ptminfo->tm_mday,
    //         ptminfo->tm_hour, ptminfo->tm_min, ptminfo->tm_sec);
	// //���Ƹ�ʽ���

    // time_t timep;
    // time (&timep);
    // char tmp[30];
    // strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
    // cout<<tmp;
	return 0;
}