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

using namespace std;

class Client {
private:
    // creator constants
    char* m_name;
    char* m_ubitName[];
    char* m_ubEmail[];

    // client utilites
    int m_nListenPort;
public:
    // constructor and destructor
    Client(int port);
    ~Client();

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
};

#endif /* !CLIENT_H */
