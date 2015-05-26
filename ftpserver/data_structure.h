//
//  data_structure.h
//  httpclient
//
//  Created by loki on 2015. 5. 14..
//  Copyright (c) 2015년 loki. All rights reserved.
//

#ifndef httpclient_data_structure_h
#define httpclient_data_structure_h

#define MAX_LENGTH 4096
#define MAX_LENGTH_VERSION 16
#define MAX_LENGTH_STATUS_MESSAGE 32
#define MAX_NUM_OF_HEADER 32
#define MAX_LENGTH_HEADER 128
#define MAX_LENGTH_METHOD 16
#define MAX_LENGTH_URL 128


typedef struct http_request {
	int sock;
	char method[MAX_LENGTH_METHOD];
	char url[MAX_LENGTH_URL];
	char version[MAX_LENGTH_VERSION];
	int header_count;
	char headers[MAX_NUM_OF_HEADER][MAX_LENGTH_HEADER];
	char body[MAX_LENGTH];
} http_request;

typedef struct http_response {
	char version[MAX_LENGTH_VERSION];
	int status_code;
	char status_message[MAX_LENGTH_STATUS_MESSAGE];
	int num_of_header;
	char headers[MAX_NUM_OF_HEADER][MAX_LENGTH_HEADER];
	char body[MAX_LENGTH];
} http_response;

#endif
