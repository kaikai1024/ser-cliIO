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
#include<time.h>
#include "MD5.h"
#include "packge.h"
#include "socketwrapper.h"
#include "file_translate.h"
/*packge*/
PackInfo file_inf;
/*out_side*/
int filesize = 0;
char file_name[FILE_NAME_MAX_SIZE+1];
char buffer[BUFFER_SIZE];
char md5_sum[MD5_LEN + 1];
/*client_addr*/
struct sockaddr_in client_addr;
socklen_t client_addr_length = sizeof(client_addr);
/*main */
int main()
{
	struct sockaddr_in server_addr;
	int server_socket_fd = init_server(&server_addr);
	/* translate file info */
	/* receive the file_name*/
	bzero(buffer, BUFFER_SIZE);
	if(recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,
		(struct sockaddr*)&client_addr, &client_addr_length) == -1)
	{
		perror("Receive Data Failed:");
		exit(1);
	}
	bzero(file_name,FILE_NAME_MAX_SIZE+1);
	strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer));
	printf("receive from client :%s\n", file_name);
	/*receive other file information*/
	if(recvfrom(server_socket_fd, (char*)&file_inf, sizeof(file_inf), 0, 
					(struct sockaddr*)&client_addr,&client_addr_length) < 0)
	{
		perror("receive file information Failed:");
		exit(1);
	}
	printf("******filesize:%d ; maxID:%d******\n",file_inf.buf_size,file_inf.id);
	/* open the file */
	FILE *fp = fopen("recv.img", "w");
	if(NULL == fp)
	{
		printf("File:\t%s Can Not Open To Write\n", file_name); 
		exit(1);
	}
	while(1)
	{
		/* time start */
		time_t t_start,t_end;
    		t_start=time(NULL);
		/*file receive */
		file_receive(server_socket_fd,client_addr,fp,file_inf.id);
		/* time end */
    		t_end=time(NULL);
    		printf("******use time:%.0fs******\n",difftime(t_end,t_start));
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
  		printf("******the server' MD5 sum is :%s ******\n", md5_sum);
		/* receive the client'md5 */
		bzero(buffer, BUFFER_SIZE);
		if(recvfrom(server_socket_fd, buffer, BUFFER_SIZE,0,
			(struct sockaddr*)&client_addr, &client_addr_length) == -1)
		{
			perror("Receive md5 Data Failed:");
			exit(1);
		}
		else
		{
			printf("******the client' MD5 sum is :%s******\n", md5_sum);
		}		
		/* confirm the file */
		if(strcmp(buffer,md5_sum) == 0)
       		{
          		printf("******the file has been modified******\n"); 
			printf("******File:%s Transfer Successful!******\n", file_name);
			/* send the server'md5 */
			bzero(buffer, BUFFER_SIZE);
			strncpy(buffer, md5_sum, strlen(md5_sum)>BUFFER_SIZE?BUFFER_SIZE:strlen(md5_sum));
			if(sendto(server_socket_fd, buffer, BUFFER_SIZE,0,
				(struct sockaddr*)&client_addr,client_addr_length) < 0)
			{
				perror("Send server' md5_sum Failed:");
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
