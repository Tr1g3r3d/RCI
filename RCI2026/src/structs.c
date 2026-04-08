#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../headers/structs.h"

#define MAX(x, y) (x > y ? x : y)
#define INF 999999

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

    node->int_list = NULL;
    node->n_int = 0;

    node->net = -1;

    node->monitoring = 0;

    return node;
}

/**
 * @brief Get the highest file descriptor of all the open sockets
 *
 * @param node Necessary information about the node
 * @return [ @b int ] Value of the highest file descriptor
 */
int get_maxfd(t_nodeinfo *node)
{
    int max_int = -1, mx = -1;
    if (node->n_int == 1)
    {
        mx = node->int_list->int_tcp_sockfd;
    }
    else
    {
        if (node->int_list != NULL)
        {
            for (t_int *current = node->int_list; current->next_int != NULL; current = current->next_int)
            {
                max_int = MAX(current->int_tcp_sockfd, current->next_int->int_tcp_sockfd);
                if (max_int > mx)
                    mx = max_int;
            }
        }
    }
    mx = MAX(mx, node->server_tcp_sockfd);
    return mx;
}

void printNode(t_nodeinfo *node)
{
    printf("Node details:\n");
    printf("\tNode IP address: %s\n", node->self_ipaddr);
    printf("\tNode TCP Server port: %s\n", node->self_port);
    printf("\tNode REG IP address: %s\n", node->reg_ipaddr);
    printf("\tNode REG port: %s\n", node->reg_port);
    if(node->self_id == -1) printf("\tNode ID: Undefined\n");
    else printf("\tNode ID: %02d\n", node->self_id);
    if(node->net == -1) printf("\tNode ID: Not in network\n");
    else printf("\tNode ID: %03d\n", node->net);
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

    if (node->int_list != NULL)
        clearIntList(node->int_list);
    node->int_list = NULL;
    node->n_int = 0;

    node->net = -1;
}

/**
 * @brief Closing of all the open sockets
 *
 * @param node Necessary information about the node
 */
void close_sockets(t_nodeinfo *node)
{
    if (node->int_list != NULL)
    {
        t_int *current = node->int_list;
        while (current != NULL)
        {
            if (current->int_tcp_sockfd != -1)
                close(current->int_tcp_sockfd);
            current = current->next_int;
        }
    }
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
    new_rout-> dist = 1;
    new_rout->succ = neighbour;
    new_rout->state = 0;
    new_rout->succ_coord = -1;
    new_rout->coord_list = NULL;
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
        if (curr->dest == id || curr->succ == id)
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
        t_coord *c = current->coord_list;
        while (c != NULL)
        {
            t_coord *next_c = c->next_coord;
            free(c);
            c = next_c;
        }
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
            return current->succ;
        current = current->next_rout;
    }
    return -1;
}

int findDist(t_routing *head, int dest)
{
    t_routing *current = head;
    while (current != NULL)
    {
        if (current->dest == dest)
            return current->dist;
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
    printf("           _____________________________\n");
    printf("          |         |         |         |\n");
    printf("          | Destino | Vizinho |Distância|\n");
    printf("          |_________|_________|_________|\n");
    if (head != NULL)
    {
        while (head != NULL)
        {
            printf("          |         |         |         |\n");
            printf("          |   %02d    |    %02d   |    %02d   |\n", head->dest, head->succ, head->dist);
            printf("          |_________|_________|_________\n");
            head = head->next_rout;
        }
        printf("\n");
    }
    else
    {
        printf("          |                            |\n");
        printf("          |          NO ROUTING        |\n");
        printf("          |____________________________|\n\n");
    }
}

void print_dest_rout(t_routing *head, int dest)
{
    printf("[INFO]: ==== EXPEDITION TABLE ====\n\n");
    printf("           _____________________________\n");
    printf("          |         |         |         |\n");
    printf("          | Destino | Vizinho |Distância|\n");
    printf("          |_________|_________|_________|\n");
    t_routing *current = head;
    while (current != NULL)
    {
        if (current->dest == dest)
        {
            printf("          |         |         |         |\n");
            printf("          |   %02d    |    %02d   |    %02d   |\n", current->dest, current->succ, (current->dist == INF) ? -1 : current->dist);
            printf("          |_________|_________|_________|\n\n");
            return;
        }
        current = current->next_rout;
    }
    printf("          |         |                   |\n");
    printf("          |   %02d    |     NO ROUTING    |\n", dest);
    printf("          |_________|___________________|\n\n");
}

/**
 * @brief Creates a new Internal object
 *
 * @param intr_id NODE ID of the internal node
 * @param intr_ipaddr NODE IP ADDRESS of the internal node
 * @param intr_port PORT of the internal node
 * @param intr_tcp_port TCP socket of the connection with the internal node
 * @return [ @b t_int* ] Pointer to the new object of the internal node
 */
t_int *create_int(unsigned int intr_id, int intr_tcp_sockfd)
{
    t_int *new_int = (t_int *)calloc(1, sizeof(t_int));
    new_int->int_id = intr_id;
    new_int->int_tcp_sockfd = intr_tcp_sockfd;
    new_int->next_int = NULL;
    return new_int;
}

/**
 * @brief Add a new internal node to the list
 *
 * @param head_ref Double pointer to the head of the list of internal nodes
 * @param intr_id NODE ID of the internal node
 * @param intr_ipaddr IP ADDRESS of the internal node
 * @param intr_port PORT of the internal node
 * @param intr_tcp_port TCP socket of the connection with the internal node
 */
void add_int(t_int **head_ref, unsigned int intr_id, int intr_tcp_sockfd)
{
    t_int *new_int = create_int(intr_id, intr_tcp_sockfd);
    new_int->next_int = *head_ref;
    *head_ref = new_int;
}

/**
 * @brief Removes an internal node from the list
 *
 * @param head_ref Double pointer for the head of the list of internal nodes
 * @param intr_id NODE ID of the internal node to remove
 */
void remove_int(t_int **head_ref, unsigned int intr_id)
{
    t_int *current = *head_ref;
    t_int *previous = NULL;
    while (current != NULL)
    {
        if (current->int_id == intr_id)
        {
            if (previous == NULL)
            {
                *head_ref = current->next_int;
            }
            else
            {
                previous->next_int = current->next_int;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next_int;
    }
    printf("\n[INFO]: No connection found with NODE %02u\n\n", intr_id);
}

/**
 * @brief Cleans and frees the list of internal nodes
 *
 * @param head Pointer to the head of the list
 */
void clearIntList(t_int *head)
{
    t_int *current = head;
    t_int *next;
    while (current != NULL)
    {
        next = current->next_int;
        free(current);
        current = next;
    }
}

/**
 * @brief Finds the TCP socket of an internal node
 *
 * @param head Pointer to the head of the list
 * @param id NODE ID of the internal node
 * @return [ @b int ] Value of the TCP socket if sucessfull, -1 otherwise
 */
int findIntSock(t_int *head, unsigned int id)
{
    t_int *current = head;
    while (current != NULL)
    {
        if (current->int_id == id)
            return current->int_tcp_sockfd;
        current = current->next_int;
    }
    return -1;
}

/**
 * @brief Prints the list of internal nodes to the stdout
 *
 * @param head Pointer to de head of the list
 */
void print_int(t_int *head)
{
    printf("\n[INFO]: ===== NEIGHBORS LIST ===== \n");
    if (head == NULL)
        printf("        No Neighbors\n");
    while (head != NULL)
    {
        printf("        Node  %02u\n", head->int_id);
        head = head->next_int;
    }
    printf("\n");
}

t_routing *get_rout(t_routing *head, int dest)
{
    while (head != NULL)
    {
        if (head->dest == dest) return head;
        head = head->next_rout;
    }
    return NULL;
}

t_coord *get_coord(t_coord *head, int neigh_id)
{
    while (head != NULL)
    {
        if (head->neigh_id == neigh_id) return head;
        head = head->next_coord;
    }
    return NULL;
}

void set_coord_state(t_routing *r, int neigh_id, int state)
{
    if (r == NULL) return;

    t_coord *c = get_coord(r->coord_list, neigh_id);
    if (c != NULL)
    {
        c->coord_state = state;
        return;
    }

    c = (t_coord *)calloc(1, sizeof(t_coord));
    c->neigh_id = neigh_id;
    c->coord_state = state;
    c->next_coord = r->coord_list;
    r->coord_list = c;
}

int get_coord_state(t_routing *r, int neigh_id)
{
    if (r == NULL) return 0;
    t_coord *c = get_coord(r->coord_list, neigh_id);
    return (c == NULL) ? 0 : c->coord_state;
}

int all_coords_zero(t_routing *r)
{
    if (r == NULL) return 1;

    for (t_coord *c = r->coord_list; c != NULL; c = c->next_coord)
    {
        if (c->coord_state != 0) return 0;
    }
    return 1;
}

void remove_coord_from_all_routes(t_routing *head, int neigh_id)
{
    while (head != NULL)
    {
        set_coord_state(head, neigh_id, 0);
        head = head->next_rout;
    }
}

void set_route_succ(t_routing *head, int dest, int succ)
{
    t_routing *r = get_rout(head, dest);
    if (r != NULL) r->succ = succ;
}

void set_route_state(t_routing *head, int dest, int state)
{
    t_routing *r = get_rout(head, dest);
    if (r != NULL) r->state = state;
}

void set_route_succ_coord(t_routing *head, int dest, int succ_coord)
{
    t_routing *r = get_rout(head, dest);
    if (r != NULL) r->succ_coord = succ_coord;
}

void print_coord_table(t_nodeinfo *node)
{
    printf("\n--- Coordination Table ---\n");

    if (node->rout_list == NULL)
    {
        printf("Tabela vazia.\n");
        return;
    }

    for (t_routing *r = node->rout_list; r != NULL; r = r->next_rout)
    {
        printf("Dest: %02d | State: %s | Succ: %02d | SuccCoord: %02d\n", r->dest, (r->state == 0) ? "EXP" : "COORD", r->succ, r->succ_coord);
        printf("   coord[t,j]: ");

        if (r->coord_list == NULL)
        {
            printf("(vazio)");
        }
        else
        {
            for (t_coord *c = r->coord_list; c != NULL; c = c->next_coord)
            {
                printf("[%02d:%d] ", c->neigh_id, c->coord_state);
            }
        }

        printf("\n");
    }

    printf("--------------------------\n\n");

    return;
}