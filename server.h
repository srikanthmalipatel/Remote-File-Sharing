/*
 * server.h
 * Copyright (C) 2015 SrikanthMalipatel <smalipat@buffalo.edu>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef SERVER_H
#define SERVER_H

#include <cstdio>
#include <iostream>
#include <cstdlib>

using namespace std;

class Server {
private:
    // creator constants
    char *m_name = "Srikanth Reddy Malipatel\0";
    char *m_ubitName = "smalipat\0";
    char *m_ubEmail = "smalipat@buffalo.edu\0";

    // server utilites
    int m_nListenPort;
public:
    // constructor and destructor
    Server(int port);
    ~Server();

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

#endif /* !SERVER_H */
