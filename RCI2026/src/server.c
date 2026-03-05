#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>


/**
 * @brief TCP server inicialization
 *
 * @param port PORT of the TCP server to initiate
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int server_init(char *iptcp, char *porttcp)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
    if (sockfd == -1) return -1; //error

    struct addrinfo hints, *res;
    memset (&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int errcode = getaddrinfo (iptcp, porttcp, &hints, &res);
    if ((errcode) != 0) return -1; //error

    ssize_t n = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (n == -1) return -1; //error

    if (listen(sockfd, 5) == -1) return -1; //error    

    return sockfd;
}

int server_close(int sockfd) {
    return close(sockfd);
}