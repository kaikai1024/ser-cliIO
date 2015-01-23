/********************************************************************/
/* Copyright (C) USTC, 2015                                         */
/*                                                                  */
/*  FILE NAME             :  client.c                               */
/*  PRINCIPAL AUTHOR      :  kkFeng                                 */
/*  SUBSYSTEM NAME        :  DB                                     */
/*  MODULE NAME           :  client                                 */
/*  LANGUAGE              :  C                                      */
/*  TARGET ENVIRONMENT    :  Linux                                  */
/*  DATE OF FIRST RELEASE :  2015/01/15                             */
/*  DESCRIPTION           :  The client of of translate 1G file     */
/********************************************************************/

/*
 * Revision log:
 * Created by kkFeng,2015/01/15
 *
 */
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<netdb.h>
#include<stdarg.h>
#include<string.h>

#include "MD5.h"
#include "packge.h"
#include "socketwrapper.h"
#include "file_translate.h"

/*file informaton*/
PackInfo file_inf;
char file_name[FILE_NAME_MAX_SIZE+1];
char buffer[BUFFER_SIZE];
char md5_sum[MD5_LEN + 1];
/*id*/
int filesize = 0;		
/*server_addr*/
struct sockaddr_in server_addr;
socklen_t server_addr_length = sizeof(server_addr);
/*file_translate*/

/*main*/
int main()
{
	/*init socket*/
	int client_socket_fd = init_client(&server_addr);
		
	/* input the file Path */	
	bzero(file_name, FILE_NAME_MAX_SIZE+1);
	printf("Please Input File Path to Server: ");
	scanf("%s", file_name);

	bzero(buffer, BUFFER_SIZE);
	strncpy(buffer, file_name, strlen(file_name)>BUFFER_SIZE?BUFFER_SIZE:strlen(file_name));

	/* send the file path  */
	if(sendto(client_socket_fd, buffer, BUFFER_SIZE,0,
			(struct sockaddr*)&server_addr,server_addr_length) < 0)
	{
		perror("Send File Name Failed:");
		exit(1);
	}

	/* open the file */
	FILE *fp = fopen(file_name, "r");
	/*put the ptr of the file to the end */
	fseek(fp,0,SEEK_END);
	/*get the file size*/		
	filesize=ftell(fp);		
	//printf("filesize=%d\n",filesize);
	/*move the ptr back to the head */
	rewind(fp); 

	/*send the filesize and the id:file_inf*/
	file_inf.buf_size = filesize;
	file_inf.id = filesize/BUFFER_SIZE;
	printf("******filesize:%d ; maxID:%d******\n",file_inf.buf_size,file_inf.id);
	if(sendto(client_socket_fd, (char*)&file_inf, sizeof(file_inf), 0 , 
			(struct sockaddr*)&server_addr,server_addr_length) < 0)
	{
		perror("Send File information Failed:");
		exit(1);
	}
	/*send the file*/


	file_translate( client_socket_fd, server_addr,fp);

	
	/* close the file */			
	fclose(fp);		

	/* calc the file'md5 */
       	if(!CalcFileMD5(file_name, md5_sum))
      	{
       		puts("Error occured!");
      	}
       	printf("******the md5 of the file %s is:\n******%s******\n",file_name,md5_sum);
	/* send the file'md5 */
	bzero(buffer, BUFFER_SIZE);
	strncpy(buffer, md5_sum, strlen(md5_sum)>BUFFER_SIZE?BUFFER_SIZE:strlen(md5_sum));	
	if(sendto(client_socket_fd, buffer, BUFFER_SIZE,0,
		(struct sockaddr*)&server_addr,server_addr_length) < 0)
	{
		perror("******Send File Name Failed:******");
		exit(1);
	}
	/* receive the server'md5 */
	bzero(buffer, BUFFER_SIZE);
	if(recvfrom(client_socket_fd, buffer, BUFFER_SIZE,0,
		(struct sockaddr*)&server_addr, &server_addr_length) == -1)
	{
		perror("Receive md5 Data Failed:");
		exit(1);
	}
	else
	{
		printf("******server's md5: \n******%s******\n", buffer);
	}
		
       	/* confirm the md5 */
       	if(strcmp(buffer,md5_sum) == 0)
       	{
          	printf("******the file has been modified******\n"); 
		printf("******File:%s Transfer Successful!******\n", file_name);
		exit(1);
       	}
 	else
	{
		printf("******File:%s Transfer Wrong!******\n", file_name);
	}
				
	//close(client_socket_fd);
	return 0;
}

