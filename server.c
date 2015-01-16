/*************************************************************************
    > File Name: server.c
    > Author: kkFeng
    > Created Time: 2015年01月11日 
 ************************************************************************/
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<netdb.h>
#include<stdarg.h>
#include<string.h>
#include "MD5.h"

#define SERVER_PORT 8000
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512

/* package head */
typedef struct
{
	int id;
	int buf_size;
}PackInfo;

/* package send */
struct Pack
{
	PackInfo head;
	char buf[BUFFER_SIZE];
} data;
//
PackInfo pack_info;
PackInfo file_inf;
//
int main()
{
	int filesize = 0;
	int id = 1;
	char file_name[FILE_NAME_MAX_SIZE+1];
	char buffer[BUFFER_SIZE];
	char md5_sum[MD5_LEN + 1];
	/* create socket  */
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVER_PORT);
	int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(server_socket_fd == -1)
	{
		perror("Create Socket Failed:");
		exit(1);
	}

	/* bind */
	if(-1 == (bind(server_socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))))
	{
		perror("Server Bind Failed:");
		exit(1);
	}

	/* translate file */
	while(1)
	{	
		
		struct sockaddr_in client_addr;
		socklen_t client_addr_length = sizeof(client_addr);
		/* file_name*/
		bzero(buffer, BUFFER_SIZE);
		if(recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
		{
			perror("Receive Data Failed:");
			exit(1);
		}
		bzero(file_name,FILE_NAME_MAX_SIZE+1);
		strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
		printf("receive from client :%s\n", file_name);
		/* open the file */
		//FILE *fp = fopen(file_name, "w");
		FILE *fp = fopen("recv.img", "w");
		if(NULL == fp)
		{
			printf("File:\t%s Can Not Open To Write\n", file_name); 
			exit(1);
		}
		/*接收文件信息*/
		if(recvfrom(server_socket_fd, (char*)&file_inf, sizeof(file_inf), 0, 
					(struct sockaddr*)&client_addr,&client_addr_length) < 0)
		{
			perror("receive file information Failed:");
			exit(1);
		}
		printf("******filesize:%d;maxID:%d******\n",file_inf.buf_size,file_inf.id);
		/* receive from client and write */
		while(id <= file_inf.id)
		{
			
			if(recvfrom(server_socket_fd, (char*)&data, sizeof(data), 0, 
					(struct sockaddr*)&client_addr,&client_addr_length) < 0)
			{
				perror("receive file information Failed:");
				exit(1);
			}
			else
			{
				if(data.head.id == id)
				{
					pack_info.id = data.head.id;
					pack_info.buf_size = data.head.buf_size;
					++id;
					/* 发送数据包确认信息 */
					if(sendto(server_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, 
						(struct sockaddr*)&client_addr, client_addr_length) < 0)
					{
						printf("Send confirm information failed!");
					}
					/* 写入文件 */
					if(fwrite(data.buf, sizeof(char), data.head.buf_size, fp) < data.head.buf_size)
					{
						printf("File:\t%s Write Failed\n", file_name);
						break;
					}
				}
				else if(data.head.id < id)  /* 如果是重发的包 */
				{
					pack_info.id = data.head.id;
					pack_info.buf_size = data.head.buf_size;
					/* 重发数据包确认信息 */
					if(sendto(server_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, 
						(struct sockaddr*)&client_addr, client_addr_length) < 0)
					{
						printf("Send confirm information failed!");
					}
				}
			}
		}
		/*得到接收文件长度*/
		filesize=ftell(fp);		
		printf("******receive filesize=%d******\n",filesize);
		/* 关闭文件 */
		fclose(fp);
		/* 计算server端的md5 */
		bzero(md5_sum,MD5_LEN+1);
                if(!CalcFileMD5(file_name, md5_sum))
       		{
       			puts("Error occured!");
      			//return 0;
  		}
  		printf("******the server MD5 sum is :%s ******\n", md5_sum);
		/* 接收client端的文件md5值 */
		bzero(buffer, BUFFER_SIZE);
		if(recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
		{
			perror("Receive md5 Data Failed:");
			exit(1);
		}
		else
		{
			printf("******the client' md5 :%s******\n", md5_sum);
		}		
		/* 验证文件的正确性 */
		if(strcmp(buffer,md5_sum) == 0)
       		{
          		printf("******the file has been modified******\n"); 
			printf("******File:%s Transfer Successful!******\n", file_name);
			/* 发送server端的文件md5值 */
			bzero(buffer, BUFFER_SIZE);
			strncpy(buffer, md5_sum, strlen(md5_sum)>BUFFER_SIZE?BUFFER_SIZE:strlen(md5_sum));
			if(sendto(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr,client_addr_length) < 0)
			{
				perror("Send server‘s md5_sum Failed:");
				exit(1);
			}
          		break;
       		}
 		else
		{
			printf("**File:%s Transfer Wrong!**\n", file_name);
		}

			
	}		
	close(server_socket_fd);
	return 0;
}
