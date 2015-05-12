#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_LENGTH 1000
#define MAX_NUM_OF_HEADER 32
#define PORT 8888


typedef struct http_request {
	char method[16];
	char url[128];
	char version[16];
	int header_count;
	char headers[MAX_NUM_OF_HEADER][MAX_LENGTH];
	char body[MAX_LENGTH];
} http_request;


int process_request_get(const struct http_request* request, char* response) {
	return 1;
}

int process_request_post(const struct http_request* request, char* response) {
	return 1;
}

int process_request_put(const struct http_request* request, char* response) {
	return 1;
}

int process_request_delete(const struct http_request* request, char* response) {
	return 1;
}

int process_request(const struct http_request* request, char* response) {
	if( strcmp(request->method, "GET") == 0 ) {
		return process_request_get(request, response);
	} else if( strcmp(request->method, "POST") == 0 ) {
		return process_request_post(request, response);
	} else if( strcmp(request->method, "PUT") == 0 ) {
		return process_request_put(request, response);
	} else if( strcmp(request->method, "DELETE") == 0 ) {
		return process_request_delete(request, response);
	}

	return -1;
}

void parsing_http_request(struct http_request* request, char* message) {
	// printf("%s\n", message);
	char *last;
	last = strtok(message, " ");
	strcpy(request->method, last);

	last = strtok(NULL, " ");
	strcpy(request->url, last);

	last = strtok(NULL, "\n");
	strcpy(request->version, last);

	last += strlen(last) + 1;
	char *body = strstr(last, "\n\n") + 2;
	strcpy(request->body, body);

	char header[MAX_LENGTH] = { 0, };
	strncpy(header, last, body - last - 2);

	request->header_count = 0;
	char *tok = strtok(header, "\n");
	while(tok != NULL) {
		strcpy(request->headers[request->header_count++], tok);
		tok = strtok(NULL, "\n");
	}

	strcpy(request->body, body);
}

int read_all_data(int sock_client, char* buffer) {
	char *tmp = buffer;
	ssize_t ret, len;
	while ((ret = read (sock_client, tmp, MAX_LENGTH) != 0)) { 
		if (ret == -1) {
			fprintf(stderr, "read error\n");
			return 0;
		}
		tmp += ret;
		len += ret;
	}

	return len;
}

void clear_recv_buffer(int sock_client) {
	char buffer[MAX_LENGTH];
	read(sock_client, buffer, MAX_LENGTH);
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

		http_request request;
		parsing_http_request(&request, str);

		char response[MAX_LENGTH] = { 0, };
		if( process_request(&request, response) < 0 ) {
			printf("failed to process request\n");
		} else {
			write(comm_fd, response, strlen(response));
		}

		shutdown(comm_fd, SHUT_WR);
		clear_recv_buffer(comm_fd);
		close(comm_fd);
 	}

	return 0;
}