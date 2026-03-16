#ifndef PROCESSING_H
#define PROCESSING_H

#include "structs.h"

int process_join_command(t_nodeinfo * node, int net, int id);
int process_n_command(t_nodeinfo *node, unsigned int net);
int process_leave_command(t_nodeinfo * node);
int process_ae_command(t_nodeinfo *node, int id);
int process_re_command(t_nodeinfo *node, int id);
int process_user_input(t_nodeinfo * node);
char *process_tcp_message(t_nodeinfo *node, char *message, int client_sockfd, int origem);
int process_intern_disconnection(t_nodeinfo *node, unsigned int int_id);
int process_intern_connection(t_nodeinfo *node, unsigned int int_id);

#endif