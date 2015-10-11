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

void Base::getHostName(char *ip, char *buf) {
	/*struct sockaddr_in remoteAddress;
	socklen_t len = sizeof(remoteAddress);
	if (getpeername(sockFd, (struct sockaddr*)&remoteAddress, &len)!=0)
	{
		// could not get socket info of the peer so to identify hostname will be server
		cout << "getpeername error" << endl;
		strcpy(buf,"SERVER");
	}
	else
	{
		len = sizeof(remoteAddress);
		if(getnameinfo((struct sockaddr*)&remoteAddress, remoteAddress.sin_len, buf, sizeof(buf), NULL, 0, NI_NUMERICHOST)!=0)
		{
			// could not resolve host info of the peer so to identify hostname will be server
			cout << "getnameinfo error" << endl;
			strcpy(buf,"SERVER");
		}
	}
	cout << "getHostName buf: " << buf << endl;*/
	printf("ip %s \n", ip);
	struct in_addr ipv4addr;
	if(!inet_aton(ip, &ipv4addr)) {
		perror("inet_aton");
		exit(EXIT_FAILURE);
	}
	struct hostent *host;
	if((host = gethostbyaddr((const void*)&ipv4addr, sizeof ipv4addr, AF_INET)) == NULL) {
		perror("gethostbyaddr");
		exit(EXIT_FAILURE);
	}
	strcpy(buf, host->h_name);

}





