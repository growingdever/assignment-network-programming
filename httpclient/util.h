//
//  util.h
//  httpclient
//
//  Created by loki on 2015. 5. 14..
//  Copyright (c) 2015년 loki. All rights reserved.
//

#ifndef httpclient_util_h
#define httpclient_util_h

#include <stdio.h>

#define ERROR_LOGGING(content) { fprintf(stderr, "%d : %s\n", __LINE__, (content)); exit(1); }


void clear_recv_buffer(int sock_client) {
	char buffer[MAX_LENGTH];
	memset(buffer, 0, sizeof(buffer));
	read(sock_client, buffer, MAX_LENGTH);
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

ssize_t read_line(int fd, void *buffer, size_t n)
{
	ssize_t numRead;                    /* # of bytes fetched by last read() */
	size_t totRead;                     /* Total bytes read so far */
	char *buf;
	char ch;
	
	if (n <= 0 || buffer == NULL) {
		errno = EINVAL;
		return -1;
	}
	
	buf = buffer;                       /* No pointer arithmetic on "void *" */
	
	totRead = 0;
	for (;;) {
		numRead = read(fd, &ch, 1);
		if (numRead == -1) {
			if (errno == EINTR)         /* Interrupted --> restart read() */
				continue;
			else
				return -1;              /* Some other error */
		} else if (numRead == 0) {      /* EOF */
			if (totRead == 0)           /* No bytes read; return 0 */
				return 0;
			else                        /* Some bytes read; add '\0' */
				break;
		} else {                        /* 'numRead' must be 1 if we get here */
			if (totRead < n - 1) {      /* Discard > (n - 1) bytes */
				totRead++;
				*buf++ = ch;
			}
			if (ch == '\n')
				break;
		}
	}
	
	*buf = '\0';
	return totRead;
}

void str_tolower(char* str) {
	int length = (int)strlen(str);
	for( int i = 0; i < length; i ++ ) {
		if( str[i] >= 'A' && str[i] <= 'Z' ) {
			str[i] += 'a' - 'A';
		}
	}
}


#endif
