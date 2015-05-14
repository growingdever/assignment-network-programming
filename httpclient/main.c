#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include "data_structure.h"
#include "util.h"

#define ARGV_INDEX_IP 1
#define ARGV_INDEX_PORT 2


void work(int sock);
void build_request_get(char* url);


const char* str_ip;
const char* str_port;


int main(int argc, char const *argv[])
{
	if( argc <= 1 ) {
		ERROR_LOGGING("usage:\narg1 : ip, arg2 : port");
	}
	
	str_ip = argv[ARGV_INDEX_IP];
	str_port = argv[ARGV_INDEX_PORT];
	
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
	char command[16] = { 'g', 'e', 't', };
	printf("command : ");
	scanf("%s", command);
	str_tolower(command);
	
	if( strcmp(command, "get") == 0 ) {
		sprintf(buffer, "GET /bin HTTP/1.1\r\nHost: %s:%s\r\n\r\n", str_ip, str_port);
		write(sock, buffer, strlen(buffer));
		
		http_response response;
		memset(&response, 0, sizeof(response));
		
		read_response_by_method(sock, &response);
		printf("%s\n", response.body);
	}
}

void read_response_by_method(int sock, http_response* response) {
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