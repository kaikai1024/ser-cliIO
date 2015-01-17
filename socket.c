#include<netinet/in.h>
#include<errno.h>
#include<stdarg.h>
#include "packge.h"
#include "socketwrapper.h"
#include "socket.h"

#define SERVER_PORT 8000
int init_client(struct sockaddr_in *sockaddr)
{
	bzero(sockaddr, sizeof(struct sockaddr_in));
	(*sockaddr).sin_family = AF_INET;
	(*sockaddr).sin_addr.s_addr = inet_addr("127.0.0.1");
	(*sockaddr).sin_port = htons(SERVER_PORT);

	/* create socket */
	int client_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(client_socket_fd < 0)
	{
		perror("Create Socket Failed:");
		exit(1);
	}
	return client_socket_fd;
}

int init_server(struct sockaddr_in *sockaddr)
{
	/* create socket  */
	bzero(sockaddr, sizeof(struct sockaddr_in));
	(*sockaddr).sin_family = AF_INET;
	(*sockaddr).sin_addr.s_addr = htonl(INADDR_ANY);
	(*sockaddr).sin_port = htons(SERVER_PORT);
	int server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(server_socket_fd == -1)
	{
		perror("Create Socket Failed:");
		exit(1);
	}

	/* bind */
	if(-1 == (bind(server_socket_fd,(struct sockaddr*)sockaddr,sizeof(struct sockaddr_in))))
	{
		perror("Server Bind Failed:");
		exit(1);
	}
	return server_socket_fd;	
}
