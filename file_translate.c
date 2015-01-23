#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<netdb.h>
#include<stdarg.h>
#include<string.h>
#include "packge.h"
#include "socketwrapper.h"
#include "file_translate.h"


int file_translate(int client_socket_fd,struct sockaddr_in server_addr,FILE *fp)
{	
	
	socklen_t server_addr_length = sizeof(server_addr);	
	int len = 0;
	int send_id = 0;
	int receive_id = 0;
	/* read the buf and send to the server */
	while(1)
	{
		PackInfo pack_head;

		if(receive_id == send_id)
		{
			++send_id;
			if((len = fread(data.buf, sizeof(char), BUFFER_SIZE, fp)) > 0)
			{
				/* put the send_id in the head  */
				data.head.id = send_id;  
				/* bufsize */
				data.head.buf_size = len;  
				//printf("id=%d\n",send_id);
				//printf("sendsize=%d\n",len);
				if(sendto(client_socket_fd, (char*)&data, sizeof(data), 0, 
					(struct sockaddr*)&server_addr, server_addr_length) < 0)
				{
					perror("Send File Failed:");
					break;
					
				}
				/* receive confirm information */
				recvfrom(client_socket_fd, (char*)&pack_head, sizeof(pack_head), 0,
						(struct sockaddr*)&server_addr, &server_addr_length);
				receive_id = pack_head.id;	
			}
			else return 0;
		}
		else
		{
			/* if id is dif  re-send  */
			if(sendto(client_socket_fd, (char*)&data,sizeof(data),0,
				(struct sockaddr*)&server_addr, server_addr_length) < 0)
			{
				perror("Send File Failed:");
				break;
				
			}

			/* receive confirm information */
			recvfrom(client_socket_fd, (char*)&pack_head, sizeof(pack_head), 0,
 				(struct sockaddr*)&server_addr, &server_addr_length);
			receive_id = pack_head.id;	
		}
	}
	return send_id;
}
/*file_receive*/
void file_receive(int server_socket_fd,struct sockaddr_in client_addr,FILE *fp,int n)
{
	socklen_t client_addr_length = sizeof(client_addr);
	int id = 1;
	PackInfo pack_head;
	/* receive from client and write */
	while(id <= n)
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
				pack_head.id = data.head.id;
				pack_head.buf_size = data.head.buf_size;
				++id;
				/* Send confirm information */
				if(sendto(server_socket_fd, (char*)&pack_head, sizeof(pack_head), 0, 
						(struct sockaddr*)&client_addr, client_addr_length) < 0)
				{
					printf("Send confirm information failed!");
				}
				/* write file */
				if(fwrite(data.buf, sizeof(char), data.head.buf_size, fp) 
						< data.head.buf_size)
				{
					printf(" Write Failed\n");
					break;
				}
			}
			/* re-send pack */
			else if(data.head.id < id)  
			{
				pack_head.id = data.head.id;
				pack_head.buf_size = data.head.buf_size;
				/* re-send confirm information */
				if(sendto(server_socket_fd, (char*)&pack_head, sizeof(pack_head), 0, 
						(struct sockaddr*)&client_addr, client_addr_length) < 0)
				{
					printf("Send confirm information failed!");
				}
			}
		}
	}
	
}
