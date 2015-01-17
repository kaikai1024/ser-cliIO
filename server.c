/********************************************************************/
/* Copyright (C) MC2Lab-USTC, 2012                                  */
/*                                                                  */
/*  FILE NAME             :  server.c                               */
/*  PRINCIPAL AUTHOR      :  kkFeng                                 */
/*  SUBSYSTEM NAME        :  DB                                     */
/*  MODULE NAME           :  server                                 */
/*  LANGUAGE              :  C                                      */
/*  TARGET ENVIRONMENT    :  Linux                                  */
/*  DATE OF FIRST RELEASE :  2015/01/15                             */
/*  DESCRIPTION           :  The server of translate 1G file        */
/********************************************************************/

/*
 * Revision log:
 * Created by kkFeng,2015/01/15
 *
 */
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<stdarg.h>
#include<string.h>
#include "MD5.h"
#include "packge.h"
#include "socketwrapper.h"

/*packge*/
PackInfo pack_info;
PackInfo file_inf;

int filesize = 0;
int id = 1;
char file_name[FILE_NAME_MAX_SIZE+1];
char buffer[BUFFER_SIZE];
char md5_sum[MD5_LEN + 1];
/*main */
int main()
{
	struct sockaddr_in server_addr;
	int server_socket_fd = init_server(&server_addr);
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

		/*receive file information*/
		if(recvfrom(server_socket_fd, (char*)&file_inf, sizeof(file_inf), 0, 
					(struct sockaddr*)&client_addr,&client_addr_length) < 0)
		{
			perror("receive file information Failed:");
			exit(1);
		}
		printf("******filesize:%d ; maxID:%d******\n",file_inf.buf_size,file_inf.id);

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
					/* Send confirm information */
					if(sendto(server_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, 
						(struct sockaddr*)&client_addr, client_addr_length) < 0)
					{
						printf("Send confirm information failed!");
					}
					/* write file */
					if(fwrite(data.buf, sizeof(char), data.head.buf_size, fp) < data.head.buf_size)
					{
						printf("File:\t%s Write Failed\n", file_name);
						break;
					}
				}
				/* re-send pack */
				else if(data.head.id < id)  
				{
					pack_info.id = data.head.id;
					pack_info.buf_size = data.head.buf_size;
					/* re-send confirm information */
					if(sendto(server_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, 
						(struct sockaddr*)&client_addr, client_addr_length) < 0)
					{
						printf("Send confirm information failed!");
					}
				}
			}
		}
		/*get the size of file received*/
		filesize=ftell(fp);		
		printf("******receive filesize=%d******\n",filesize);
		/* close the file */
		fclose(fp);
		/* calc the server'md5 */
		bzero(md5_sum,MD5_LEN+1);
                if(!CalcFileMD5(file_name, md5_sum))
       		{
       			puts("Error occured!");
      			//return 0;
  		}
  		printf("******the server MD5 sum is :%s ******\n", md5_sum);
		/* receive the client'md5 */
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
		/* confirm the file */
		if(strcmp(buffer,md5_sum) == 0)
       		{
          		printf("******the file has been modified******\n"); 
			printf("******File:%s Transfer Successful!******\n", file_name);
			/* send the server'md5 */
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
