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

/* 包头 */
typedef struct
{
	int id;
	int buf_size;
}PackInfo;

/* 接收包 */
struct SendPack
{
	PackInfo head;
	char buf[BUFFER_SIZE];
} data;


int main()
{
	int id = 1;
	char file_name[FILE_NAME_MAX_SIZE+1];
	char buffer[BUFFER_SIZE];
	char md5_sum[MD5_LEN + 1];
	/* 创建UDP套接口 */
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVER_PORT);

	/* 创建socket */
	int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(server_socket_fd == -1)
	{
		perror("Create Socket Failed:");
		exit(1);
	}

	/* 绑定套接口 */
	if(-1 == (bind(server_socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))))
	{
		perror("Server Bind Failed:");
		exit(1);
	}

	/* 数据传输 */
	while(1)
	{	
		/* 定义一个地址，用于捕获客户端地址 */
		struct sockaddr_in client_addr;
		socklen_t client_addr_length = sizeof(client_addr);

		/* 接收数据 */

		bzero(buffer, BUFFER_SIZE);
		if(recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
		{
			perror("Receive Data Failed:");
			exit(1);
		}

		/* 从buffer中拷贝出file_name */

		bzero(file_name,FILE_NAME_MAX_SIZE+1);
		strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
		printf("%s\n", file_name);
		/* 打开文件，准备写入 */
		FILE *fp = fopen(file_name, "w");
		if(NULL == fp)
		{
			printf("File:\t%s Can Not Open To Write\n", file_name); 
			exit(1);
		}

		/* 从client接收数据，并写入文件 */
		while(1)
		{
			PackInfo pack_info;

			if(recvfrom(server_socket_fd, (char*)&data, sizeof(data), 0, 
					(struct sockaddr*)&client_addr,&client_addr_length) > 0)
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
			else
			{
				break;
			}
		}
		/* 关闭文件 */
		fclose(fp);
		/* 接收client端的文件md5值 */
		bzero(buffer, BUFFER_SIZE);
		if(recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr, &client_addr_length) == -1)
		{
			perror("Receive md5 Data Failed:");
			exit(1);
		}
		/* 从buffer中拷贝出md5 */
		bzero(md5_sum,MD5_LEN+1);
		strncpy(md5_sum, buffer, strlen(buffer)>MD5_LEN?MD5_LEN:strlen(buffer));
		printf("the client md5 :%s\n", md5_sum);
		/* 计算server端的md5 */
		bzero(md5_sum,MD5_LEN+1);
                if(!CalcFileMD5(file_name, md5_sum))
       		{
       			puts("Error occured!");
      			return NULL;
  		}
  		printf("the server MD5 sum is :%s \n", md5_sum);
		/* 验证文件的正确性 */
		if(strcmp(buffer,md5_sum) == 0)
       		{
          		printf("*******the file has been modified*********\n"); 
			printf("**File:%s Transfer Successful!**\n", file_name);
          		break;
       		}
 		else
		{
			printf("**File:%s Transfer Wrong!**\n", file_name);
		}
		/* 发送server端的文件md5值 */
		bzero(buffer, BUFFER_SIZE);
		strncpy(buffer, md5_sum, strlen(md5_sum)>BUFFER_SIZE?BUFFER_SIZE:strlen(md5_sum));
		if(sendto(server_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&client_addr,client_addr_length) < 0)
		{
			perror("Send server‘s md5_sum Failed:");
			exit(1);
		}
			
	}		
	close(server_socket_fd);
	return 0;
}
