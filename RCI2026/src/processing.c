#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../headers/structs.h"
#include "../headers/client.h"
#include "../headers/utils.h"
#include "../headers/server.h"

/**
 * @brief User help message for incorret input
 *
 */
void user_message_help()
{
    puts("");
    puts("\t\033[mjoin \x1b[4m(n) \x1b[3mnet id\033[m\t\t\t\t-> Join a network");
    puts("\t\033[mshow \x1b[3mnodes \x1b[4m(n) \x1b[3mnet\033[m\t\t\t-> Show nodes in network");
    puts("\t\033[mleave \x1b[4m(l)\033[m\t\t\t\t-> Leave a network");
    puts("\t\033[mexit \x1b[4m(x)\033[m\t\t\t\t-> Exit the programm");
    puts("\t\033[msadd \x1b[3medge \x1b[4m(ae) \x1b[3mid\033[m\t\t\t-> Connect to another NODE");
    puts("\t\033[mremove \x1b[3medge \x1b[4m(re) \x1b[3mid\033[m\t\t\t-> Disconnect from another NODE");
    puts("\t\033[mshow \x1b[3mneighbors \x1b[4m(sg)\033[m\t\t\t-> Show connected neighbors list");
    puts("\t\033[mannounce \x1b[4m(a)\033[m\t\t\t\t-> Announce the NODE to the network");
    puts("\t\033[mshow \x1b[3mrouting \x1b[4m(sr) \x1b[3mdest\033[m\t\t\t-> Show routing information to destination NODE");
    puts("\t\033[mstart \x1b[3mmonitor \x1b[4m(sm)\033[m\t\t\t-> Activate message monitoring");
    puts("\t\033[mend \x1b[3mmonitor \x1b[4m(em)\033[m\t\t\t-> Deactivate message monitoring");
    puts("\t\033[mmessage \x1b[4m(m) \x1b[3mdest message\033[m\t\t-> Send message to destination NODE");
    puts("\t\033[mdirect \x1b[3mjoin \x1b[4m(dj) \x1b[3mnet id\033[m\t\t\t-> Directly join a network without communicating with NODE server");
    puts("\t\033[mdirect \x1b[3madd \x1b[3medge \x1b[4m(dae) \x1b[3mid idIP idTCP\033[m\t-> Directly connect to another NODE");
    puts("");
}

/**
 * @brief Processing of the join command
 *
 * @param node Necessary information about the node
 * @param net NET ID your connected to
 * @param id NODE ID of your node
 * @return [ @b int ] 0 if successfull or only input error, -1 otherwise
 */
int process_join_command(t_nodeinfo *node, int net, int id)
{

    if (id < 0 || id > 99) // checks if is a valid id
    {
        fprintf(stderr, "[ERROR]: ID must be between 01 and 99 (was %d).\n", id);
        return 0;
    }
    if (net < 1 || net > 999) // checks if is a valid network
    {
        fprintf(stderr, "[ERROR]: NET must be between 01 and 999 (was %d).\n", net);
        return 0;
    }

    //iniciate the basic and intial informtion of the node
    node->self_id = id;
    node->net = net;

    unsigned int tid = rand() % 1000;
    char message[64] = "";
    sprintf(message, "NODES %03u 0 %03d", tid, net);

    //sends a udp message
    if (client_udp(node, node->reg_ipaddr, node->reg_port, message, tid, 1) != 0)
    {
        fprintf(stderr, "[ERROR]: ERROR sending udp message!\n");
        return -1;
    }

    if (node->self_id != -1)
    {
        printNode(node);
        printf("\n");
    }
        
    return 0;
}

int process_n_command(t_nodeinfo *node, unsigned int net)
{
    unsigned int tid = rand () % 1000;
    char message[64] = "";
    sprintf(message, "NODES %03u 0 %03u", tid, net);
    if(client_udp(node, node->reg_ipaddr, node->reg_port, message, tid, 0) != 0)
    {
        fprintf(stderr, "[ERROR]: ERROR sending udp message!\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Processing of the leave command
 *
 * @param node Necessary information about the node
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int process_leave_command(t_nodeinfo *node)
{
    if (node->self_id == -1)
        return 0;
        
    unsigned int tid = rand() % 1000;
    char message[64] = "";
    sprintf(message, "REG %03u 3 %03d %02d", tid, node->net, node->self_id);

    if (client_udp(node, node->reg_ipaddr, node->reg_port, message, tid, 0) != 0)
    {
        fprintf(stderr, "[ERROR]: ERROR sending udp message!\n");
        return -1;
    }

    close_sockets(node);

    node_refresh(node);

    return 0;
}

int process_ae_command(t_nodeinfo *node, int id)
{
    unsigned int tid = rand() % 1000;
    char message[64] = "";
    sprintf(message, "CONTACT %03u 0 %03d %02d", tid, node->net, id);

    if(client_udp(node, node->reg_ipaddr, node->reg_port, message, tid, 0) != 0)
    {
        fprintf(stderr, "[ERROR]: ERROR sending udp message!\n");
        return -1;
    }

    return 0;
}

int process_re_command(t_nodeinfo *node, unsigned int id)
{
    for (t_int *current = node->int_list; current != NULL; current = current->next_int)
    {
        if (current->int_id == id)
        {
            close(current->int_tcp_sockfd);
            break;
        }
    }

    remove_int(&node->int_list, id);
    node->n_int--;

    printf("\n[INFO]: Edege with NODE %02u disconnected.\n\n", id);

    return 0;
}

void process_sg_command(t_nodeinfo *node){ print_int(node->int_list); }

/**
 * @brief Processing of the user command input
 *
 * @param node Necessary information about the node
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int process_user_input(t_nodeinfo *node)
{

    char input[128];
    fgets(input, 128, stdin);

    if (strncmp(input, "join ", 5) == 0 || strncmp(input, "j ", 2) == 0)
    {
        if (node->self_id != -1)
        {
            fprintf(stderr, "[INFO]: NODE already in a network\n\n");
            return 0;
        }
        int net, id;
        char *token = strchr(input, ' ');
        if (token && sscanf(token + 1, "%u %u\n", &net, &id) == 2)
        {
            if (process_join_command(node, net, id) != 0)
                return -1;
            return 0;
        }
        else
        {
            puts("[ERROR]: Invalid format.\n         Usage: \x1b[4mj\033[moin \x1b[3mnet id\033[m");
        }
        return 0;
    }
    if (strncmp(input, "show nodes ", 11) == 0 || strncmp(input, "n ", 2) == 0)
    {
        int net;
        char * token = strchr(input, ' ');
        if (token && sscanf(token + 1, "%d\n", &net) == 1)
        {
            if (net < 0 || net > 999)
            {
                puts("[ERROR]: Invalid Network. (Must be between 000 and 999)\n");
                return 0;
            }
            /* process show nodes */
            if(process_n_command(node, net) != 0)
                return -1;
            return 0;
        }
        else
        {
            puts("[ERROR]: Invalid format.\n");
        }
        return 0;
    }
    if (strcmp(input, "leave\n") == 0 || strcmp(input, "l\n") == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "\n[INFO]: NODE already not in a network.\n\n");
        }
        else
        {
            if (process_leave_command(node) != 0)
                return -1;
        }
        return 0;
    }
    if (strcmp(input, "exit\n") == 0 || strcmp(input, "x\n") == 0)
    {
        if(node->self_id != -1)
        {
            if (process_leave_command(node) != 0)
            return -1;
        }

        if (node->server_tcp_sockfd != -1)
            close(node->server_tcp_sockfd);
        printf("[INFO]: success in exiting the program.\n");
        return 1;
    }
    if (strncmp(input, "add edge ", 11) == 0 || strncmp(input, "ae ", 3) == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "[INFO]: NODE isn't in a network! Please join a network first.\n\n");
            return 0;
        }
        int id;
        char *token = strchr(input, ' ');
        if (token && sscanf(token + 1, "%u\n", &id) == 1)
        {
            if (process_ae_command(node, id) != 0)
                return -1;
            return 0;
        }
        else
        {
            puts("[ERROR]: Invalid format.\n         Usage: \033[msadd \x1b[3medge \x1b[4m(ae) \x1b[3mid\033[m");
        }
        return 0;
    }
    if (strncmp(input, "remove edge ", 11) == 0 || strncmp(input, "re ", 3) == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "[INFO]: NODE isn't in a network! Please join a network first.\n\n");
            return 0;
        }
        unsigned int id;
        char *token = strchr(input, ' ');
        if (token && sscanf(token + 1, "%u\n", &id) == 1)
        {
            if (process_re_command(node, id) != 0)
                return -1;
            return 0;
        }
        else
        {
            puts("[ERROR]: Invalid format.\n         Usage: \033[mremove \x1b[3medge \x1b[4m(re) \x1b[3mid\033[m");
        }
        return 0;
    }
    if (strncmp(input, "show neighbors\n", 15) == 0 || strncmp(input, "sg\n", 3) == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "[INFO]: NODE isn't in a network! Please join a network first.\n\n");
            return 0;
        }
        process_sg_command(node);
        return 0;
    }

    size_t input_len = strlen(input);
    if (input_len > 0)
        input[input_len - 1] = '\0';
    if (input_len == 1)
        return 0;

    printf("[ERROR]: Invalid command \"%s\". Available commands:\n", input);
    user_message_help();
    return 0;
}

/**
 * @brief Processing of the message received by TCP socket
 *
 * @param node Necessary information about the node
 * @param message Message received
 * @param client_sockfd TCP socket that received the message
 * @param origem NODE ID of the node that sent the message
 * @return [ @b char* ] Pointer to the reply message string
 */
char *process_tcp_message(t_nodeinfo *node, char *message, int client_sockfd, int origem)
{

    static char server_message[128] = "";
    if (strncmp(message, "NEIGHBOR ", 9) == 0 && client_sockfd != -1)
    {

        int id;
        char *token = strchr(message, ' ');
        if (sscanf(token + 1, "%d\n", &id) != 1)
        {
            fprintf(stderr, "[ERROR]: Wrong NEIGHBOR message format\n");
            strcpy(server_message, "\0");
            return server_message;
        }

        printf("\r[INFO]: New connection to NODE %02d\n\n", id);

        add_int(&node->int_list, id, client_sockfd);
        node->n_int++;
        
        sprintf(server_message, "OK. message received from node %02d\n", origem);
    }
    else
    {
        printf("\r[ERROR]: Special message not processed: %s\n\n", message);
        strcpy(server_message, "Special message sent was not processed.\n");
    }

    return server_message;
}

/**
 * @brief Internal node disconnet processing
 *
 * @param node Necessary information about the node
 * @param int_id NODE ID of the internal node that disconnected
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int process_intern_disconnection(t_nodeinfo *node, unsigned int int_id)
{
    printf("\r[INFO]: Edege with NODE %02d disconnected.\n", int_id);

    for (t_int *current = node->int_list; current != NULL; current = current->next_int)
    {
        if (current->int_id == int_id)
        {
            close(current->int_tcp_sockfd);
            break;
        }
    }

    remove_int(&node->int_list, int_id);
    node->n_int--;
    return 0;
}

/**
 * @brief Internal node connection processing
 *
 * @param node Necessary information about the node
 * @param int_id NODE ID of the internal node that sent a message
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int process_intern_connection(t_nodeinfo *node, unsigned int int_id)
{

    char buffer[1024];
    int intsock = findIntSock(node->int_list, int_id);
    int rcv = receive_tcp_message(intsock, buffer);
    if (rcv == -1)
        return -1;
    else if (rcv == 0)
    {
        int result = process_intern_disconnection(node, int_id);
        return result;
    }
    else if (rcv == -2)
    {
        return 0;
    }
    else
    {
        char *token = strtok(buffer, "\n");
        while (token != NULL)
        {
            char *server_response = process_tcp_message(node, buffer, intsock, int_id);
            if (strcmp(server_response, "\0") == 0)
                return -1;
            else if (strcmp(server_response, "OK\n") == 0)
                return 0;
            int snd = send_tcp_message(intsock, server_response);
            if (snd == -1)
                return -1;
            token = strtok(NULL, "\n");
        }
        return 0; // process other messages
    }
}