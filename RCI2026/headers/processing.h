#ifndef PROCESSING_H
#define PROCESSING_H

#include "structs.h"

int enter_coordination(t_nodeinfo *node, int dest, int caused_by);
int sync_new_neighbor_routes(t_nodeinfo *node, int neigh_id);
int handle_lost_neighbor_for_routes(t_nodeinfo *node, int lost_id);

int process_join_command(t_nodeinfo * node, int net, int id);
int process_direct_join_command(t_nodeinfo *node, int net, int id);
int process_n_command(t_nodeinfo *node, unsigned int net);
int process_leave_command(t_nodeinfo * node);
int process_a_command(t_nodeinfo * node, int dist);
int process_ae_command(t_nodeinfo *node, int id);
int process_direct_ae_command(t_nodeinfo *node, int id, char idIP[], int idTCP);
int process_re_command(t_nodeinfo *node, int id);
int send_chat_message(t_nodeinfo *node, int origin, int dest, const char *chat);

int process_user_input(t_nodeinfo * node);
char *process_tcp_message(t_nodeinfo *node, char *message, int client_sockfd, int origem);
int process_intern_disconnection(t_nodeinfo *node, unsigned int int_id);
int process_intern_connection(t_nodeinfo *node, unsigned int int_id);

#endif