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

using namespace std;

class Client {
private:
    // creator constants
    char *m_name = "Srikanth Reddy Malipatel\0";
    char *m_ubitName = "smalipat\0";
    char *m_ubEmail = "smalipat@buffalo.edu\0";

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
    void command_list();
    void command_terminate();
    void command_quit();

    // Utility functions
    int getListenPort();
};

#endif /* !CLIENT_H */
