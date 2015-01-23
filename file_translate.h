void file_receive(int server_socket_fd,struct sockaddr_in client_addr,FILE *fp,int n);
int file_translate(int client_socket_fd,struct sockaddr_in server_addr,FILE *fp);
