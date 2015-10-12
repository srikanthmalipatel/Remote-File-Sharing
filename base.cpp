/*
 * common.cpp
 *
 *  Created on: Oct 10, 2015
 *      Author: SrikanthMalipatel
 */

#include "base.h"


// given a char buffer it sends all the buffer contents until remaining bytes are zero
int Base::sendall(int sockFd, char *buf, int *length)
{
	int btotal = 0; 			// total number of bytes to send
	int bleft = *length; 	// number of bytes remaining to be sent
	int n;
	while(btotal < *length)
	{
		n = send(sockFd, buf+btotal, bleft, 0);
		if (n == -1)
		{
			break;
		}
		btotal += n;
		bleft -= n;
	}
	*length = btotal; // actual number of bytes sent
	if(n == -1)
		return -1;	// failed
	return 0;	// success
}

bool Base::getHostName(char *ip, char *buf, bool printErr) {
	struct hostent *host;
	struct in_addr ipv4addr;
	if(!inet_aton(ip, &ipv4addr)) {
		if(printErr)
			perror("inet_aton");
		return false;
	}

	if((host = gethostbyaddr((const void*)&ipv4addr, sizeof ipv4addr, AF_INET)) == NULL) {
		if(printErr)
			perror("gethostbyaddr");
		return false;
	}
	strcpy(buf, host->h_name);
	return true;
}

bool Base::getIPaddress(char *hostname, char *buf, bool printErr) {
	struct hostent *host;
	struct in_addr **ipv4addr;
	if ( (host = gethostbyname( hostname ) ) == NULL)
	{
		if(printErr)
			perror("gethostbyname");
		return false;
	}

	ipv4addr = (struct in_addr **) host->h_addr_list;
	for(int i = 0; ipv4addr[i] != NULL; i++)
	{
		strcpy(buf , inet_ntoa(*ipv4addr[i]) );
		return true;
	}
	return false;
}





