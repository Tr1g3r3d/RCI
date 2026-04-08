#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#include "../headers/utils.h"
#include "../headers/structs.h"
#include "../headers/processing.h"

#define MAX_NODES 100

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

/**
 * @brief Sending a message to a TCP server
 *
 * @param sockfd Server TCP socket to send to
 * @param message Message to send
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int send_tcp_message(int sockfd, char *message)
{

    // envia uma mensagem para o servidor do cliente
    int status = send(sockfd, message, strlen(message), 0);
    if (status == -1)
    {
        perror("[ERROR]: Erro ao enviar mensagem para o servidor");
        return -1;
    }

    return 0;
}

/**
 * @brief Receiving a message from a TCP server
 *
 * @param sockfd Server TCP socket to receive from
 * @param buffer Pointer to the string that saves the message
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int receive_tcp_message(int sockfd, char *buffer)
{
    // define o tempo limite para a operação de recebimento de socket
    struct timeval timeout;
    timeout.tv_sec = 1;  // um segundo
    timeout.tv_usec = 0; // sem microsegundos
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
    {
        perror("[ERROR]: Erro ao definir o tempo limite");
        return -1;
    }

    //recebe durante 1 segundo mensagens do cliente
    int bytes_received = recv(sockfd, buffer, 1024, 0);
    if (bytes_received == -1)
    {
        if (errno == EAGAIN)
        {
            printf("[ERROR]: Nothing was received for 1 second.\n");
            return -2;
        }
        else
        {
            perror("[ERROR]: Erro ao receber mensagem do cliente");
            return -1;
        }
    }
    buffer[bytes_received] = '\0'; // Adiciona um terminador de string

    return bytes_received;
}

/**
 * @brief Establishing a connection with another node
 *
 * @param node Necessary information about the node
 * @param ipaddr IP ADDRESS of the node to connect to
 * @param port PORT of the node to connect to
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int establish_connection(t_nodeinfo *node, unsigned int id, char *ipaddr, char *port)
{
    //inicia a ligação ao cliente
    int sockfd = client_init(ipaddr, strtoui(port));
    if (sockfd == -1)
        return -1;

    //envia mensagem de NEIGHBOR
    char tcp_message[64] = "";
    sprintf(tcp_message, "NEIGHBOR %02d\n", node->self_id);

    int snd = send_tcp_message(sockfd, tcp_message);
    if (snd == -1)
        return -1;
    
    if(node->monitoring) printf("\r[INFO]: NEIGHBOR message sent to NODE %02d: %s\n", id, tcp_message);

    add_int(&node->int_list, id, sockfd);
    node->n_int++;

    sync_new_neighbor_routes(node, id);

    printf("\n[INFO]: Connection established successfully.\n\n");
    return 0;
}

/**
 * @brief UDP client connection to the node server and its processing
 *
 * @param node Necessary information about the node
 * @param ipaddr IP ADDRESS of the node server
 * @param port PORT of the node server
 * @param message Message to send to the server
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int client_udp(t_nodeinfo *node, char *ipaddr, char *port, char *message, unsigned int tid, int joining)
{
    int sockfd;
    struct sockaddr_in servaddr;

    // cria o socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("[ERROR]: Erro ao criar o socket");
        return -1;
    }

    // define o endereço IP e a porta do servidor UDP
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(strtoui(port));
    if (inet_aton(ipaddr, &servaddr.sin_addr) == 0)
    {
        perror("[ERROR]: Erro ao converter endereço IP");
        return -1;
    }

    // envia uma mensagem para o servidor
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("[ERROR]: Erro ao enviar mensagem");
        return -1;
    }

    char buffer[1024];
    socklen_t len = sizeof(servaddr);
    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, &len);
    if (n > 0)
    {
        buffer[n] = '\0'; // adiciona o caractere nulo ao final do buffer
    }
    else
    {
        printf("[ERROR]: Nenhuma mensagem recebida.\n");
    }

    if(strncmp(buffer, "NODES ", 6) == 0)
    {
        unsigned int net, op, r_tid;
        char id[3] = "";
        sprintf(id, "%02d", node->self_id);

        char *nodes[MAX_NODES];
        int num_nodes = 0;

        char *token = strtok(buffer, "\n");
        if(sscanf(token, "NODES %u %u %u", &r_tid, &op, &net) != 3)
        {
            fprintf(stderr, "[ERROR]: Failed to receive the right NODES message.\n");
            return -1;
        }

        if(op != 1)
        {
            fprintf(stderr, "[ERROR]: Failed to receive the right NODES message. OP received %d\n", op);
        }
        if(r_tid != tid)
        {
            fprintf(stderr, "[ERROR]: Failed to receive the right NODES message. TID sent: %03d - TID received %03d\n", tid, r_tid);
        }

        token = strtok(NULL, "\n");
        int *nodes_ids = (int *)calloc(MAX_NODES, sizeof(int));
        int already_taken = 0, temp_id;

        while (token != NULL)
        {
            if(strncmp(token, id, 2) == 0)
            {
                already_taken = 1;
            }
            if (sscanf(token, "%d", &temp_id) != 1)
            {
                fprintf(stderr, "[ERROR]: Failed to read the right format of NODE in NODES message\n");
                return -1;
            }
            nodes_ids[temp_id] = 1;
            nodes[num_nodes++] = token;
            token = strtok(NULL, "\n");
        }

        if(joining == 1)
        {
            if (already_taken == 0 && node->self_id != -1)
            {
                unsigned int j_tid = rand() % 1000;
                char message[64] = "";
                sprintf(message, "REG %03u 0 %03d %02d %s %s", j_tid, net, node->self_id, node->self_ipaddr, node->self_port);
                //sends a udp message
                if (client_udp(node, node->reg_ipaddr, node->reg_port, message, j_tid, 0) != 0)
                {
                    fprintf(stderr, "[ERROR]: ERROR sending udp message!\n");
                    return -1;
                }
                return 0;
            }

            if (already_taken == 1 && node->self_id != -1)
            {
                printf("\n[INFO]: Your NODE ID (%02d) is already taken!\nTry again with one that isn't in the network.\n", node->self_id);
                printf("Network %3d has the following nodes:\n", net);
                for (int i = 0; i < num_nodes; i++)
                    printf("%s\n", nodes[i]);
                printf("\n");
                node->self_id = -1;
                node->net = -1;
                return 0;
            }
        }

        printf("\n[INFO]: Network %03d has the following nodes:\n", net);
        for (int i = 0; i < num_nodes; i++)
            printf("Node %s\n", nodes[i]);
        printf("\n");

    }
    else if(strncmp(buffer, "REG ", 4) == 0)
    {
        if(node->monitoring) printf("\n[INFO]: REG message received: %s\n", buffer);
        unsigned int net;
        char id[3] = "";
        sprintf(id, "%02d", node->self_id);

        char *token = strtok(buffer, " ");
        token = strtok(NULL, " ");
        if (strtoui(token) != tid)
        {
            printf("\n[ERROR]: Wrong TID received!\n\tSent: %d - Received: %s\n\n", tid, token);
            return -1;
        }
        token = strtok(NULL, " ");
        unsigned int op = strtoui(token);
        if(op == 1)
        {
            token = strtok(NULL, " ");
            net = strtoui(token);
            token = strtok(NULL, " ");
            printf("\n[INFO]: Node %s successfully joined network %u!\n\n", token, net);
        }
        else if (op == 2)
        {
            printf("\n[INFO]: Network already full! Try another one.\n\n");
        }
        else if (op == 4)
        {
            token = strtok(NULL, " ");
            net = strtoui(token);
            token = strtok(NULL, " ");
            printf("\n[INFO]: Node %s successfully left network %u!\n\n", token, net);
        }
        else
        {
            printf("\n[ERROR]: Error trying to join network! OP receveid: %u\n\n", op);
        }
    }
    else if(strncmp(buffer, "CONTACT ", 8) == 0)
    {
        if(node->monitoring) printf("\n[INFO]: CONTACT message received: %s\n", buffer);
        unsigned int net, op, r_tid, id;
        char ipaddr[INET_ADDRSTRLEN], port[6];

        char *token = strtok(buffer, "\n");
        if(sscanf(token, "CONTACT %u %u %u %u %s %s", &r_tid, &op, &net, &id, ipaddr, port) != 6)
        {
            fprintf(stderr, "\n[ERROR]: Failed to receive the right CONTACT message.\n\n");
            return -1;
        }

        if (op == 1)
        {
            if (establish_connection(node, id, ipaddr, port) != 0)
                return -1;            

        }
        else if (op == 2)
        {
            printf("\n[INFO]: NODE %02u isn't registered in the network!\n\n", id);
        }
        else
        {
            printf("\n[ERROR]: Error contacting NODE %02u! OP receveid: %u\n\n", id, op);
        }

    }

    close(sockfd);
    return 0;
}