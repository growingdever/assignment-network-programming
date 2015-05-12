#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_LENGTH 1000
#define PORT 8888

int read_all_data(int sock_client, char* buffer) {
	char *tmp = buffer;
	ssize_t ret;
	while ((ret = read (sock_client, tmp, MAX_LENGTH) != 0)) { 
		if (ret == -1) {
			fprintf(stderr, "read error\n");
			return 0;
		}
		tmp += ret;
	}

	return 1;
}

int main() {
	printf("start\n");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_port = htons(PORT); 
	
	int listen_fd, comm_fd;
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));
 
	printf("start listen\n");
	listen(listen_fd, 10);
	if( listen_fd < 0 ) {
		printf("failed to create listening socket\n");
		return 0;
	}
 
 	while(1) {
 		comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);
		if( comm_fd < 0 ) {
			printf("failed to create client socket\n");
			return 0;
		}

		char str[MAX_LENGTH] = { 0, };
		if( ! read_all_data(comm_fd, str) ) {
			return 0;
		}

		write(comm_fd, str, strlen(str));
		close(comm_fd);
 	}

	return 0;
}