all:server client
server: server.o  socket.o file_translate.o MD5.o
client: client.o  socket.o file_translate.o MD5.o
	gcc -o server server.o  socket.o file_translate.o MD5.o
	gcc -o client client.o  socket.o file_translate.o MD5.o

server.o:server.c  socket.c socket.h file_translate.c file_translate.h MD5.c MD5.h
	gcc -c server.c  socket.c file_translate.c MD5.c

client.o:client.c  socket.c socket.h file_translate.c file_translate.h MD5.c MD5.h
	gcc -c client.c socket.c file_translate.c MD5.c

socket.o:socket.c socket.h
	gcc -c socket.c

fileoperate.o:file_translate.c file_translate.h
	gcc -c file_translate.c

MD5.o:MD5.c MD5.h
	gcc -c MD5.c

clean:
	rm server client server.o client.o   socket.o file_translate.o MD5.o
