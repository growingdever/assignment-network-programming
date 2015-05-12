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
	strcat(response, "HTTP/1.0 200 OK\r\n");
	strcat(response, "Server: myserver\r\n");
	strcat(response, "Content-Type: text/html\r\n");
	strcat(response, "\r\n\r\n");
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

	return 1;
}

char* tokenizing_multi_character_delim(char* dst, char* src, char* delim) {
	char *next = strstr(src, delim);
	if( next == NULL ) {
		strcpy(dst, src);
		return NULL;
	}
	strncpy(dst, src, next - src);
	return next + strlen(delim);
}

void parsing_http_request(struct http_request* request, char* message) {
	char *last = message;
	char header_part[MAX_LENGTH] = { 0, };
	
	last = tokenizing_multi_character_delim(request->method, last, " ");
	last = tokenizing_multi_character_delim(request->url, last, " ");
	last = tokenizing_multi_character_delim(request->version, last, "\r\n");
	last = tokenizing_multi_character_delim(header_part, last, "\r\n\r\n");
	last = tokenizing_multi_character_delim(request->body, last, "\r\n");
	
	request->header_count = 0;
	char *tok = strtok(header_part, "\n");
	while(tok != NULL) {
		strcpy(request->headers[request->header_count++], tok);
		tok = strtok(NULL, "\n");
	}
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

void print_error(const char* content) {
	fprintf(stderr, "%s\n", content);
}

int main() {
	printf("start\n");
	
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_port = htons(PORT);
	
	int opt_sock_reuse = 1;
	struct timeval tv_timeout = { 3, 500000 };
	int sock_listen, sock_client;
	sock_listen = socket(AF_INET, SOCK_STREAM, 0);
	if( setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, (char *)&opt_sock_reuse, (int)sizeof(opt_sock_reuse)) ) {
		print_error("failed to setsockopt");
		return 1;
	}
	
	bind(sock_listen, (struct sockaddr *) &servaddr, sizeof(servaddr));
	
	printf("start listen\n");
	listen(sock_listen, 10);
	if( sock_listen < 0 ) {
		printf("failed to create listening socket\n");
		return 0;
	}
	
	while(1) {
		sock_client = accept(sock_listen, (struct sockaddr*) NULL, NULL);
		if( sock_client < 0 ) {
			printf("failed to create client socket\n");
			return 0;
		}
		
		char str[MAX_LENGTH] = { 0, };
		if( ! read_all_data(sock_client, str) ) {
			return 0;
		}
		
		http_request request;
		parsing_http_request(&request, str);
		
		char response[MAX_LENGTH] = { 0, };
		if( process_request(&request, response) < 0 ) {
			printf("failed to process request\n");
		} else {
			write(sock_client, response, strlen(response));
		}
		
		shutdown(sock_client, SHUT_WR);
		clear_recv_buffer(sock_client);
		close(sock_client);
	}
	
	return 0;
}