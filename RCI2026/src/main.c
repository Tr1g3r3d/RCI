#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../headers/utils.h"
#include "../headers/structs.h"
#include "../headers/udp_client.h"

int main(int argc, char *argv[])
{
    char input[100];
    //verifica os argumentos da consola
    if (process_invocation(argc, argv) != 0)
        exit(1);
    char *inputs[5] = {argv[0], argv[1], argv[2], (argc < 4) ? "193.136.138.142" : argv[3], (argc < 5) ? "59000" : argv[4]};
    
    /*
    
        Iniciar Client UDP
    
    */
    udp_client_init(inputs[3], inputs[4]);
   

    //criação do node
    t_nodeinfo *node = node_init(inputs[1], inputs[2], inputs[3], inputs[4], -1);
    if (node == NULL)
    {
        fprintf(stderr, "ERROR creating the node.\n");
        exit(1);
    }

    /*
    
        Iniciar Server TCP
    
    */
     

    exit(1);
}