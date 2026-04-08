#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../headers/structs.h"
#include "../headers/client.h"
#include "../headers/utils.h"
#include "../headers/server.h"

#define INF 999999

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

int enter_coordination(t_nodeinfo *node, int dest, int caused_by)
{
    t_routing *r = get_rout(node->rout_list, dest);
    if (r == NULL) return 0;

    r->state = 1;
    r->succ_coord = caused_by;   // sucessor antigo, ou -1 se foi falha local
    r->dist = INF;
    r->succ = -1;

    for (t_int *cur = node->int_list; cur != NULL; cur = cur->next_int)
    {
        char msg[64];
        sprintf(msg, "COORD %02d\n", dest);
        if (send_tcp_message(cur->int_tcp_sockfd, msg) != 0)
            return -1;
        if(node->monitoring) printf("\r[INFO]: COORD message sent to NODE %02d: %s\n", cur->int_id, msg);
        set_coord_state(r, cur->int_id, 1);
    }

    return 0;
}

int sync_new_neighbor_routes(t_nodeinfo *node, int neigh_id)
{
    int sock = findIntSock(node->int_list, neigh_id);
    if (sock == -1) return -1;

    for (t_routing *r = node->rout_list; r != NULL; r = r->next_rout)
    {
        if (r->state == 0 && r->dist < INF)
        {
            char msg[64];
            sprintf(msg, "ROUTE %02d %d\n", r->dest, r->dist);
            if(send_tcp_message(sock, msg) != 0)
                return -1;
            if(node->monitoring) printf("\r[INFO]: ROUTE message sent to new neighbor NODE %02d: %s\n", r->dest, msg);
            
        }
        else if (r->state == 1)
        {
            set_coord_state(r, neigh_id, 0);
        }
    }
    return 0;
}

int handle_lost_neighbor_for_routes(t_nodeinfo *node, int lost_id)
{
    for (t_routing *r = node->rout_list; r != NULL; r = r->next_rout)
    {
        if (r->state == 0 && r->succ == lost_id)
        {
            if (enter_coordination(node, r->dest, -1) != 0)
                return -1;
        }
        else if (r->state == 1)
        {
            set_coord_state(r, lost_id, 0);
        }
    }
    return 0;
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
    if(node->monitoring) printf("\r[INFO]: NODE message sent: %s\n", message);

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

int process_direct_join_command(t_nodeinfo *node, int net, int id)
{
    if (id < 0 || id > 99)
    {
        fprintf(stderr, "[ERROR]: ID must be between 01 and 99 (was %d).\n", id);
        return 0;
    }
    if (net < 1 || net > 999)
    {
        fprintf(stderr, "[ERROR]: NET must be between 01 and 999 (was %d).\n", net);
        return 0;
    }

    node->self_id = id;
    node->net = net;

    printNode(node);
    printf("\n");

    return 0;
}

int process_n_command(t_nodeinfo *node, unsigned int net)
{
    unsigned int tid = rand () % 1000;
    char message[64] = "";
    sprintf(message, "NODES %03u 0 %03u", tid, net);
    if(node->monitoring) printf("\r[INFO]: NODE message sent: %s\n", message);
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
    if(node->monitoring) printf("\r[INFO]: REG message sent: %s\n", message);

    if (client_udp(node, node->reg_ipaddr, node->reg_port, message, tid, 0) != 0)
    {
        fprintf(stderr, "[ERROR]: ERROR sending udp message!\n");
        return -1;
    }

    close_sockets(node);

    node_refresh(node);

    return 0;
}

int process_a_command(t_nodeinfo * node, int dist)
{
    if (node->int_list == NULL)
    {
        printf("\n[INFO]: NODE doesn't have any neighbors yet.\n\n");
        return 0;
    }

    for (t_int *cur = node->int_list; cur != NULL; cur = cur->next_int) {
        char msg[64];
        sprintf(msg, "ROUTE %02d %d\n", node->self_id, dist);
        if(node->monitoring) printf("\r[INFO]: ROUTE message sent to neighbor NODE %02d: %s\n", cur->int_id, msg);
        if (send_tcp_message(cur->int_tcp_sockfd, msg) != 0)
            return -1;
        t_routing *r = get_rout(node->rout_list, cur->int_id);
        if (r == NULL)
        {
            add_rout(&node->rout_list, cur->int_id, cur->int_id);
        }
        else
        {
            r->succ = cur->int_id;
            r->dist = 1;
            r->state = 0;
        }
    }
    
    return 0;
}

int process_ae_command(t_nodeinfo *node, int id)
{
    if(findIntSock(node->int_list, id) != -1)
    {
        printf("\n[INFO]: Already have a connection with NODE %02d!\n\n", id);
        return 0;
    }

    if(node->self_id == id)
    {
        printf("\n[ERROR]: Cannot connect to itself!\n\n");
        return 0;
    }

    unsigned int tid = rand() % 1000;
    char message[64] = "";
    sprintf(message, "CONTACT %03u 0 %03d %02d", tid, node->net, id);
    if(node->monitoring) printf("\r[INFO]: CONTACT message sent: %s\n", message);

    if(client_udp(node, node->reg_ipaddr, node->reg_port, message, tid, 0) != 0)
    {
        fprintf(stderr, "[ERROR]: ERROR sending udp message!\n");
        return -1;
    }

    return 0;
}

int process_direct_ae_command(t_nodeinfo *node, int id, char idIP[], int idTCP)
{
    unsigned int tid = rand() % 1000;
    char message[64] = "";
    if (node->self_id == -1)
    {
        fprintf(stderr, "\n[INFO]: NODE isn't in a network! Please join a network first.\n\n");
        return 0;
    }

    if(findIntSock(node->int_list, id) != -1)
    {
        printf("\n[INFO]: Already have a connection with NODE %02d!\n\n", id);
        return 0;
    }

    if(node->self_id == id)
    {
        printf("\n[ERROR]: Cannot connect to itself or a node with the same ID!\n\n");
        return 0;
    }

    char port[6];
    snprintf(port, sizeof(port), "%d", idTCP);

    sprintf(message, "CONTACT %03u 0 %03d %02d %s %s", tid, node->net, id, idIP, port);

    if (establish_connection(node, id, idIP, port) != 0)
        return -1;

    return 0;
}

int process_re_command(t_nodeinfo *node, unsigned int id)
{
    if (node->self_id == -1)    
    {
        fprintf(stderr, "\n[INFO]: NODE isn't in a network! Please join a network first.\n\n");
        return 0;
    }

    if(node->self_id == (int)id)
    {
        printf("\n[ERROR]: Cannot disconnect from itself!\n\n");
        return 0;
    }

     if(findIntSock(node->int_list, id) == -1)
    {
        printf("\n[INFO]: No connection with NODE %02d!\n\n", id);
        return 0;
    }

    for (t_int *current = node->int_list; current != NULL; current = current->next_int)
    {
        if (current->int_id == id)
        {
            close(current->int_tcp_sockfd);
            printf("\n[INFO]: Edge with NODE %02u disconnected.\n\n", id);
            remove_int(&node->int_list, id);
            node->n_int--;
            if(handle_lost_neighbor_for_routes(node, id) != 0)
                return -1;
            return 0;
        }
    }

    printf("\n[INFO]: No connection with NODE %02d!\n\n", id);
    return 0;    
}

void process_sg_command(t_nodeinfo *node){ print_int(node->int_list); }

int send_chat_message(t_nodeinfo *node, int origin, int dest, const char *chat)
{
    if (chat == NULL || chat[0] == '\0')
    {
        printf("\n[ERROR]: Empty message.\n\n");
        return 0;
    }

    if ((int)strlen(chat) > 128)
    {
        printf("\n[ERROR]: CHAT message can have at most 128 characters.\n\n");
        return 0;
    }

    if (dest == node->self_id)
    {
        printf("\r[CHAT %02d -> %02d]: %s\n\n", origin, dest, chat);
        return 0;
    }

    t_routing *r = get_rout(node->rout_list, dest);
    if (r == NULL || r->state != 0 || r->dist >= INF || r->succ == -1)
    {
        printf("\n[ERROR]: No valid expedition route to NODE %02d.\n\n", dest);
        return 0;
    }

    if (r->succ == origin && origin != node->self_id)
    {
        printf("\n[ERROR]: Refusing to bounce CHAT back to previous hop.\n\n");
        return 0;
    }

    int sock = findIntSock(node->int_list, r->succ);
    if (sock == -1)
    {
        printf("\n[ERROR]: Expedition neighbor %02d is not connected.\n\n", r->succ);
        return 0;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "CHAT %02d %02d %s\n", origin, dest, chat);

    if (send_tcp_message(sock, msg) != 0)
        return -1;

    if(node->monitoring) printf("[INFO]: CHAT message sent to NODE %02d via NODE %02d: %s\n", dest, r->succ, chat);
    return 0;
}


/**
 * @brief Processing of the user command input
 *
 * @param node Necessary information about the node
 * @return [ @b int ] 0 if sucessfull, -1 otherwise
 */
int process_user_input(t_nodeinfo *node)
{

    char input[1024];
    fgets(input, 1024, stdin);

    if(strncmp(input, "direct join", 11) == 0 || strncmp(input, "dj", 2) == 0)
    {
        if (node->self_id != -1)
        {
            fprintf(stderr, "\n[INFO]: NODE already in a network\n\n");
            return 0;
        }
        int net, id;
        char *token = strchr(input, ' ');
        if (token && sscanf(token + 1, "%u %u\n", &net, &id) == 2)
        {
            if (process_direct_join_command(node, net, id) != 0)
                return -1;
            return 0;
        }
        else
        {
            puts("[ERROR]: Invalid format.\n         Usage: \x1b[4mj\033[moin \x1b[3mnet id\033[m");
        }
        return 0;
    }
    if(strncmp(input, "direct add edge", 15) == 0 || strncmp(input, "dae", 3) == 0)
    {
        int id, idTCP;
        char idIP[INET_ADDRSTRLEN];
        char *token = strchr(input, ' ');
        if (token && sscanf(token + 1, "%u %s %u\n", &id, idIP, &idTCP) == 3)
        {
            if (process_direct_ae_command(node, id, idIP, idTCP) != 0)
                return -1;
            return 0;
        }
        else
        {
            puts("\n[ERROR]: Invalid format.\n         Usage: \033mdirect \x1b[3madd \x1b[3medge \x1b[4m(dae) \x1b[3mid idIP idTCP\033[m");
        }
        return 0;
    }
    if (strncmp(input, "join ", 5) == 0 || strncmp(input, "j ", 2) == 0)
    {
        if (node->self_id != -1)
        {
            fprintf(stderr, "\n[INFO]: NODE already in a network\n\n");
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
                puts("\n[ERROR]: Invalid Network. (Must be between 000 and 999)\n");
                return 0;
            }
            /* process show nodes */
            if(process_n_command(node, net) != 0)
                return -1;
            return 0;
        }
        else
        {
            puts("\n[ERROR]: Invalid format.\n");
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
            fprintf(stderr, "\n[INFO]: NODE isn't in a network! Please join a network first.\n\n");
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
            puts("\n[ERROR]: Invalid format.\n         Usage: \033[msadd \x1b[3medge \x1b[4m(ae) \x1b[3mid\033[m");
        }
        return 0;
    }
    if (strncmp(input, "remove edge ", 11) == 0 || strncmp(input, "re ", 3) == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "\n[INFO]: NODE isn't in a network! Please join a network first.\n\n");
            return 0;
        }
        unsigned int id;
        char *token = strchr(input, ' ');
        if (token && sscanf(token + 1, "%u", &id) == 1)
        {
            if (process_re_command(node, id) != 0)
                return -1;
            return 0;
        }
        else
        {
            puts("\n[ERROR]: Invalid format.\n         Usage: \033[mremove \x1b[3medge \x1b[4m(re) \x1b[3mid\033[m");
        }
        return 0;
    }
    if (strncmp(input, "show neighbors\n", 15) == 0 || strncmp(input, "sg\n", 3) == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "\n[INFO]: NODE isn't in a network! Please join a network first.\n\n");
            return 0;
        }
        process_sg_command(node);
        return 0;
    }
    if (strncmp(input, "announce\n", 9) == 0 || strncmp(input, "a\n", 2) == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "\n[INFO]: NODE isn't in a network! Please join a network first.\n\n");
            return 0;
        }
        if (process_a_command(node, 0) != 0)
            return -1;
        return 0;
    }
    if(strncmp(input, "show routing ", 13) == 0 || strncmp(input, "sr ", 3) == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "\n[INFO]: NODE isn't in a network! Please join a network first.\n\n");
            return 0;
        }
        unsigned int dest;
        char *args = NULL;
        if (strncmp(input, "show routing ", 13) == 0)
            args = input + 13;
        else
            args = input + 3;

        if (sscanf(args, "%u\n", &dest) == 1)
        {
            if (dest > 99)
            {
                printf("\n[ERROR]: Invalid destination id.\n\n");
                return 0;
            }

            print_dest_rout(node->rout_list, dest);
            return 0;
        }
        else
        {
            puts("\n[ERROR]: Invalid format.\n         Usage: \033[mshow \x1b[3mrouting \x1b[4m(sr) \x1b[3mdest\033[m");
        }
        return 0;
    }
    if (strncmp(input, "message ", 8) == 0 || strncmp(input, "m ", 2) == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "\n[INFO]: NODE isn't in a network! Please join a network first.\n\n");
            return 0;
        }

        int dest;
        char chat_full[1024];
        char chat[129];

        char *token = strchr(input, ' ');
        if (token && sscanf(input + 1, "%d %[^\n]", &dest, chat_full) == 2)
        {
            if (dest < 0 || dest > 99)
            {
                printf("\n[ERROR]: Invalid destination id.\n\n");
                return 0;
            }

            if (strlen(chat) == 0)
            {
                printf("\n[ERROR]: Empty message.\n\n");
                return 0;
            }

            if (strlen(chat) > 128)
            {
                printf("\n[ERROR]: Message too long (max 128 characters).\n\n");
                return 0;
            }

            strcpy(chat, chat_full);

            printf("\n");
            if (send_chat_message(node, node->self_id, dest, chat) != 0)
                return -1;

            return 0;
        }
        else
        {
            puts("\n[ERROR]: Invalid format.\n         Usage: message dest text");
            return 0;
        }
    }
    if (strncmp(input, "show coord\n", 11) == 0 || strncmp(input, "sc\n", 3) == 0)
    {
        if (node->self_id == -1)
        {
            fprintf(stderr, "\n[INFO]: NODE isn't in a network! Please join a network first.\n\n");
            return 0;
        }
        print_coord_table(node);
        return 0;
    }
    if (strncmp(input, "start monitor\n", 14) == 0 || strncmp(input, "sm\n", 3) == 0)
    {
        node->monitoring = 1;
        printf("\n[INFO]: Message monitoring activated.\n\n");
        return 0;
    }
    if (strncmp(input, "end monitor\n", 12) == 0 || strncmp(input, "em\n", 3) == 0)
    {
        node->monitoring = 0;
        printf("\n[INFO]: Message monitoring deactivated.\n\n");
        return 0;
    }

    size_t input_len = strlen(input);
    if (input_len > 0)
        input[input_len - 1] = '\0';
    if (input_len == 1)
        return 0;

    printf("\n[ERROR]: Invalid command \"%s\". Available commands:\n", input);
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
            fprintf(stderr, "\r[ERROR]: Wrong NEIGHBOR message format\n");
            strcpy(server_message, "\0");
            return server_message;
        }

        if(node->monitoring) printf("\r[INFO]: NEIGHBOR message received from node %02d: %s\n", id, message);
        if(node->monitoring) printf("\r[INFO]: New connection to NODE %02d\n\n", id);

        add_int(&node->int_list, id, client_sockfd);
        node->n_int++;

        sync_new_neighbor_routes(node, id);
        
        sprintf(server_message, "OK\n");
    }
    else if (strncmp(message, "ROUTE ", 6) == 0 && client_sockfd != -1)
    {
        int dest, dist;
        char *token = strchr(message, ' ');
        if(sscanf(token + 1, "%d %d", &dest, &dist) != 2)
        {
            fprintf(stderr, "\r[ERROR]: Wrong ROUTE message format\n");
            strcpy(server_message, "\0");
            return server_message;
        }
        if(node->monitoring) printf("\r[INFO]: ROUTE message received from node %02d: %s\n", origem, message);
        
        if (dest == node->self_id)
        {
            strcpy(server_message, "OK\n");
            return server_message;
        }

        t_routing *r = get_rout(node->rout_list, dest);

        if(r == NULL)
        {
            add_rout(&node->rout_list, dest, origem);
            r = get_rout(node->rout_list, dest);
            r->dist = dist + 1;
            r->succ = origem;
        }
        else
        {
            if(dist+1 < findDist(node->rout_list, dest))
            {
                r->dist = dist + 1;
                r->succ = origem;
            }
            else
            {
                strcpy(server_message, "OK\n");
                return server_message;
            }
        }
        
        if(r->state == 0)
        {
            for (t_int *cur = node->int_list; cur != NULL; cur = cur->next_int) {
                char msg[64];
                sprintf(msg, "ROUTE %02d %d\n", dest, dist + 1);
                if (send_tcp_message(cur->int_tcp_sockfd, msg) != 0)
                {
                    strcpy(server_message, "\0");
                    return server_message;
                }
                if(node->monitoring) printf("\r[INFO]: ROUTE message sent to NODE %02d: %s\n", cur->int_id, msg);                    
            }
        }

        sprintf(server_message, "OK\n");
    }
    else if (strncmp(message, "COORD ", 6) == 0)
    {
        int dest;
        if (sscanf(message, "COORD %d", &dest) != 1)
        {
            strcpy(server_message, "\0");
            return server_message;
        }
        if(node->monitoring) printf("\r[INFO]: COORD message received from node %02d: %s\n", origem, message);

        t_routing *r = get_rout(node->rout_list, dest);
        if (r == NULL)
        {
            add_rout(&node->rout_list, dest, -1);
            r = get_rout(node->rout_list, dest);
            r->dist = INF;
            r->succ = -1;
            r->state = 0;
        }

        if (r->state == 1)
        {
            int sock = findIntSock(node->int_list, origem);
            if (sock != -1)
            {
                char msg[64];
                sprintf(msg, "UNCOORD %02d\n", dest);
                if(send_tcp_message(sock, msg) != 0)
                {
                    strcpy(server_message, "\0");
                    return server_message;
                }
                if(node->monitoring) printf("\r[INFO]: UNCOORD message sent to NODE %02d: %s\n", origem, msg);
            }
        }
        else if (r->state == 0 && origem != r->succ)
        {
            int sock = findIntSock(node->int_list, origem);
            if (sock != -1)
            {
                if (r->dist < INF)
                {
                    char msg1[64];
                    sprintf(msg1, "ROUTE %02d %d\n", dest, r->dist);
                    if(send_tcp_message(sock, msg1) != 0)
                    {
                        strcpy(server_message, "\0");
                        return server_message;
                    }
                    if(node->monitoring) printf("\r[INFO]: ROUTE message sent to NODE %02d: %s\n", origem, msg1);
                }

                char msg2[64];
                sprintf(msg2, "UNCOORD %02d\n", dest);
                if(send_tcp_message(sock, msg2) != 0)
                {
                    strcpy(server_message, "\0");
                    return server_message;
                }
                if(node->monitoring) printf("\r[INFO]: UNCOORD message sent to NODE %02d: %s\n", origem, msg2);
            }
        }
        else if (r->state == 0 && origem == r->succ)
        {
            if (enter_coordination(node, dest, r->succ) != 0)
            {
                strcpy(server_message, "\0");
                return server_message;
            }
        }
    }
    else if (strncmp(message, "UNCOORD ", 8) == 0)
    {
        int dest;
        if (sscanf(message, "UNCOORD %d", &dest) != 1)
        {
            strcpy(server_message, "\0");
            return server_message;
        }
        if(node->monitoring) printf("\r[INFO]: UNCOORD message received from node %02d: %s\n", origem, message);

        t_routing *r = get_rout(node->rout_list, dest);
        if (r == NULL) return server_message;

        if (r->state == 1)
            set_coord_state(r, origem, 0);

        if (r->state == 1 && all_coords_zero(r))
        {
            r->state = 0;

            if (r->dist < INF)
            {
                for (t_int *cur = node->int_list; cur != NULL; cur = cur->next_int)
                {
                    char msg[64];
                    sprintf(msg, "ROUTE %02d %d\n", dest, r->dist);
                    if(send_tcp_message(cur->int_tcp_sockfd, msg) != 0)
                    {
                        strcpy(server_message, "\0");
                        return server_message;
                    }
                    if(node->monitoring) printf("\r[INFO]: ROUTE message sent to NODE %02d: %s\n", cur->int_id, msg);
                }
            }

            if (r->succ_coord != -1)
            {
                int sock = findIntSock(node->int_list, r->succ_coord);
                if (sock != -1)
                {
                    char msg[64];
                    sprintf(msg, "UNCOORD %02d\n", dest);
                    if(send_tcp_message(sock, msg) != 0)
                    {
                        strcpy(server_message, "\0");
                        return server_message;
                    }
                    if(node->monitoring) printf("\r[INFO]: UNCOORD message sent to NODE %02d: %s\n", r->succ_coord, msg);
                }
                r->succ_coord = -1;
            }
        }
    }
    else if (strncmp(message, "CHAT ", 5) == 0 && client_sockfd != -1)
    {
        int origin, dest;
        char chat[129];

        if (sscanf(message, "CHAT %d %d %128[^\n]", &origin, &dest, chat) != 3)
        {
            fprintf(stderr, "\r[ERROR]: Wrong CHAT message format\n");
            strcpy(server_message, "\0");
            return server_message;
        }

        if(node->monitoring) printf("\r[INFO]: CHAT message received from node %02d: %s\n", origem, message);

        if (dest == node->self_id)
        {
            printf("\r[CHAT %02d -> %02d]: %s\n\n", origin, dest, chat);
        }
        else
        {
            if (send_chat_message(node, origin, dest, chat) != 0)
            {
                strcpy(server_message, "\0");
                return server_message;
            }
        }

        sprintf(server_message, "OK\n");
    }
    else
    {
        printf("\r[ERROR]: Special message not processed: %s\n\n", message);
        strcpy(server_message, "\0");
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
    if(node->monitoring) printf("\r[INFO]: Edge with NODE %02d disconnected.\n\n", int_id);

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

    if(handle_lost_neighbor_for_routes(node, int_id) != 0)
        return -1;

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
            char *server_response = process_tcp_message(node, token, intsock, int_id);
            if (strcmp(server_response, "\0") == 0)
                return -1;
            else if (strcmp(server_response, "OK\n") == 0)
            {
                token = strtok(NULL, "\n");
                continue;
            }
            int snd = send_tcp_message(intsock, server_response);
            if (snd == -1)
                return -1;
            token = strtok(NULL, "\n");
        }
        return 0; // process other messages
    }
}