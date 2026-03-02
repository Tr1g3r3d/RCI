#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

/**
 * @brief TCP client inicialization
 *
 * @param ipaddr IPADDR of the node to connect to
 * @param port PORT of the node to connect to
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int client_init(char *ipaddr, unsigned int port)
{
    //cria o socket de client
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        return -1;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ipaddr); // Endereço do servidor
    server_addr.sin_port = htons(port);              // Porta do servidor

    //conecta-se ao servidor do cliente
    int status = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status == -1)
    {
        perror("[ERROR]: Erro ao conectar-se ao servidor");
        return -1;
    }

    return sockfd;
}