/*
 * ==========================================================================
 *      
 *      Filename: driver.cpp
 *
 *      Description: 
 * 
 *      Date: 2015-09-22 22:23
 *
 *      Author: Srikanth Malipatel
 *
 * ==========================================================================
 */
#include "server.h"
#include "client.h"

using namespace std;

void printUsage(int argc, char* argv[]) {
    printf("Usage: ./%s <program type> <port> \n", argv[0]);
    printf("<program type>: 's' for server and 'c' for client \n");
    printf("<port>: listening port of server/client \n");
}

int main(int argc, char* argv[]) {
    // check the parameters passed to this program
    if(argc != 3 || (strcmp("s", argv[1]) != 0) || (strcmp("c", argv[1]) != 0)) {
        printUsage(argc, argv);
        return 0;
    }
    // also do a valid port number check
    // check the program type and based on it initalize corresponding objects
    
    return 0;
}


