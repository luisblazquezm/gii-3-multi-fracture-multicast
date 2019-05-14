/*
** Fichero: suscriptor.c
** Autores:
** Luis Blázquez Miñambres DNI 70910465Q 
** Usuario: i0910465
** Samuel Gómez Sánchez DNI 445136357F
** Usuario: i45136357
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include "utils.h"

#define IPV6_DIR_SIZE              80
#define ADDRNOTFOUND	           0xffffffff	/* value returned for unknown host */
#define RETRIES	                   5		    /* number of times to retry before givin up */
#define BUFFERSIZE	               1024	        /* maximum size of packets to be received */

#define DEFAULT_PORT              7878
#define DEFAULT_MULTICAST_GROUP   "ff15::33"
#define DEFAULT_INTERFACE         "l0"

extern int errno;

int multicast_subscriber(int argc, char *argv[]);
int register_SIGINT();
void SIGINT_handler();
void receive_msg(int s);

void SIGINT_hdlr()
{
 printf("Terminated\n\n");
 exit(1);
}

int register_SIGINT()
{
    int value = -5;
    sigset_t sigset;
    struct sigaction sa_sigint;

	sigfillset(&sigset);
	sigdelset(&sigset, SIGINT);
		
	sa_sigint.sa_handler = SIGINT_hdlr;
	sa_sigint.sa_flags = 0;

	value = sigfillset(&sa_sigint.sa_mask);
    if (-1 == value){
        perror(stderr, "\n\nregister_SIGINT: error in sigfillset()\n\n");
        return -1;
    }

	value = sigaction(SIGINT, &sa_sigint, NULL);
    if (-1 == value){
        perror(stderr, "\n\nregister_SIGINT: error in sigaction()\n\n");
        return -1;
    }

    return 0;
}


int main(int argc, char *argv[])
{   
    int value = -5;
    value = test_args(argc, argv);

    if (-1 == value || 1 == value) 
        return -1;

    if (-1 == register_SIGINT()) 
        return -1;
    
    return multicast_subscriber(argc, argv);
}

int multicast_subscriber(int argc, char *argv[])
{
	int s;				                 /* socket descriptor */
    int value = 0;
    char group[IPV6_DIR_SIZE];
    char interface[IPV6_DIR_SIZE];  
    int port;

    struct ipv6_mreq ipv6mreq;           /* IPv6 multicast request structure */
    struct sockaddr_in6 subscriber_conf; /* socket structure */

    // Fill argument data 
    value = strcpy(group, argv[1]);
    if (-1 == value){
        perror(stderr, "\n\nmulticast_subscriber: error in strcpy(group)\n\n");
        return -1;
    }

    value = strcpy(interface, argv[2]);
    if (-1 == value){
        perror(stderr, "\n\nmulticast_subscriber: error in strcpy(interface)\n\n");
        return -1;
    }

    port = atoi(argv[3]);

    // Emtpy both structures
    memset(&ipv6mreq, 0, sizeof(ipv6mreq));
    memset(&subscriber_data, 0, sizeof(subscriber_conf));

    // Fill structures for IPv6 connection
    // -- Binding structure
        subscriber_conf.sin6_family = AF_INET6;      // Family = socket IPv6
        subscriber_conf.sin6_port = htons(group);  // Port
        subscriber_conf.sin6_addr = in6addr_any;     // Address

    // -- Join group structure
        ipv6mreq.ipv6mr_interface = if_nametoindex(interface);         // Interface index
    
        // Fills structure address with multicast group address (its like doing strcpy)
        value = inet_pton(AF_INET6, group, &ipv6mreq.ipv6mr_multiaddr);
        if (-1 == value) {
            perror(stderr, "\n\nmulticast_subscriber: error in inet_pton()\n\n");
            return -1;
        }  
    
    // Get socket
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (-1 == s) {
        perror(stderr, "\n\nmulticast_subscriber: error in socket()\n\n");
        return -1;
    }

    // Enable multiple subscribers for that multicast group. Setting SO_REUSERADDR
    value = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable) );
    if (-1 == value){
        perror(stderr, "\n\nmulticast_subscriber: error in setsockopt(enable subscribers)\n\n");
        return -1;
    }

    // Bind socket to our IPv6 address
    value = bind(s, (const struct sockaddr *) &subscriber_conf, sizeof(subscriber_conf) );
    if (-1 == value){
        perror(stderr, "\n\nmulticast_subscriber: error in bind()\n\n");
        return -1;
    }

    // Send message to join multicast group
    value = setsockopt(s, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &ipv6mreq, sizeof(ipv6mreq) );
    if (-1 == value){
        perror(stderr, "\n\nmulticast_subscriber: error in setsockopt(join)\n\n");
        return -1;
    }

    // Start receiving UPD messages/data
    while(1){
        value = receive_msg(s);
        if (-1 == value){
            perror(stderr, "\n\nmulticast_subscriber: error in receive_msg\n\n");
            return -1;
        }
    }

    return 0;
}

void receive_msg(int s)
{
    int numbytes = -2;
    int addrlen = sizeof(struct sockaddr_in6);
    int value = -2;

    char debug_file[] = "reception_debug.txt";
    char multicast_group_sender[IPV6_DIR_SIZE];
    char buf[BUFFERSIZE];
    char temp_buf[100];

    struct sockaddr_in6 servaddr_in_ipv6;

    // Empty structures and message content every time we receive a message
    memset(buf, 0 , BUFFERSIZE * sizeof(char));
	memset(multicast_group_sender, 0 , IPV6_DIR_SIZE * sizeof(char));
	memset(&servaddr_in_ipv6, 0 , sizeof(servaddr_in_ipv6));

    // Receive data in buffer
    numbytes = recvfrom(s, buf, BUFFERSIZE, 0, (struct sockaddr *)&servaddr_in_ipv6, &addr_len);
    if (numbytes == -1) {
        perror("receive_msg: read: error receiving data");
        return -1;
    } else if (numbytes > 0) {
        // This will store the data in the struct and the direction in IPv6 of the emitter 
        // for that multicast group
        value = inet_ntop(AF_INET6, &servaddr_in_ipv6, multicast_group_sender, addrlen); 
        if (-1 == value) {
            perror(stderr, "\n\nmulticast_subscriber: error in inet_pton()\n\n");
            return -1;
        } 

        // Write the message received into file
        snprintf(temp_buf, sizeof(temp_buf), "New message from %s: %s\n", multicast_group_sender, buf);
        printmtof(temp_buf, debug_file);
    }
    
}


