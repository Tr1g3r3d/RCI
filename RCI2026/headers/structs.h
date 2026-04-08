#ifndef STRUCTS_H
#define STRUCTS_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct t_coord
{
    int neigh_id;       
    int coord_state;    
    struct t_coord *next_coord;
} t_coord;

typedef struct t_routing
{
    int dest;
    int dist;
    int succ;
    int state;
    int succ_coord;
    t_coord *coord_list;
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

    int monitoring;

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
int findDist(t_routing *head, int dest);
// void update_rout(t_routing **head_ref, int origem, int dest, int dist);
void print_rout(t_routing * head);
void print_dest_rout(t_routing *head, int dest);

t_int * create_int(unsigned int intr_id, int intr_tcp_sockfd);
void add_int(t_int ** head_ref, unsigned int intr_id, int intr_tcp_sockfd);
void remove_int(t_int ** head_ref, unsigned int intr_id);
void clearIntList(t_int * head);
int findIntSock(t_int * head, unsigned int id);
void print_int(t_int * head);

t_routing *get_rout(t_routing *head, int dest);

t_coord *get_coord(t_coord *head, int neigh_id);
void set_coord_state(t_routing *r, int neigh_id, int state);
int get_coord_state(t_routing *r, int neigh_id);
void remove_coord_from_all_routes(t_routing *head, int neigh_id);
int all_coords_zero(t_routing *r);
void set_route_succ(t_routing *head, int dest, int succ);
void set_route_state(t_routing *head, int dest, int state);
void set_route_succ_coord(t_routing *head, int dest, int succ_coord);
void print_coord_table(t_nodeinfo *node);

#endif