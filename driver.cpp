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


int main(int argc, char* argv[]) {
    if(argv != 3) {
        printf("Usage: ./%s <program type> <port> \n", argv[0]);
        printf("<program type>: 's' for server and 'c' for client \n");
        printf("<port>: listening port of server/client \n");
        return 0;
    }
    
    return 0;
}


