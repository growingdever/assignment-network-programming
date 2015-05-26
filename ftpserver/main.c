#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "data_structure.h"
#include "util.h"

#define PORT_FTP_COMMAND 8888
#define STR_EQUAL(p, q) strcmp(p, q) == 0


int init_listening_socket(struct sockaddr_in*, int);
int handle_socket(int);
void handle_command_pwd(int, char*);


int main(int argc, char* argv[]) {
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	int sock_listen = init_listening_socket(&servaddr, PORT_FTP_COMMAND);

	bind(sock_listen, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(sock_listen, 10);

	while(1) {
		printf("read to accept %d\n", sock_listen);
		int sock_client = accept(sock_listen, (struct sockaddr*) NULL, NULL);
		printf("accepted %d\n", sock_client);
		if( sock_client < 0 ) {
			ERROR_LOGGING("failed to accept client socket")
			continue;
		}

		if( handle_socket(sock_client) < 0 ) {
			ERROR_LOGGING("failed to handle request")
			continue;
		}
	}

	return 0;
}


int init_listening_socket(struct sockaddr_in* addr, int port) {
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htons(INADDR_ANY);
	addr->sin_port = htons(port);

	int sock_listen = socket(AF_INET, SOCK_STREAM, 0);
	if( sock_listen < 0 ) {
		ERROR_LOGGING("failed to create listening socket")
	}

	int opt_sock_reuse = 1;
	if( setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, (char *)&opt_sock_reuse, (int)sizeof(opt_sock_reuse)) ) {
		ERROR_LOGGING("failed to setsockopt")
	}

	return sock_listen;
}


int handle_socket(int sock) {
	char str[MAX_LENGTH] = { 0, };
	ssize_t length = read_line(sock, str, MAX_LENGTH);
	if( length <= 0 ) {
		return -1;
	}

	char *command = strtok(str, " \r\n");
	if( STR_EQUAL(command, "PWD") ) {
		handle_command_pwd(sock, str);
	}

	shutdown(sock, SHUT_WR);
	clear_recv_buffer(sock);
	close(sock);

	return 1;
}

void handle_command_pwd(int sock, char* line) {
	char path[MAX_LENGTH] = { 0, };
	getcwd(path, sizeof(path));

	write(sock, path, strlen(path));
}