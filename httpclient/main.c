#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "data_structure.h"
#include "util.h"

#define ARGV_INDEX_IP 1
#define ARGV_INDEX_PORT 2


void work(int sock);
void read_response(int sock, http_response* response);
void build_request_get(char* dest, const char* url);
void build_request_post(char* dest, const char* url, const char* content);
void build_request_put(char* dest, const char* url, const char* content);
void build_request_delete(char* dest, const char* url);
void pretty_print(char* title, char* content);


const char* str_ip;
const char* str_port;

int console_width;


int main(int argc, char const *argv[])
{
	if( argc <= 1 ) {
		ERROR_LOGGING("usage:\narg1 : ip, arg2 : port");
	}
	
	str_ip = argv[ARGV_INDEX_IP];
	str_port = argv[ARGV_INDEX_PORT];

	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	console_width = w.ws_col;
	
	while(1) {
		int sock_server;
		if( (sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			ERROR_LOGGING("failed to create socket");
		}
		
		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(argv[ARGV_INDEX_IP]);
		server_addr.sin_port = htons( atoi(argv[ARGV_INDEX_PORT]) );
		
		if( connect(sock_server, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0 ) {
			close(sock_server);
			sleep(1);
			continue;
		}
		
		work(sock_server);
		
		shutdown(sock_server, SHUT_WR);
		clear_recv_buffer(sock_server);
		close(sock_server);
	}
	
	return 0;
}

void work(int sock) {
	char buffer[MAX_LENGTH] = { 0, };
	char command[16] = { 0, };
	char url[32] = { 0, };
	printf("command : ");
	scanf("%s", command);
	printf("url : ");
	scanf("%s", url);

	str_tolower(command);
	str_tolower(url);
	
	if( strcmp(command, "get") == 0 ) {
		build_request_get(buffer, url);
	} else if( strcmp(command, "post") == 0 ) {
		char content[MAX_LENGTH] = { 0, };
		printf("body : ");
		int length = my_gets(content);
		
		build_request_post(buffer, url, content);
	} else if( strcmp(command, "put") == 0 ) {
		char content[MAX_LENGTH] = { 0, };
		printf("body : ");
		int length = my_gets(content);
		
		build_request_put(buffer, url, content);
	} else if( strcmp(command, "delete") == 0 ) {
		build_request_delete(buffer, url);
	} else {
		return;
	}

	pretty_print(" R E Q E S T ", buffer);

	write(sock, buffer, strlen(buffer));

	http_response response;
	memset(&response, 0, sizeof(response));
	
	read_response(sock, &response);
	char response_string[MAX_LENGTH] = { 0, };
	tostring_response(response_string, &response);
	pretty_print(" R E S P O N S E ", response_string);
}

void read_response(int sock, http_response* response) {
	char buffer[MAX_LENGTH] = { 0, };
	int length;
	
	memset(buffer, 0, MAX_LENGTH);
	length = (int)read_line(sock, buffer, MAX_LENGTH);
	strcpy(response->version, strtok(buffer, " "));
	response->status_code = atoi(strtok(NULL, " "));
	strcpy(response->status_message, strtok(NULL, "\r\n"));
	
	while(1) {
		memset(buffer, 0, MAX_LENGTH);
		int length = (int)read_line(sock, buffer, MAX_LENGTH);
		if( length == 2 && strcmp(buffer, "\r\n") == 0 ) {
			break;
		}
		
		strcpy(response->headers[response->num_of_header], buffer);
		response->num_of_header++;
	}
	
	while(1) {
		memset(buffer, 0, MAX_LENGTH);
		int length = (int)read_line(sock, buffer, MAX_LENGTH);
		if( length == 0 ) {
			break;
		}
		
		strcat(response->body, buffer);
	}
}

void build_request_get(char* dest, const char* url) {
	char buffer[MAX_LENGTH];

	sprintf(buffer, "GET %s HTTP/1.1\r\n", url);
	strcat(dest, buffer);
	sprintf(buffer, "Host: %s\r\n", str_ip);
	strcat(dest, buffer);
	sprintf(buffer, "\r\n");
	strcat(dest, buffer);
}

void build_request_post(char* dest, const char* url, const char* body) {
	char buffer[MAX_LENGTH];
	int length = 0;
	if( body != NULL ) {
		length = (int)strlen(body);
	}

	sprintf(buffer, "POST %s HTTP/1.1\r\n", url);
	strcat(dest, buffer);
	sprintf(buffer, "Host: %s\r\n", str_ip);
	strcat(dest, buffer);
	sprintf(buffer, "Content-Length: %d\r\n", length);
	strcat(dest, buffer);
	sprintf(buffer, "\r\n");
	strcat(dest, buffer);
	if( body != NULL ) {
		sprintf(buffer, "%s\r\n", body);
		strcat(dest, buffer);
	}
}

void build_request_put(char* dest, const char* url, const char* body) {
	char buffer[MAX_LENGTH];
	int length = 0;
	if( body != NULL ) {
		length = (int)strlen(body);
	}

	sprintf(buffer, "PUT %s HTTP/1.1\r\n", url);
	strcat(dest, buffer);
	sprintf(buffer, "Host: %s\r\n", str_ip);
	strcat(dest, buffer);
	sprintf(buffer, "Content-Length: %d\r\n", length);
	strcat(dest, buffer);
	sprintf(buffer, "\r\n");
	strcat(dest, buffer);
	if( body != NULL ) {
		sprintf(buffer, "%s\r\n", body);
		strcat(dest, buffer);
	}
}

void build_request_delete(char* dest, const char* url) {
	char buffer[MAX_LENGTH];

	sprintf(buffer, "DELETE %s HTTP/1.1\r\n", url);
	strcat(dest, buffer);
	sprintf(buffer, "Host: %s\r\n", str_ip);
	strcat(dest, buffer);
	sprintf(buffer, "\r\n");
	strcat(dest, buffer);
}

void pretty_print(char* title, char* content) {
	int length_response_title = strlen(title);
	int iter = (console_width - length_response_title) / 2;
	int additional = (console_width - length_response_title) % 2;
	for( int i = 0; i < iter + additional; i ++ ) {
		printf("=");
	}
	printf("%s", title);
	for( int i = 0; i < iter + additional; i ++ ) {
		printf("=");
	}
	printf("\n%s\n", content);
	for( int i = 0; i < console_width; i ++ ) {
		printf("=");
	}
}