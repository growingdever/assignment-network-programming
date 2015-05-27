#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include "data_structure.h"
#include "util.h"

#define PORT_FTP_COMMAND 8888
#define PORT_FTP_DATA_TRANSFER 8889
#define STR_EQUAL(p, q) strcmp(p, q) == 0


int init_listening_socket(struct sockaddr_in*, int);
int handle_socket(int);
void handle_command_pwd(int, char*);
void handle_command_cwd(int, char*);
void handle_command_mkd(int, char*);
void handle_command_rmd(int, char*);
void handle_command_nlst(int, char*);
void handle_command_list(int, char*);
void handle_command_dele(int, char*);
void handle_command_stor(int, char*);
void handle_command_retr(int, char*);
void handle_command_quit(int, char*);
void build_absolute_path(char* dest, char* target);
int is_file_for_dirent(struct dirent* dir);


char WORKING_DIRECTORY[MAX_LENGTH];

int sock_client, 
	sock_listen_command, 
	sock_listen_data;
int is_quit;

int main(int argc, char* argv[]) {
	// for command
	struct sockaddr_in servaddr_command;
	memset(&servaddr_command, 0, sizeof(servaddr_command));
	sock_listen_command = init_listening_socket(&servaddr_command, PORT_FTP_COMMAND);
	bind(sock_listen_command, (struct sockaddr *) &servaddr_command, sizeof(servaddr_command));
	listen(sock_listen_command, 10);

	// for data transfer
	struct sockaddr_in servaddr_data;
	memset(&servaddr_data, 0, sizeof(servaddr_data));
	sock_listen_data = init_listening_socket(&servaddr_data, PORT_FTP_DATA_TRANSFER);
	bind(sock_listen_data, (struct sockaddr *) &servaddr_data, sizeof(servaddr_data));
	listen(sock_listen_data, 10);

	sock_client = accept(sock_listen_command, (struct sockaddr*) NULL, NULL);
	if( sock_client < 0 ) {
		ERROR_LOGGING("failed to accept client socket")
		return -1;
	}

	getcwd(WORKING_DIRECTORY, sizeof(WORKING_DIRECTORY));

	while( !is_quit ) {
		if( handle_socket(sock_client) < 0 ) {
			ERROR_LOGGING("failed to handle request")
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
	printf("command : %s\n", command);
	if( STR_EQUAL(command, "PWD") ) {
		handle_command_pwd(sock, str);
	}  else if( STR_EQUAL(command, "CWD") ) {
		handle_command_cwd(sock, str);
	} else if( STR_EQUAL(command, "MKD") ) {
		handle_command_mkd(sock, str);
	} else if( STR_EQUAL(command, "RMD") ) {
		handle_command_rmd(sock, str);
	} else if( STR_EQUAL(command, "NLST") ) {
		handle_command_nlst(sock, str);
	} else if( STR_EQUAL(command, "LIST") ) {
		handle_command_list(sock, str);
	} else if( STR_EQUAL(command, "DELE") ) {
		handle_command_dele(sock, str);
	} else if( STR_EQUAL(command, "STOR") ) {
		handle_command_stor(sock, str);
	} else if( STR_EQUAL(command, "RETR") ) {
		handle_command_retr(sock, str);
	} else if( STR_EQUAL(command, "QUIT") ) {
		handle_command_quit(sock, str);
	}

	return 1;
}

void handle_command_pwd(int sock, char* line) {
	char path[MAX_LENGTH] = { 0, };
	getcwd(path, sizeof(path));

	write(sock, path, strlen(path));
}

void handle_command_cwd(int sock, char* line) {
	char response[MAX_LENGTH] = { 0, };

	char *target = strtok(NULL, " \r\n");
	if( chdir(target) == 0 ) {
		sprintf(response, "success\r\n");
		strcpy(WORKING_DIRECTORY, target);
	} else {
		sprintf(response, "fail\r\n");
	}

	write(sock, response, strlen(response));
}

void handle_command_mkd(int sock, char* line) {
	char path[MAX_LENGTH] = { 0, };
	build_absolute_path(path, strtok(NULL, " \r\n"));

	char response[MAX_LENGTH] = { 0, };
	if( mkdir(path, 0755) == 0 ) {
		sprintf(response, "success\r\n");
		write(sock, response, strlen(response));
	} else {
		switch(errno) {
		case EACCES:
			sprintf(response, "fail : permission denied\r\n");
			break;
		case EEXIST:
			sprintf(response, "fail : file exist\r\n");
			break;
		}

		write(sock, response, strlen(response));
	}
}

void handle_command_rmd(int sock, char* line) {
	char *path = strtok(NULL, " \r\n");

	char response[MAX_LENGTH] = { 0, };
	if( rmdir(path) == 0 ) {
		sprintf(response, "success\r\n");
	} else {
		switch(errno) {
		case ENOTDIR:
			sprintf(response, "fail : target file is not directory\r\n");
			break;
		case EEXIST:
			sprintf(response, "fail : directory exist\r\n");
			break;
		case ENOTEMPTY:
			sprintf(response, "fail : directory is not empty\r\n");
			break;
		}
	}

	write(sock, response, strlen(response));
}

void handle_command_nlst(int sock, char* line) {
	char response[MAX_LENGTH] = { 0, };

	DIR *p_root_dir;
	struct dirent *p_dir_node;
	p_root_dir = opendir(WORKING_DIRECTORY);
	if (p_root_dir != NULL)	{
		while (1) {
			p_dir_node = readdir (p_root_dir);
			if( p_dir_node == NULL ) { 
				break;
			}

			if( is_file_for_dirent(p_dir_node) ) {
				continue;
			}

			strcat(response, p_dir_node->d_name);
			strcat(response, ", ");
		}
		closedir (p_root_dir);

		response[strlen(response) - 2] = 0;
		strcat(response, "\r\n");
	} else {
		strcat(response, "fail\r\n");
	}

	write(sock, response, strlen(response));
}

void handle_command_list(int sock, char* line) {
	char response[MAX_LENGTH] = { 0, };

	DIR *p_root_dir;
	struct dirent *p_dir_node;
	p_root_dir = opendir(WORKING_DIRECTORY);
	if (p_root_dir != NULL)	{
		while (1) {
			p_dir_node = readdir (p_root_dir);
			if( p_dir_node == NULL ) { 
				break;
			}

			if( ! is_file_for_dirent(p_dir_node) ) {
				continue;
			}

			strcat(response, p_dir_node->d_name);
			strcat(response, ", ");
		}
		closedir (p_root_dir);

		response[strlen(response) - 2] = 0;
		strcat(response, "\r\n");
	} else {
		strcat(response, "fail\r\n");
	}

	write(sock, response, strlen(response));
}

void handle_command_dele(int sock, char* line) {
	char path[MAX_LENGTH] = { 0, };
	build_absolute_path(path, strtok(NULL, " \r\n"));

	char response[MAX_LENGTH];
	if( remove(path) == -1 ) {
		sprintf(response, "fail\r\n");
	} else {
		sprintf(response, "success\r\n");
	}

	write(sock, response, strlen(response));
}

void handle_command_stor(int sock, char* line) {
	char *target = strtok(NULL, " \r\n");

	char response[MAX_LENGTH] = { 0, };
	sprintf(response, "OK %d", PORT_FTP_DATA_TRANSFER);
	write(sock, response, strlen(response));

	int sock_data_channel = accept(sock_listen_data, (struct sockaddr*) NULL, NULL);
	if( sock_data_channel < 0 ) {
		ERROR_LOGGING("failed to accept client socket")
		return;
	}


	char path[MAX_LENGTH] = { 0, };
	sprintf(path, "%s/%s", WORKING_DIRECTORY, target);
	FILE *output = fopen(path, "wb");
	if( ! output ) {
		sprintf(response, "fail to open file");
		write(sock, response, strlen(response));
	} else {
		char buffer[MAX_LENGTH] = { 0, };
		while(1) {
			memset(buffer, 0, sizeof(buffer));
			int numRead = read_line(sock_data_channel, buffer, MAX_LENGTH);
			if( numRead == 0 ) {
				break;
			}

			fwrite(buffer, sizeof(char), numRead, output);
		}
		
	}

	fclose(output);
	close(sock_data_channel);
}

void handle_command_retr(int sock, char* line) {
	char *target = strtok(NULL, " \r\n");

	char response[MAX_LENGTH] = { 0, };
	sprintf(response, "OK %d", PORT_FTP_DATA_TRANSFER);
	write(sock, response, strlen(response));

	int sock_data_channel = accept(sock_listen_data, (struct sockaddr*) NULL, NULL);
	if( sock_data_channel < 0 ) {
		ERROR_LOGGING("failed to accept client socket")
		return;
	}


	char path[MAX_LENGTH] = { 0, };
	sprintf(path, "%s/%s", WORKING_DIRECTORY, target);
	FILE *input = fopen(path, "rb");
	if( ! input ) {
		sprintf(response, "fail to open file");
		write(sock, response, strlen(response));
	} else {
		char buffer[MAX_LENGTH] = { 0, };
		while( !feof(input) ) {
			memset(buffer, 0, sizeof(buffer));
			int numRead = fread(buffer, sizeof(char), MAX_LENGTH, input);
			write(sock_data_channel, buffer, numRead);
		}
	}

	close(sock_data_channel);
	fclose(input);
}

void handle_command_quit(int sock, char* line) {
	is_quit = 1;

	char response[MAX_LENGTH] = { 0, };
	sprintf(response, "bye");
	write(sock, response, strlen(response));

	shutdown(sock_client, SHUT_WR);
	clear_recv_buffer(sock_client);
	close(sock_client);
}


void build_absolute_path(char* dest, char* target) {
	strcat(dest, WORKING_DIRECTORY);
	strcat(dest, "/");
	strcat(dest, target);
}

int is_file_for_dirent(struct dirent* dir) {
	return dir->d_type == 0x8;
}