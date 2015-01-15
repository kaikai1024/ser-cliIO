/*************************************************************************
    > File Name: client.c
    > Author: kkFeng
    > Created Time: 2015年05月11日 
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
#include<time.h>

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
struct RecvPack
{
	PackInfo head;
	char buf[BUFFER_SIZE];
} data;


int main()
{
	//int id = 1;
	/* 发送id */
	int send_id = 0;

	/* 接收id */
	int receive_id = 0;
	/* 服务端地址 */
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(SERVER_PORT);
	socklen_t server_addr_length = sizeof(server_addr);

	/* 创建socket */
	int client_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(client_socket_fd < 0)
	{
		perror("Create Socket Failed:");
		exit(1);
	}

	/* 输入文件名到缓冲区 */
	char file_name[FILE_NAME_MAX_SIZE+1];
	bzero(file_name, FILE_NAME_MAX_SIZE+1);
	printf("Please Input File Name to Server: ");
	scanf("%s", file_name);

	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);
	strncpy(buffer, file_name, strlen(file_name)>BUFFER_SIZE?BUFFER_SIZE:strlen(file_name));

	/* 发送文件名 */
	if(sendto(client_socket_fd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&server_addr,server_addr_length) < 0)
	{
		perror("Send File Name Failed:");
		exit(1);
	}

	/* 打开文件 */
	//FILE *fp = fopen(file_name, "r");
	FILE *fp = fopen("/home/kkf/1G.img", "r");
	if(NULL == fp)
	{
		printf("File:%s Not Found.\n", file_name);
	}
	else
	{
		int len = 0;
		/* time start */
		time_t t_start,t_end;
    		t_start=time(NULL);
		/* 每读取一段数据，便将其发给server */
		while(1)
		{
			PackInfo pack_info;

			if(receive_id == send_id)
			{
				++send_id;
				if((len = fread(data.buf, sizeof(char), BUFFER_SIZE, fp)) > 0)
				{
					data.head.id = send_id;  /* 发送id放进包头,用于标记顺序 */
					data.head.buf_size = len;  /* 记录数据长度 */
					if(sendto(client_socket_fd, (char*)&data, sizeof(data), 0, (struct sockaddr*)&server_addr, 								server_addr_length) < 0)
					{
						perror("Send File Failed:");
						break;
					}
					/* 接收确认消息 */
					recvfrom(client_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&server_addr, 								&server_addr_length);
					receive_id = pack_info.id;	
				}
				else
				{
					break;
				}
			}
			else
			{
				/* 如果接收的id和发送的id不相同,重新发送 */
				if(sendto(client_socket_fd, (char*)&data,sizeof(data),0,(struct sockaddr*)&server_addr, server_addr_length) < 0)
				{
					perror("Send File Failed:");
					break;
				}
				/* 接收确认消息 */
				recvfrom(client_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&server_addr, 									&server_addr_length);
				receive_id = pack_info.id;	
			}
		}
		/* time end */
    		t_end=time(NULL);
    		printf("共用时%.0fs\n",difftime(t_end,t_start));
		/* 关闭文件 */
		fclose(fp);
		printf("File:%s Transfer Successful!\n", file_name);
	}

	close(client_socket_fd);
	return 0;
}
