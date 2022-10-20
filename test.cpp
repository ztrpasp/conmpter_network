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
	// //首先创建一个time_t 类型的变量nowtime
	// struct tm* ptminfo;
	// //然后创建一个新时间结构体指针 p 
	// time(&nowtime);
	// //使用该函数就可得到当前系统时间，使用该函数需要将传入time_t类型变量nowtime的地址值。
	// ptminfo = localtime(&nowtime);
	// //由于此时变量nowtime中的系统时间值为日历时间，我们需要调用本地时间函数p=localtime（time_t* nowtime）将nowtime变量中的日历时间转化为本地时间，存入到指针为p的时间结构体中。不改的话，可以参照注意事项手动改。
	// printf("current: %02d-%02d-%02d %02d:%02d:%02d\n",
    //         ptminfo->tm_year + 1900, ptminfo->tm_mon + 1, ptminfo->tm_mday,
    //         ptminfo->tm_hour, ptminfo->tm_min, ptminfo->tm_sec);
	// //控制格式输出

    // time_t timep;
    // time (&timep);
    // char tmp[30];
    // strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S",localtime(&timep) );
    // cout<<tmp;
	return 0;
}