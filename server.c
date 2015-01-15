
#include<stdio.h> 			/* perror */
#include<stdlib.h>			/* exit	*/
#include<sys/types.h>		/* WNOHANG */
#include<sys/wait.h>		/* waitpid */
#include<string.h>			/* memset */
//
#include <sys/time.h>
#include <pthread.h>
//
#include "socketwrapper.h" 	/* socket layer wrapper */

#define	true		1
#define false		0

#define MYPORT 		3490                /* 监听的端口 */ 
#define BACKLOG 	10                 	/* listen的请求接收队列长度 */
//
int sendexit=0;		//控制发送线程状态的全局变量
int recvexit=0;		//控制接收程状态的全局变量
int filesize=1;		//记录文件大小的全局变量
int recvsize=0;		//记录接收文件大小的全局变量
int id=1;

int sockfd;            /* 监听端口，数据端口 */
char * name = NULL;	//记录文件名

struct baohead		//包头
{
int size;
int id;
int recvsize;
};
//typedef baohead ElemType;
struct baohead datahead;
struct recvbuf		//包格式
{
//ElemType head;		//包头
struct baohead head;
char buf[1024];		//存放数据的变量
int bufSize;		//存放数据长度的变量

};
struct recvbuf data;

void* recvfunc(void* p);		//接收线程
//
int theiraddr_len ;
struct sockaddr_in sa;         /* 自身的地址信息 */ 
struct sockaddr_in their_addr; /* 连接对方的地址信息 */ 

int main() 
{

	//
	pthread_t tid;
	pthread_attr_t attr;
	//
	if ((sockfd = Socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
	{
		perror("socket"); 
		exit(1); 
	}
	//setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*) &opt, sizeof(opt));

	sa.sin_family = AF_INET;
	sa.sin_port = Htons(MYPORT);         /* 网络字节顺序 */
	sa.sin_addr.s_addr = INADDR_ANY;     /* 自动填本机IP */
	memset(&(sa.sin_zero),0, 8);            /* 其余部分置0 */
	theiraddr_len = sizeof(their_addr);

	if ( Bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) == -1) 
	{
		perror("bind");
		exit(1);
	}
	recvfrom(sockfd, name, 100, 0,(struct sockaddr *)&their_addr,&theiraddr_len);
	recvfrom(sockfd, &filesize, 100, 0,(struct sockaddr *)&their_addr,&theiraddr_len);
	printf("receive the file :%s,with the size :%d",name,filesize);
       	//r = fcntl(sockfd, F_GETFL, 0);
       	//fcntl(sockfd, F_SETFL, r & ~O_NONBLOCK);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&tid, &attr, recvfunc, NULL) != 0) 
	{
		fprintf(stderr, "pthread_create failed\n");
		return -1;
	}
	while(recvexit<2)//如果接收线程未退出，则主线程睡一段时间。
	{
		//Sleep(100);
	}
	printf("退出成功!\n");
	return 0;
}
void* recvfunc(void* p)
{
	recvexit=1;	//告诉主线程，接收线程已经创建成功。
	FILE * fp;
	fp=fopen("/home/kkf/recv.img", "wb");//如果原文件存在，则将原来文件删除。并新建一个空文件
	fclose(fp);
	fp=fopen("/home/kkf/recv.img", "ab");//打开那个空文件。
	fd_set fdread;//建立集合
	while( recvexit==1 )
	{
		FD_ZERO(&fdread);//清空集合
		FD_SET(sockfd,&fdread);//将socket放进集合内
		struct timeval val;//设置超时时间
		int Result=select(0,&fdread,0,0,&val);//检查sokcet的可读性
		if(Result==-1)//错误则退出线程
		{
			printf("select error %s\n",strerror(errno));
                        exit(-1);
		}
		else if(Result==0)//没数据到则马上返回。
		{
			printf("select timeout,continue circle\n");
			continue;
		}
	
		int rec = recvfrom(sockfd, (char *)(&data), 2048, 0,(struct sockaddr *)&their_addr,&theiraddr_len);//读取到来的数据报
		if( rec > 0 )
		{
			if(data.head.id == id)//包ID是所期望的,则读入数据
			{
				// printf("id=%d\n",data.head.id);
				// printf("bufsize=%d\n",data.bufSize);
				fwrite( data.buf, sizeof(char), data.bufSize, fp);
				recvsize=recvsize+data.bufSize;//记录新的接收到的文件长度
				// printf("recvsize=%d\n",recvsize);
				filesize=data.head.size;
				datahead.id= data.head.id;
				datahead.recvsize=recvsize;
				datahead.size=filesize;
				//发送收到数据的确认包
				int d=sendto(sockfd,(char*)(&datahead),sizeof(datahead),0,(struct sockaddr *)&their_addr,sizeof(their_addr));
				if(d>0)
				{
					// printf("发送收到数据包的确认信息成功!\n");
				}
				id=id+1;
			}
		}
		else if(data.head.id < id)//如果是已经接收过的包的重发包，则丢弃，并重发一次确认包告诉发送端该包已收。
		{
			datahead.id = data.head.id;
			datahead.recvsize=recvsize;
			datahead.size=filesize;
			
			int d=sendto(sockfd,(char*)(&datahead),sizeof(datahead),0,(struct sockaddr *)&their_addr,sizeof(their_addr));
			if(d>0)
			{
				// printf("发送收到数据包的确认信息成功!\n");
			}
		
		
		}
	
		if(filesize == recvsize )//如果接收到的文件长度等于文件大小，
		{ //等发送一个确认包告诉发送端文件已接收完成，并退出自己的接收线程
			datahead.id = data.head.id;
			datahead.recvsize=-1;
			datahead.size=filesize;
			printf("filesize=%d\n",filesize);
			printf("recvsize=%d\n",recvsize);
			printf("文件接收完成!\n");
			int d=sendto(sockfd,(char*)(&datahead),sizeof(datahead),0,(struct sockaddr *)&their_addr,sizeof(their_addr));
			sendexit=2;
			recvexit=2;
			break;
		}
	}
	fclose(fp);
	recvexit=2;
	return NULL;
}
