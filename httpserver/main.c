#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define SERVER_STRING "Server: httpserver/0.0.1\r\n"

#define MAX_LENGTH 1024
#define MAX_LENGTH_METHOD 16
#define MAX_LENGTH_URL 128
#define MAX_LENGTH_VERSION 128
#define MAX_NUM_OF_HEADER 32
#define MAX_LENGTH_HEADER 128
#define PORT 8888

#define ERROR_LOGGING(content) { fprintf(stderr, "%d : %s\n", __LINE__, (content)); exit(1); }


typedef struct http_request {
	int sock;
	char method[MAX_LENGTH_METHOD];
	char url[MAX_LENGTH_URL];
	char version[MAX_LENGTH_VERSION];
	int header_count;
	char headers[MAX_NUM_OF_HEADER][MAX_LENGTH_HEADER];
	char body[MAX_LENGTH];
} http_request;


char* tokenizing_multi_character_delim(char* dst, char* src, char* delim);
int get_list_of_files(const char* path, char* content);
int get_content_of_file(const char* path, char* content);
int handle_request(int sock);
int init_listening_socket(struct sockaddr_in* addr);
void response_200(int sock, 
	const char* extra_header, 
	const char* content);
void response_200_json(int sock, 
	const char* extra_header, 
	const char* content);
void response_201(int sock, 
	const char* extra_header, 
	const char* content);
void response_400(int sock);
void response_404(int sock);


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
		char header_key_only[MAX_LENGTH_HEADER];
		tokenizing_multi_character_delim(header_key_only, (char*)request->headers[i], ": ");
		if( strcmp(header_key_only, key) == 0 ) {
			tokenizing_multi_character_delim(dest, 
				(char*)request->headers[i] + strlen(header_key_only) + 2, 
				"\r\n");
			return;
		}
	}
}

int get_list_of_files(const char* path, char* content) {
	char process_str[MAX_LENGTH];
	sprintf(process_str, "ls -al %s", path);
	printf("%s\n", process_str);
	
	char buffer[MAX_LENGTH];
	FILE *process_fp = popen(process_str, "r");
	if (process_fp == NULL)	{
		return -1;
	}
	
	strcat(content, "[");
	{
		fgets(buffer, MAX_LENGTH, process_fp);
		while(fgets(buffer, MAX_LENGTH, process_fp) != NULL) {
			char *str_permission = strtok(buffer, " ");
			char *str_link = strtok(NULL, " ");
			char *str_owner = strtok(NULL, " ");
			char *str_group = strtok(NULL, " ");
			char *str_size = strtok(NULL, " ");
			char *str_time = strtok(NULL, " ");
			strtok(NULL, " ");
			strtok(NULL, " ");
			char *str_name = strtok(NULL, " \n");
			
			char tmp[MAX_LENGTH];
			sprintf(tmp, "{ \
					\"%s\" : \"%s\" \
					\"%s\" : \"%s\" \
					\"%s\" : \"%s\" \
					\"%s\" : \"%s\" \
					\"%s\" : \"%s\" \
					\"%s\" : \"%s\" \
					\"%s\" : \"%s\" },",
					"permission", str_permission,
					"link", str_link,
					"owner", str_owner,
					"group", str_group,
					"size", str_size,
					"time", str_time,
					"name", str_name);
			strcat(content, tmp);
		}
	}
	strcat(content, "]");
	pclose(process_fp);
	
	return 1;
}

int get_content_of_file(const char* path, char* content) {
	printf("%s\n", path);
	FILE *fp = fopen(path, "r");
	if( !fp ) {
		fclose(fp);
		return -1;
	}

	char buffer[MAX_LENGTH];
	while(fgets(buffer, MAX_LENGTH, fp) != NULL) {
		strcat(content, buffer);
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
	
	char content[MAX_LENGTH] = { 0, };
	if( (stat_buffer.st_mode & S_IFMT) == S_IFDIR ) {
		if( get_list_of_files(path, content) < 0 ) {
			response_404(request->sock);
			return -1;
		}

		response_200_json(request->sock, "", content);
		return 1;
	} else if( (stat_buffer.st_mode & S_IFMT) == S_IFREG ) {
		if( get_content_of_file(path, content) < 0 ) {
			response_404(request->sock);
			return -1;
		}

		response_200(request->sock, "", content);
		return 1;
	}
	
	
	response_400(request->sock);
	return -1;
}

int process_request_post(const struct http_request* request, char* response) {
	char path[64];
	sprintf(path, ".%s", request->url);
	
	struct stat stat_buffer;
	if( stat(path, &stat_buffer) == -1 ) {
		response_404(request->sock);
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

		response_201(request->sock, location_part, request->body);
		return 1;		
	} else if( is_file(stat_buffer) ) {
		FILE *fp = fopen(path, "a");
		fprintf(fp, "%s", request->body);
		fclose(fp);
		
		response_200(request->sock, "", request->body);
		return 1;
	}
	
	response_400(request->sock);
	return -1;
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
		pclose(process_fp);
		
		response_201(request->sock, "", "");
		return 1;
	}
	
	if( is_file(stat_buffer) ) {
		FILE *fp = fopen(path, "w");
		fprintf(fp, "%s", request->body);
		fclose(fp);

		response_200(request->sock, "", request->body);
		return 1;
	}
	
	response_400(request->sock);
	return -1;
}

int process_request_delete(const struct http_request* request, char* response) {
	char path[64];
	sprintf(path, ".%s", request->url);
	
	struct stat stat_buffer;
	if( stat(path, &stat_buffer) == -1 ) {
		response_404(request->sock);
		return -1;
	}
	
	if( ! is_file(stat_buffer) ) {
		response_400(request->sock);
		return -1;
	}
	
	char process_str[MAX_LENGTH];
	sprintf(process_str, "rm -f %s", path);
	printf("%s\n", process_str);
	
	char buffer[MAX_LENGTH];
	FILE *process_fp = popen(process_str, "r");
	if (process_fp == NULL)	{
		return -1;
	}
	pclose(process_fp);
	
	response_200(request->sock, "", "");
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
	
	response_400(request->sock);
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
	char header_part[MAX_LENGTH_HEADER] = { 0, };
	
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

int handle_request(int sock) {
	char str[MAX_LENGTH] = { 0, };
	if( ! read_all_data(sock, str) ) {
		return -1;
	}
	
	http_request request;
	memset(&request, 0, sizeof(request));

	request.sock = sock;
	parsing_http_request(&request, str);
	
	char response[MAX_LENGTH] = { 0, };
	if( process_request(&request, response) < 0 ) {
		fprintf(stderr, "failed to process request\n");
	}

	write(sock, response, strlen(response));
	shutdown(sock, SHUT_WR);
	clear_recv_buffer(sock);
	close(sock);

	return 1;
}

int init_listening_socket(struct sockaddr_in* addr) {
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htons(INADDR_ANY);
	addr->sin_port = htons(PORT);

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

int main() {
	printf("start\n");
	
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	int sock_listen = init_listening_socket(&servaddr);

	bind(sock_listen, (struct sockaddr *) &servaddr, sizeof(servaddr));
	
	listen(sock_listen, 10);
	printf("start listen\n");
	
	while(1) {
		int sock_client = accept(sock_listen, (struct sockaddr*) NULL, NULL);
		if( sock_client < 0 ) {
			fprintf(stderr, "failed to accept client socket\n");
			continue;
		}

		if( handle_request(sock_client) < 0 ) {
			fprintf(stderr, "failed to handle request\n");
			continue;
		}
	}
	
	return 0;
}

void response_200(int sock, const char* extra_header, const char* content) {
	char response[MAX_LENGTH];
	sprintf(response, "HTTP/1.1 200 OK\r\n");
	send(sock, response, strlen(response), 0);
	sprintf(response, SERVER_STRING);
	send(sock, response, strlen(response), 0);
	sprintf(response, "Content-Type: text/plain; charset=utf-8\r\n");
	send(sock, response, strlen(response), 0);
	if( extra_header != NULL && strlen(extra_header) > 0 ) {
		sprintf(response, "%s", extra_header);
		send(sock, response, strlen(response), 0);
	}
	sprintf(response, "\r\n");
	send(sock, response, strlen(response), 0);
	if( content != NULL && strlen(content) > 0 ) {
		sprintf(response, "%s", content);
		send(sock, response, strlen(response), 0);
	}
}

void response_200_json(int sock, const char* extra_header, const char* content) {
	char response[MAX_LENGTH];
	sprintf(response, "HTTP/1.1 200 OK\r\n");
	send(sock, response, strlen(response), 0);
	sprintf(response, SERVER_STRING);
	send(sock, response, strlen(response), 0);
	sprintf(response, "Content-Type: %s\r\n", "application/json");
	send(sock, response, strlen(response), 0);
	if( extra_header != NULL && strlen(extra_header) > 0 ) {
		sprintf(response, "%s", extra_header);
		send(sock, response, strlen(response), 0);
	}
	sprintf(response, "\r\n");
	send(sock, response, strlen(response), 0);
	if( content != NULL && strlen(content) > 0 ) {
		sprintf(response, "%s", content);
		send(sock, response, strlen(response), 0);
	}
}

void response_201(int sock, const char* extra_header, const char* content) {
	char response[MAX_LENGTH];
	sprintf(response, "HTTP/1.1 201 Created\r\n");
	send(sock, response, strlen(response), 0);
	sprintf(response, SERVER_STRING);
	send(sock, response, strlen(response), 0);
	sprintf(response, "Content-Type: text/plain; charset=utf-8\r\n");
	send(sock, response, strlen(response), 0);
	if( extra_header != NULL && strlen(extra_header) > 0 ) {
		sprintf(response, "%s", extra_header);
		send(sock, response, strlen(response), 0);
	}
	sprintf(response, "\r\n");
	send(sock, response, strlen(response), 0);
	if( content != NULL && strlen(content) > 0 ) {
		sprintf(response, "%s", content);
		send(sock, response, strlen(response), 0);
	}
}

void response_400(int sock) {
	char response[MAX_LENGTH];
	sprintf(response, "HTTP/1.1 400 Bad Request\r\n");
	send(sock, response, strlen(response), 0);
	sprintf(response, SERVER_STRING);
	send(sock, response, strlen(response), 0);
	sprintf(response, "Content-Type: text/plain; charset=utf-8\r\n");
	send(sock, response, strlen(response), 0);
	sprintf(response, "\r\n");
	send(sock, response, strlen(response), 0);
}

void response_404(int sock) {
	char response[MAX_LENGTH];
	sprintf(response, "HTTP/1.1 404 Not Found\r\n");
	send(sock, response, strlen(response), 0);
	sprintf(response, SERVER_STRING);
	send(sock, response, strlen(response), 0);
	sprintf(response, "Content-Type: text/plain; charset=utf-8\r\n");
	send(sock, response, strlen(response), 0);
	sprintf(response, "\r\n");
	send(sock, response, strlen(response), 0);
}