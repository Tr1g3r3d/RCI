#ifndef STRUCTS_H
#define STRUCTS_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct t_routing
{
    int dest;
    int neighbour;
    struct t_routing * next_rout;

}t_routing;

typedef struct t_int
{
    unsigned int int_id;
    int int_tcp_sockfd;
    struct t_int * next_int;

}t_int;

typedef struct t_nodeinfo
{
    int self_id;
    char self_ipaddr[INET_ADDRSTRLEN];
    char self_port[6];

    char reg_ipaddr[INET_ADDRSTRLEN];
    char reg_port[6];

    t_routing * rout_list;
    int n_rout;

    int server_tcp_sockfd;

    t_int * int_list;
    int n_int;

    int net;

}t_nodeinfo;

t_nodeinfo *node_init(char *ipaddr, char *port, char *reg_ipaddr, char *reg_port, int server_sockfd);
int get_maxfd(t_nodeinfo *node);
void printNode(t_nodeinfo *node);
void node_refresh(t_nodeinfo * node);
void close_sockets(t_nodeinfo *node);

t_routing * create_rout(int dest, int neighbour);
void add_rout(t_routing ** head_ref, int dest, int neighbour);
void remove_rout(t_routing ** head_ref, int id);
void clearRoutList(t_routing * head);
int findRout(t_routing * head, int dest);
void print_rout(t_routing * head);

t_int * create_int(unsigned int intr_id, int intr_tcp_sockfd);
void add_int(t_int ** head_ref, unsigned int intr_id, int intr_tcp_sockfd);
void remove_int(t_int ** head_ref, unsigned int intr_id);
void clearIntList(t_int * head);
int findIntSock(t_int * head, unsigned int id);
void print_int(t_int * head);

#endif