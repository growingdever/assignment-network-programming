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
#define STR_EQUAL(p, q) strcmp(p, q) == 0


int init_listening_socket(struct sockaddr_in*, int);
int handle_socket(int);
void handle_command_pwd(int, char*);
void handle_command_cwd(int, char*);
void handle_command_mkd(int, char*);
void handle_command_nlst(int, char*);
int is_file_for_dirent(struct dirent* dir);


char WORKING_DIRECTORY[MAX_LENGTH];


int main(int argc, char* argv[]) {
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	int sock_listen = init_listening_socket(&servaddr, PORT_FTP_COMMAND);

	bind(sock_listen, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(sock_listen, 10);

	int sock_client = accept(sock_listen, (struct sockaddr*) NULL, NULL);
	if( sock_client < 0 ) {
		ERROR_LOGGING("failed to accept client socket")
		return -1;
	}

	getcwd(WORKING_DIRECTORY, sizeof(WORKING_DIRECTORY));

	while(1) {
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
	} else if(STR_EQUAL(command, "MKD") ) {
		handle_command_mkd(sock, str);
	} else if(STR_EQUAL(command, "NLST") ) {
		handle_command_nlst(sock, str);
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
		sprintf(response, "file\r\n");
	}

	write(sock, response, strlen(response));
}

void handle_command_mkd(int sock, char* line) {
	char *target = strtok(NULL, " \r\n");
	char path[MAX_LENGTH] = { 0, };
	strcat(path, WORKING_DIRECTORY);
	strcat(path, "/.");
	strcat(path, target);

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

int is_file_for_dirent(struct dirent* dir) {
	return dir->d_type == 0x8;
}