#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pwd.h>
#include "data_structure.h"
#include "util.h"

#define PORT_FOR_COMMAND 8888
#define STR_EQUAL(p, q) strcmp(p, q) == 0


void fill_sockaddr_in(struct sockaddr_in*, const char*, int);
void work(int);
void handle_command_pwd(int);
void handle_command_cwd(int);
void handle_command_mkd(int);
void handle_command_rmd(int);
void handle_command_nlst(int);
void handle_command_list(int);
void handle_command_dele(int);
void handle_command_stor(int);
void handle_command_retr(int);
void handle_command_quit(int);


int is_quit;

int main(int argc, char* argv[]) {
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	printf("homedir : %s\n", homedir);


	int sock_server;
	if( (sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		ERROR_LOGGING("failed to create socket");
	}
	
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	fill_sockaddr_in(&server_addr, "127.0.0.1", PORT_FOR_COMMAND);
	
	if( connect(sock_server, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0 ) {
		ERROR_LOGGING("failed to call connect()");
	}
	
	while( !is_quit ) {
		work(sock_server);
	}
	
	shutdown(sock_server, SHUT_WR);
	clear_recv_buffer(sock_server);
	close(sock_server);

	return 0;
}


void fill_sockaddr_in(struct sockaddr_in* addr, const char* ip, int port) {
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(ip);
	addr->sin_port = htons(port);
}


void work(int sock) {
	char command[16];
	scanf("%s", command);

	printf("command : %s\n", command);

	if( STR_EQUAL(command, "PWD") ) {
		handle_command_pwd(sock);
	}  else if( STR_EQUAL(command, "CWD") ) {
		handle_command_cwd(sock);
	} else if( STR_EQUAL(command, "MKD") ) {
		handle_command_mkd(sock);
	} else if( STR_EQUAL(command, "RMD") ) {
		handle_command_rmd(sock);
	} else if( STR_EQUAL(command, "NLST") ) {
		handle_command_nlst(sock);
	} else if( STR_EQUAL(command, "LIST") ) {
		handle_command_list(sock);
	} else if( STR_EQUAL(command, "DELE") ) {
		handle_command_dele(sock);
	} else if( STR_EQUAL(command, "STOR") ) {
		handle_command_stor(sock);
	} else if( STR_EQUAL(command, "RETR") ) {
		handle_command_retr(sock);
	} else if( STR_EQUAL(command, "QUIT") ) {
		handle_command_quit(sock);
	}
}


void handle_command_pwd(int sock) {
	char message[MAX_LENGTH];
	sprintf(message, "PWD\r\n");
	write(sock, message, strlen(message));

	char response[MAX_LENGTH];
	read_line(sock, response, MAX_LENGTH);
	printf("response : %s", response);
}

void handle_command_cwd(int sock) {
	char path[MAX_LENGTH];
	scanf("%s", path);

	char message[MAX_LENGTH];
	sprintf(message, "CWD %s\r\n", path);
	write(sock, message, strlen(message));

	char response[MAX_LENGTH];
	read_line(sock, response, MAX_LENGTH);
	printf("response : %s", response);
}

void handle_command_mkd(int sock) {
	char dir_name[MAX_LENGTH];
	scanf("%s", dir_name);

	char message[MAX_LENGTH];
	sprintf(message, "MKD %s\r\n", dir_name);
	write(sock, message, strlen(message));

	char response[MAX_LENGTH];
	read_line(sock, response, MAX_LENGTH);
	printf("response : %s", response);
}

void handle_command_rmd(int sock) {
	char dir_name[MAX_LENGTH];
	scanf("%s", dir_name);

	char message[MAX_LENGTH];
	sprintf(message, "RMD %s\r\n", dir_name);
	write(sock, message, strlen(message));

	char response[MAX_LENGTH];
	read_line(sock, response, MAX_LENGTH);
	printf("response : %s", response);
}

void handle_command_nlst(int sock) {
	char message[MAX_LENGTH];
	sprintf(message, "NLST\r\n");
	write(sock, message, strlen(message));

	char response[MAX_LENGTH];
	read_line(sock, response, MAX_LENGTH);
	printf("response : %s", response);
}

void handle_command_list(int sock) {
	char message[MAX_LENGTH];
	sprintf(message, "LIST\r\n");
	write(sock, message, strlen(message));

	char response[MAX_LENGTH];
	read_line(sock, response, MAX_LENGTH);
	printf("response : %s", response);
}

void handle_command_dele(int sock) {
	char file_name[MAX_LENGTH];
	scanf("%s", file_name);

	char message[MAX_LENGTH];
	sprintf(message, "DELE %s\r\n", file_name);
	write(sock, message, strlen(message));

	char response[MAX_LENGTH];
	read_line(sock, response, MAX_LENGTH);
	printf("response : %s", response);
}

void handle_command_stor(int sock) {
	char file_name[MAX_LENGTH];
	scanf("%s", file_name);

	char local_path[MAX_LENGTH];
	scanf("%s", local_path);

	char message[MAX_LENGTH];
	sprintf(message, "STOR %s\r\n", file_name);
	write(sock, message, strlen(message));

	char response[MAX_LENGTH] = { 0, };
	read_line(sock, response, MAX_LENGTH);

	char *result = strtok(response, " \r\n");
	if( STR_EQUAL(result, "OK") ) {
		char *data_transfer_port = strtok(NULL, " \r\n");

		int sock_data_channel;
		if( (sock_data_channel = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			ERROR_LOGGING("failed to create socket for data transfer channel");
		}

		struct sockaddr_in server_addr_data_channel;
		memset(&server_addr_data_channel, 0, sizeof(server_addr_data_channel));
		fill_sockaddr_in(&server_addr_data_channel, "127.0.0.1", atoi(data_transfer_port));
		
		if( connect(sock_data_channel, (struct sockaddr*)&server_addr_data_channel, sizeof(server_addr_data_channel)) != 0 ) {
			ERROR_LOGGING("failed to call connect() for data transfer channel");
		}

		FILE *fp = fopen(local_path, "rb");
		if( !fp ) {
			ERROR_LOGGING("failed to open file that is target file of transfering");
		} else {
			char buffer[MAX_LENGTH];
			while( !feof(fp) ) {
				memset(buffer, 0, sizeof(buffer));
				int numRead = fread(buffer, sizeof(char), MAX_LENGTH, fp);
				write(sock_data_channel, buffer, numRead);
			}
			printf("complete sending\n");
		}

		fclose(fp);
		close(sock_data_channel);
	} else {
		ERROR_LOGGING("fail to connect data transfer channel")
	}
}

void handle_command_retr(int sock) {
	char file_name[MAX_LENGTH];
	scanf("%s", file_name);

	char message[MAX_LENGTH];
	sprintf(message, "RETR %s\r\n", file_name);
	write(sock, message, strlen(message));

	char response[MAX_LENGTH] = { 0, };
	read_line(sock, response, MAX_LENGTH);

	char *result = strtok(response, " \r\n");
	if( STR_EQUAL(result, "OK") ) {
		char *data_transfer_port = strtok(NULL, " \r\n");

		int sock_data_channel;
		if( (sock_data_channel = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			ERROR_LOGGING("failed to create socket for data transfer channel");
		}

		struct sockaddr_in server_addr_data_channel;
		memset(&server_addr_data_channel, 0, sizeof(server_addr_data_channel));
		fill_sockaddr_in(&server_addr_data_channel, "127.0.0.1", atoi(data_transfer_port));
		
		if( connect(sock_data_channel, (struct sockaddr*)&server_addr_data_channel, sizeof(server_addr_data_channel)) != 0 ) {
			ERROR_LOGGING("failed to call connect() for data transfer channel");
		}

		FILE *fp = fopen(file_name, "wb");
		if( !fp ) {
			ERROR_LOGGING("failed to open file that is target file of transfering");
		} else {
			char buffer[MAX_LENGTH];
			while(1) {
				memset(buffer, 0, sizeof(buffer));
				int numRead = read_line(sock_data_channel, buffer, MAX_LENGTH);
				if( numRead == 0 ) {
					break;
				}

				fwrite(buffer, sizeof(char), numRead, fp);
			}
			printf("complete receiving\n");
		}

		fclose(fp);
		close(sock_data_channel);
	} else {
		ERROR_LOGGING("fail to connect data transfer channel")
	}
}

void handle_command_quit(int sock) {
	char message[MAX_LENGTH];
	sprintf(message, "QUIT\r\n");
	write(sock, message, strlen(message));

	char response[MAX_LENGTH];
	read_line(sock, response, MAX_LENGTH);
	printf("response : %s", response);

	is_quit = 1;
}

