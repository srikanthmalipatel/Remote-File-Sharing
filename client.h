/*
 * server.h
 * Copyright (C) 2015 SrikanthMalipatel <smalipat@buffalo.edu>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef CLIENT_H
#define CLIENT_H

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
#include "common.h"

using namespace std;

#define MAX_CREATOR_LEN 100

class Client {
private:
    // creator constants
    char* m_name;
    char* m_ubitName;
    char* m_ubEmail;

    // client utilites
    int m_nListenPort;
    char m_ipAddress[INET_ADDRSTRLEN];
public:
    // constructor and destructor
    Client(int port);
    ~Client();
    
    // eventHandler
    void commandShell();

    // These are possible commands supported by server
    void command_help();
    void command_creator();
    void command_display();
    int command_register();
    void command_list();
    void command_terminate(int connectionId);
    void command_quit();

    // Utility functions
    int getListenPort();

private:
    CommandID getCommandID(char comnd[]);
    void updateIpAddress();
};

#endif /* !CLIENT_H */
