/*
 * common.h
 * Copyright (C) 2015 SrikanthMalipatel <SrikanthMalipatel@Srikanth>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef COMMON_H
#define COMMON_H

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
// if common functionality is moved to different file then move these headers also
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

using namespace std;

// This contains all common definitions for client and server

// macros for constants
#define MAX_CREATOR_LEN 100
#define STDIN 0
#define BYTES512 512

// all possible commands. update this when adding a new command
typedef enum {
    COMMAND_HELP=1,
    COMMAND_CREATOR,
    COMMAND_DISPLAY,
    COMMAND_LIST,
	COMMAND_REGISTER,
	COMMAND_TERMINATE,
	COMMAND_EXIT,
	COMMAND_CONNECT,
	COMMAND_PUT,
	COMMAND_GET,
	COMMAND_SYNC,
    COMMAND_NONE=-1,
}CommandID;

typedef enum {
	PROGRESS,
	ACTIVE,
	INACTIVE,
}connectionState;

// this is used by server to maintain registered client list, where as client uses it to maintain active connections
typedef struct {
	int id;
	int sockFd;
	bool isServer;
	connectionState state;
	char hostName[32];
	char ip_addr[32];
	int listenPort;
}nodes;

class Base {
public:
	nodes m_nodeList[10];
	int m_nLatestIndex;

	void addNodetoList(int sockfd, char *ipAddr, int port);
	int sendall(int sockFd, char *buf, int *length);
	bool getHostName(char *ip, char *buf, bool printErr=true);
	bool getIPaddress(char *host, char *buf, bool printErr=true);
};

#endif /* !COMMON_H */
