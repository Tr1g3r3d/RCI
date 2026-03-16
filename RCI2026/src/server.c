#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "../headers/client.h"
#include "../headers/processing.h"
#include "../headers/structs.h"


/**
 * @brief TCP server inicialization 
 *
 * @param port PORT of the TCP server to initiate
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int tcp_server_init(char port[])
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
    if (sockfd == -1) return -1; //error 
 
    struct addrinfo hints, *res;
    memset (&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int errcode = getaddrinfo (NULL, port, &hints, &res);
    if ((errcode) != 0) return -1; //error

    ssize_t n = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (n == -1) return -1; //error

    if (listen(sockfd, 5) == -1) return -1; //error    

    return sockfd;
}

/**
 * @brief Acceptance of a new connection to the TCP server
 *
 * @param sockfd TCP server socket
 * @return [ @b int ] Value of TCP client socket if sucessfull, -1 otherwise
 */
int accept_client_connection(int sockfd)
{

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sockfd == -1)
    {
        perror("[ERROR]: Erro ao aceitar conexão de cliente");
        return -1;
    }

    return client_sockfd;
}

/**
 * @brief Incoming connection to the TCP server processing
 *
 * @param node Necessary information about the node
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int incoming_connection(t_nodeinfo *node)
{
    int client_sockfd = accept_client_connection(node->server_tcp_sockfd);
    if (client_sockfd == -1)
        return -1;

    char buffer[1024];
    int rcv = receive_tcp_message(client_sockfd, buffer);
    if (rcv == -1)
    {
        return -1;
    }
    else if (rcv == 0)
    {
        return 0;
    }
    else if (rcv == -2)
    {
        return 0;
    }

    char *token = strtok(buffer, "\n");
    while (token != NULL)
    {
        char *server_response = process_tcp_message(node, token, client_sockfd, -1);
        if (strcmp(server_response, "\0") == 0)
            return -1;
        token = strtok(NULL, "\n");
    }

    return 0;
}