/*
 * ==========================================================================
 *      
 *      Filename: server.cpp
 *
 *      Description: 
 * 
 *      Date: 2015-09-22 22:37
 *
 *      Author: Srikanth Malipatel
 *
 * ==========================================================================
 */

#include "server.h"

/*
 * Function:    Constructor
 */
Server::Server(int port) {
    printf("Starting server on Port: %d \n", port);
    this->m_nListenPort = port;
}

/*
 * Function:    Destructor
 */
Server::~Server() {

}

/*
 * Function:    getCommandID(char[] comnd)
 * Parameters:  comnd: command to be executed
 * Returns:     if string is found then returns ComandID else -1
 * Description: This functions takes a string and returns appropriate CommandID
 */
CommandID Server::getCommandID(char comnd[]) {
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
void Server::commandShell() {
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
                // handle DISPLAY command
                command_display();
                break;
            case COMMAND_LIST:
                // handle LIST command
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
void Server::command_help() {
    printf("User command options \n");
    printf("\tCREATOR - Displays creators full name, UBIT name and UB email address\n");
    printf("\tDIPSLAY - Displays the IP address and listening port of this process\n");
    printf("\tLIST - Displays a list of clients registered on this server\n");
    printf("\n");
}

/*
 * Function:    Command_creator()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the name, ubitname and mail address of the creator
 */
void Server::command_creator() {
    m_name = (char* ) malloc(MAX_CREATOR_LEN);
    m_ubitName = (char* ) malloc(MAX_CREATOR_LEN);
    m_ubEmail = (char* ) malloc(MAX_CREATOR_LEN);
    strncpy(m_name, "Srikanth Reddy Malipatel\0", MAX_CREATOR_LEN);
    strncpy(m_ubitName, "smalipat\0", MAX_CREATOR_LEN);
    strncpy(m_ubEmail, "smalipat@buffalo.edu\0", MAX_CREATOR_LEN);
    printf("NAME:%s\tUBITname:%s\tUBEmail:%s \n",m_name, m_ubitName, m_ubEmail);
}

/*
 * Function:    Command_display()
 * Parameters:  None
 * Returns:     None
 * Description: This functions displays all the IP address and the listening port of this process
 */
void Server::command_display() {
    // Complete this
}

/*
 * Function:    Command_Register()
 * Parameters:  IP address of the client and Listening port number
 * Returns:     1 if successful else 0
 * Description: This functions adds the IP address and listening port of the client, which sent register command, to registered_list.
 */
int Server::command_register() {
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
void Server::command_list() {
    // Complete this
}

/*
 * Function:    Command_terminate(int connectionId)
 * Parameters:  int connectionId
 * Returns:     None
 * Description: This functions will terminate a connection already in registered_list
 */
void Server::command_terminate(int connectionId) {
    // Complete this
}

/*
 * Function:    Command_quit()
 * Parameters:  None
 * Returns:     None
 * Description: This functions closes all connections and terminates this process
 */
void Server::command_quit() {
    // Complete this
}
