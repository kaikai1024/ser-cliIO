#ifndef SOCKET_H
#define SOCKET_H

#include<stdlib.h>
#include<sys/types.h>
#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>

int init_client();

int init_server();

#endif /* socket.h */
