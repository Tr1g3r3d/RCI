#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <netdb.h>
#include "../headers/client.h"



/**
 * @brief TCP client inicialization
 *
 * @param ipaddr IPADDR of the node to connect to
 * @param port PORT of the node to connect to
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int client_init(char *ipaddr, char port[])
{
    int fd;
    ssize_t n;
    struct sockaddr_in addr;
    fd=socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return -1; //error

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    if (inet_pton(AF_INET, ipaddr, &addr.sin_addr) <= 0) return -1; //error


    n=connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (n == -1) return -1; //error

    return fd;
}