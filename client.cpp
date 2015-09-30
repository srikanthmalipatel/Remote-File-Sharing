/*
 * ==========================================================================
 *      
 *      Filename: client.cpp
 *
 *      Description: 
 * 
 *      Date: 2015-09-22 22:37
 *
 *      Author: Srikanth Malipatel
 *
 * ==========================================================================
 */

#include "client.h"

/*
 * Function:    Constructor
 */
Client::Client(int port) {
    printf("Starting client on Port: %d \n", port);
    this->m_nListenPort = port;
    updateIpAddress();
}

/*
 * Function:    Destructor
 */
Client::~Client() {

}

/*
 * Function:    getCommandID(char[] comnd)
 * Parameters:  comnd: command to be executed
 * Returns:     if string is found then returns ComandID else -1
 * Description: This functions takes a string and returns appropriate CommandID
 */
CommandID Client::getCommandID(char comnd[]) {
    if(strcasecmp(comnd, "HELP") == 0)
        return COMMAND_HELP;
    else if(strcasecmp(comnd, "CREATOR") == 0)
        return COMMAND_CREATOR;
    else if(strcasecmp(comnd, "DISPLAY") == 0)
        return COMMAND_DISPLAY;
    else if(strcasecmp(comnd, "LIST") == 0)
        return COMMAND_LIST;
    else
        return COMMAND_NONE;
}

/*
 * Function:    commandHandler()
 * Parameters:  None
 * Returns:     None
 * Description: This functions behaves like shell answering all user commands
 */
void Client::commandShell() {
    while(1) {
        char command[20];
        scanf("%s", command);
        CommandID command_id = getCommandID(command); 
        switch(command_id) {
            case COMMAND_HELP:
                // handle help command
                command_help();
                break;
            case COMMAND_CREATOR:
                // handle CREATOR command
                command_creator();
                break;
            case COMMAND_DISPLAY:
                // hadle DISPLAY command
                command_display();
                break;
            case COMMAND_LIST:
                command_list();
                break;
            default:
                printf("Please enter a valid command \n");
                printf("Type Help - to display supported commands \n");
                break;
        }
    }
}

/*
 * Function:    Command_help()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the commands avaliable to user
 */
void Client::command_help() {
    printf("User command options \n");
    printf("\tCREATOR - Displays creators full name, UBIT name and UB email address\n");
    printf("\tDIPSLAY - Displays the IP address and listening port of this process\n");
    printf("\tLIST - Displays a list of clients registered on this server\n");
}

/*
 * Function:    Command_creator()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the name, ubitname and mail address of the creator
 */
void Client::command_creator() {
    m_name = (char* ) malloc(MAX_CREATOR_LEN);
    m_ubitName = (char* ) malloc(MAX_CREATOR_LEN);
    m_ubEmail = (char* ) malloc(MAX_CREATOR_LEN);
    strcpy(m_name, "Srikanth Reddy Malipatel\0");
    strcpy(m_ubitName, "smalipat\0");
    strcpy(m_ubEmail, "srmalipat@buffalo.edu\0");
    printf("Creator:%s\tUBITname:%s\tUBEmail:%s \n",m_name, m_ubitName, m_ubEmail);
}

/*
 * Function:    Command_display()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the IP address and the listening port of this process
 */
void Client::command_display() {
    printf("Machine's IP Address is: %s and Listening port of server is: %d \n", m_ipAddress, m_nListenPort);
}

/*
 * Function:    Command_Register()
 * Parameters:  IP address of the client and Listening port number
 * Returns:     1 if successful else 0
 * Description: This functions adds the IP address and listening port of the client, which sent register command, to registered_list.
 */
int Client::command_register() {
    // Complete this and also fill appropriate parameters
}

/*
 * Function:    Command_list()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays a numbered list of all the connections this process is part of. This includes connections initated by this process
 *              and by other process.
 *              Format: <id>    <HostName>      <IP Address>        <Port Number>
 */
void Client::command_list() {
    // Complete this
}

/*
 * Function:    Command_terminate(int connectionID)
 * Parameters:  int connectionId
 * Returns:     None
 * Description: This functions will terminate a connection already in registered_list
 */
void Client::command_terminate(int connectionID) {
    // Complete this
}

/*
 * Function:    Command_quit()
 * Parameters:  None
 * Returns:     None
 * Description: This functions closes all connections and terminates this process
 */
void Client::command_quit() {
    // Complete this
}

/*****************************************************************************
 *                      Server utility functions                             *
 * **************************************************************************/

/*
 * Function:    updateIpAddress()
 * Parameters:  None
 * Returns:     None
 * Description: This function updates m_ipAddress buffer with the public interface ip
 */
void Client::updateIpAddress() {
    struct ifaddrs *ifAddr;
    char host[INET_ADDRSTRLEN];

    // ifAddr contains a list of all local interfaces
    getifaddrs(&ifAddr);
    while(ifAddr != NULL) {
        // ignore the nodes which do not contain struct sockaddr
        if(ifAddr->ifa_addr == NULL)
            continue;
        // We need to consider only ipv4 address
        if(ifAddr->ifa_addr->sa_family == AF_INET) {
            // get the ip address of this interface and update if it is not a local or private ip address
            getnameinfo(ifAddr->ifa_addr, sizeof(struct sockaddr_in), host, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            if(strcmp(host,"127.0.0.1") != 0 && strstr(host, "192.168") == NULL) 
                strcpy(m_ipAddress, host);
        }
        ifAddr = ifAddr->ifa_next;
    }
    return;
}
