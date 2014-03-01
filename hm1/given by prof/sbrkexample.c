#include <stdio.h>
#include <unistd.h>

int main()
{
 /*分配10个字节的空间，返回该空间的首地址*/
 void* p = sbrk(12);
 void* p2 = sbrk(4);
 void* p3 = sbrk(4);
 void* p4 = sbrk(4);
 printf("p=%p\n", p);
 printf("p2=%p\n", p2);
 printf("p3=%p\n", p3);
 printf("p4=%p\n", p4);
 /*用参数为0来获取未分配空间的开始位置*/
 void* p0 = sbrk(0);
 printf("p0=%p\n", p0);
 void* p5 = sbrk(-4);
 printf("p5=%p\n", p5);
 printf("pid=%d\n", getpid());
 sleep(10);
 /*当释放到一个页面的开始位置时，整个页面会被操作系统回收*/
 sbrk(-20);
 
 /*
 int* pi = p;
 *pi = 10;
 *(pi+1) = 20;
 *(pi+2) = 30;
 *(pi+1023) = 1023;
  *(pi+1024) = 1024;
 */
 
 
 while(1);
}
