#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define MAX_LENGTH 1024
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


void print_error(const char* content);
char* tokenizing_multi_character_delim(char* dst, char* src, char* delim);


void random_string(char *dest, int length) {
	static const char alphanum[] =
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	
	for (int i = 0; i < length; ++i) {
		dest[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	
	dest[length] = 0;
}

int is_directory(const struct stat stat_buffer) {
	return (stat_buffer.st_mode & S_IFMT) == S_IFDIR;
}

int is_file(const struct stat stat_buffer) {
	return (stat_buffer.st_mode & S_IFMT) == S_IFREG;
}

void find_header_value(const struct http_request* request, const char* key, char* dest) {
	for( int i = 0; i < request->header_count; i ++ ) {
		char header_key_only[32];
		tokenizing_multi_character_delim(header_key_only, (char*)request->headers[i], ": ");
		if( strcmp(header_key_only, key) == 0 ) {
			tokenizing_multi_character_delim(dest, (char*)request->headers[i] + strlen(header_key_only) + 2, "\r\n");
			return;
		}
	}
}

int print_list_of_files(const char* path, char* response) {
	char process_str[MAX_LENGTH];
	sprintf(process_str, "ls -al %s", path);
	printf("%s\n", process_str);
	
	char buffer[MAX_LENGTH];
	FILE *process_fp = popen(process_str, "r");
	if (process_fp == NULL)	{
		return -1;
	}
	
	strcat(response, "HTTP/1.0 200 OK\r\n");
	strcat(response, "Server: myserver\r\n");
	strcat(response, "Content-Type: application/json\r\n");
	strcat(response, "\r\n[");
	// remove first line
	fgets(buffer, MAX_LENGTH, process_fp);
	while(fgets(buffer, MAX_LENGTH, process_fp) != NULL) {
		char *str_permission = strtok(buffer, " ");
		char *str_link = strtok(NULL, " ");
		char *str_owner = strtok(NULL, " ");
		char *str_group = strtok(NULL, " ");
		char *str_size = strtok(NULL, " ");
		char *str_time = strtok(NULL, " ");
		char *str_name = strtok(NULL, " ");
		
		char tmp[MAX_LENGTH];
		sprintf(tmp, "{ \
				\"%s\" : \"%s\" \
				\"%s\" : \"%s\" \
				\"%s\" : \"%s\" \
				\"%s\" : \"%s\" \
				\"%s\" : \"%s\" \
				\"%s\" : \"%s\" \
				\"%s\" : \"%s\" \
				},",
				"permission", str_permission,
				"link", str_link,
				"owner", str_owner,
				"group", str_group,
				"size", str_size,
				"time", str_time,
				"name", str_name);
		strcat(response, tmp);
	}
	// remove last comma
	response[strlen(response) - 1] = 0;
	strcat(response, "]");
	
	pclose(process_fp);
	
	return 1;
}

int print_content_of_file(const char* path, char* response) {
	printf("%s\n", path);
	FILE *fp = fopen(path, "r");
	if( !fp ) {
		strcat(response, "HTTP/1.0 404 Not Found\r\n");
		strcat(response, "Server: myserver\r\n");
		strcat(response, "Content-Type: text/plain; charset=utf-8\r\n");
		strcat(response, "\r\n");
		fclose(fp);
		return -1;
	}
	
	strcat(response, "HTTP/1.0 200 OK\r\n");
	strcat(response, "Server: myserver\r\n");
	strcat(response, "Content-Type: text/plain; charset=utf-8\r\n");
	strcat(response, "\r\n");
	
	char buffer[MAX_LENGTH];
	while(fgets(buffer, MAX_LENGTH, fp) != NULL) {
		strcat(response, buffer);
	}
	
	fclose(fp);
	
	return 1;
}

int process_request_get(const struct http_request* request, char* response) {
	char path[32];
	sprintf(path, ".%s", request->url);
	
	struct stat stat_buffer;
	if( stat(path, &stat_buffer) == -1 ) {
		return -1;
	}
	
	if( (stat_buffer.st_mode & S_IFMT) == S_IFDIR ) {
		return print_list_of_files(path, response);
	} else if( (stat_buffer.st_mode & S_IFMT) == S_IFREG ) {
		return print_content_of_file(path, response);
	}
	
	
	strcat(response, "HTTP/1.0 400 Bad Request\r\n");
	strcat(response, "Server: myserver\r\n");
	strcat(response, "Content-Type: text/plain; charset=utf-8\r\n");
	strcat(response, "\r\n");
	
	return -1;
}

int process_request_post(const struct http_request* request, char* response) {
	char path[64];
	sprintf(path, ".%s", request->url);
	
	struct stat stat_buffer;
	if( stat(path, &stat_buffer) == -1 ) {
		strcat(response, "HTTP/1.0 404 Not Found\r\n");
		strcat(response, "Server: myserver\r\n");
		strcat(response, "Content-Type: text/plain; charset=utf-8\r\n");
		strcat(response, "\r\n");
		return -1;
	}
	
	if( is_directory(stat_buffer) ) {
		char new_filename[16];
		random_string(new_filename, 8);
		
		sprintf(path, ".%s/%s", request->url, new_filename);
		
		FILE *new_fp = fopen(path, "w");
		fprintf(new_fp, "%s", request->body);
		fclose(new_fp);
		
		char host_url[MAX_LENGTH];
		find_header_value(request, "Host", host_url);
		
		char location_part[MAX_LENGTH];
		sprintf(location_part, "Location: %s%s\r\n", host_url, path + 1);
		
		strcat(response, "HTTP/1.0 201 Created\r\n");
		strcat(response, "Server: myserver\r\n");
		strcat(response, "Content-Type: text/plain; charset=utf-8\r\n");
		strcat(response, location_part);
		strcat(response, "\r\n");
		strcat(response, request->body);
	} else if( is_file(stat_buffer) ) {
		FILE *fp = fopen(path, "a");
		fprintf(fp, "%s", request->body);
		fclose(fp);

		strcat(response, "HTTP/1.1 200 OK\r\n");
		strcat(response, "Server: myserver\r\n\r\n");
	}
	
	return 1;
}

int process_request_put(const struct http_request* request, char* response) {
	char path[64];
	sprintf(path, ".%s", request->url);

	struct stat stat_buffer;
	if( stat(path, &stat_buffer) == -1 && strlen(request->body) == 0 ) {
		// create new directory
		char process_str[MAX_LENGTH];
		sprintf(process_str, "mkdir -p %s", path);
		printf("%s\n", process_str);
		
		char buffer[MAX_LENGTH];
		FILE *process_fp = popen(process_str, "r");
		if (process_fp == NULL)	{
			return -1;
		}

		strcat(response, "HTTP/1.0 201 Created\r\n");
		strcat(response, "Server: myserver\r\n");
		strcat(response, "Content-Type: text/plain; charset=utf-8\r\n");
		strcat(response, "\r\n");
		return 1;
	}

	if( is_file(stat_buffer) ) {
		FILE *fp = fopen(path, "w");
		fprintf(fp, "%s", request->body);
		fclose(fp);

		strcat(response, "HTTP/1.1 200 OK\r\n");
		strcat(response, "Server: myserver\r\n");
		strcat(response, "Content-Type: text/plain; charset=utf-8\r\n\r\n");
		strcat(response, request->body);
		strcat(response, "\r\n");
		return 1;
	}

	strcat(response, "HTTP/1.0 400 Bad Request\r\n");
	strcat(response, "Server: myserver\r\n");
	strcat(response, "Content-Type: text/plain; charset=utf-8\r\n");
	strcat(response, "\r\n");
	return -1;
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

	strcat(response, "HTTP/1.0 400 Bad Request\r\n");
	strcat(response, "Server: myserver\r\n");
	strcat(response, "Content-Type: text/plain; charset=utf-8\r\n");
	strcat(response, "\r\n");
	return -1;
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
	last = header_part;
	while(1) {
		last = tokenizing_multi_character_delim(request->headers[request->header_count], last, "\r\n");
		request->header_count++;
		if( last == NULL ) {
			break;
		}
	}
}

int read_all_data(int sock_client, char* buffer) {
	ssize_t len = recv(sock_client, buffer, MAX_LENGTH, 0);
	return (int)len;
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
		memset(&request, 0, sizeof(request));
		parsing_http_request(&request, str);
		
		char response[MAX_LENGTH] = { 0, };
		if( process_request(&request, response) < 0 ) {
			printf("failed to process request\n");
			write(sock_client, response, strlen(response));
		} else {
			write(sock_client, response, strlen(response));
		}
		
		shutdown(sock_client, SHUT_WR);
		clear_recv_buffer(sock_client);
		close(sock_client);
	}
	
	return 0;
}