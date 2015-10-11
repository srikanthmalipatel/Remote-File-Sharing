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

using namespace std;

// This contains all common definitions for client and server

// macros for constants
#define MAX_CREATOR_LEN 100
#define STDIN 0

// all possible commands. update this when adding a new command
typedef enum {
    COMMAND_HELP=1,
    COMMAND_CREATOR,
    COMMAND_DISPLAY,
    COMMAND_LIST,
	COMMAND_REGISTER,
    COMMAND_NONE=-1,
}CommandID;

typedef struct {
	int sockFd;
	bool isServer;
	bool isUsed;
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
	void getHostName(char *ip, char *buf);
};

#endif /* !COMMON_H */
