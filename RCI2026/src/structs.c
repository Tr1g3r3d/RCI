#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../headers/structs.h"

/**
 * @brief Node inicialization
 *
 * @param ipaddr IP ADDRESS of the node
 * @param port PORT of the node
 * @param reg_ipaddr IP ADDRESS of the node server
 * @param reg_port PORT of the node server
 * @param server_sockfd Socket of the TCP server
 * @return [ @b t_nodeinfo* ] Pointer to the node
 */
t_nodeinfo *node_init(char *ipaddr, char *port, char *reg_ipaddr, char *reg_port, int server_sockfd)
{
    // alocação da memória do node
    t_nodeinfo *node = (t_nodeinfo *)calloc(1, sizeof(t_nodeinfo));

    // inicializa as variaveis
    node->self_id = -1;
    strcpy(node->self_ipaddr, ipaddr);
    strcpy(node->self_port, port);

    strcpy(node->reg_ipaddr, reg_ipaddr);
    strcpy(node->reg_port, reg_port);

    node->rout_list = NULL;
    node->n_rout = 0;

    node->server_tcp_sockfd = server_sockfd;

    node->net = -1;

    return node;
}

/**
 * @brief Refresh all the informtion in the node
 *
 * @param node Necessary information about the node
 */
void node_refresh(t_nodeinfo *node)
{
    // limpa e reinicia as variaveis
    node->self_id = -1;

    if (node->rout_list != NULL)
        clearRoutList(node->rout_list);
    node->rout_list = NULL;
    node->n_rout = 0;

    node->net = -1;
}

/**
 * @brief Creates a new rout object
 *
 * @param dest Target NODE ID to get to
 * @param neighbour Neighbour NODE ID to send to
 * @return [ @b t_routing* ] Pointer to the new object
 */
t_routing *create_rout(int dest, int neighbour)
{
    t_routing *new_rout = (t_routing *)calloc(1, sizeof(t_routing));
    new_rout->dest = dest;
    new_rout->neighbour = neighbour;
    new_rout->next_rout = NULL;
    return new_rout;
}

/**
 * @brief Adds a new rout to the rout list
 *
 * @param head_ref Double pointer to the head of the list
 * @param dest Targe NODE ID to get to
 * @param neighbour Neighbour NODE ID to send to
 */
void add_rout(t_routing **head_ref, int dest, int neighbour)
{
    t_routing *new_rout = create_rout(dest, neighbour);
    new_rout->next_rout = *head_ref;
    *head_ref = new_rout;
}

/**
 * @brief Removes a rout from the list
 *
 * @param head_ref Double pointer to the rout list
 * @param id NODE ID to remove from the from the list
 */
void remove_rout(t_routing **head_ref, int id)
{
    t_routing *prev = NULL;
    t_routing *curr = *head_ref;

    while (curr != NULL)
    {
        if (curr->dest == id || curr->neighbour == id)
        {
            if (prev == NULL)
            {
                *head_ref = curr->next_rout;
                free(curr);
                curr = *head_ref;
            }
            else
            {
                prev->next_rout = curr->next_rout;
                free(curr);
                curr = prev->next_rout;
            }
        }
        else
        {
            prev = curr;
            curr = curr->next_rout;
        }
    }
}

/**
 * @brief Clears and frees the rout list
 *
 * @param head Pointer to the of the list
 */
void clearRoutList(t_routing *head)
{
    t_routing *current = head;
    t_routing *next;
    while (current != NULL)
    {
        next = current->next_rout;
        free(current);
        current = next;
    }
}

/**
 * @brief Finds a rout in the list
 *
 * @param head Pointer to the head of the list
 * @param dest Target NODE ID
 * @return [ @b int ] NODE ID of the neighbour to send to if sucessfull, -1 otherwise
 */
int findRout(t_routing *head, int dest)
{
    t_routing *current = head;
    while (current != NULL)
    {
        if (current->dest == dest)
            return current->neighbour;
        current = current->next_rout;
    }
    return -1;
}

/**
 * @brief Prints the routing list to the stdout
 *
 * @param head Pointer to the of the list
 */
void print_rout(t_routing *head)
{
    printf("[INFO]: ==== EXPEDITION TABLE ====\n\n");
    printf("           ___________________\n");
    printf("          |         |         |\n");
    printf("          | Destino | Vizinho |\n");
    printf("          |_________|_________|\n");
    if (head != NULL)
    {
        while (head != NULL)
        {
            printf("          |         |         |\n");
            printf("          |   %02d    |    %02d   |\n", head->dest, head->neighbour);
            printf("          |_________|_________|\n");
            head = head->next_rout;
        }
        printf("\n");
    }
    else
    {
        printf("          |                   |\n");
        printf("          |     NO ROUTING    |\n");
        printf("          |___________________|\n\n");
    }
}