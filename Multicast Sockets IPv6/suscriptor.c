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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <string.h>

#define IPV6_DIR_SIZE              80
#define ADDRNOTFOUND	           0xffffffff	/* value returned for unknown host */
#define RETRIES	                   5		    /* number of times to retry before givin up */
#define BUFFERSIZE	               1024	        /* maximum size of packets to be received */

#define DEFAULT_PORT              7878
#define DEFAULT_MULTICAST_GROUP   "ff15::33"
#define DEFAULT_INTERFACE         "l0"

// This is just used to debug with a file and be able to identify procs
//int proc_id = -1;

/**
  * utils.h
  */
int test_args(int argc, char *argv[]);
int invalid_option_msg(char * opt);
int short_help_msg(void);
int help_msg(void);

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
    struct sigaction sa_sigint;
		
	sa_sigint.sa_handler = SIGINT_hdlr;
	sa_sigint.sa_flags = 0;

	value = sigfillset(&sa_sigint.sa_mask);
    if (-1 == value){
        perror("\n\nregister_SIGINT: error in sigfillset()\n\n");
        return -1;
    }
    
    value = sigdelset(&sa_sigint.sa_mask, SIGINT);
    if (-1 == value){
        perror("\n\nregister_SIGINT: error in sigfillset()\n\n");
        return -1;
    }

	value = sigaction(SIGINT, &sa_sigint, NULL);
    if (-1 == value){
        perror("\n\nregister_SIGINT: error in sigaction()\n\n");
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
    //proc_id = atoi(argv[1]);
    strncpy(group, argv[1], sizeof(group));
    strncpy(interface, argv[2], sizeof(group));
    port = atoi(argv[3]);

    // Emtpy both structures
    memset(&ipv6mreq, 0, sizeof(ipv6mreq));
    memset(&subscriber_conf, 0, sizeof(subscriber_conf));

    // Fill structures for IPv6 connection
    // -- Binding structure
        subscriber_conf.sin6_family = AF_INET6;      // Family = socket IPv6
        subscriber_conf.sin6_port = htons(port);  // Port
        subscriber_conf.sin6_addr = in6addr_any;     // Address

    // -- Join group structure
        ipv6mreq.ipv6mr_interface = if_nametoindex(interface);         // Interface index
    
        // Fills structure address with multicast group address (its like doing strcpy)
        value = inet_pton(AF_INET6, group, &ipv6mreq.ipv6mr_multiaddr);
        if (-1 == value) {
            perror("\n\nmulticast_subscriber: error in inet_pton()\n\n");
            return -1;
        }  
    
    // Get socket
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (-1 == s) {
        perror("\n\nmulticast_subscriber: error in socket()\n\n");
        return -1;
    }

    // Enable multiple subscribers for that multicast group. Setting SO_REUSERADDR
    int enable_flag = 1;
    value = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable_flag, sizeof(enable_flag) );
    if (-1 == value){
        perror("\n\nmulticast_subscriber: error in setsockopt(enable subscribers)\n\n");
        return -1;
    }

    // Bind socket to our IPv6 address
    value = bind(s, (const struct sockaddr *) &subscriber_conf, sizeof(subscriber_conf) );
    if (-1 == value){
        perror("\n\nmulticast_subscriber: error in bind()\n\n");
        return -1;
    }

    // Send message to join multicast group
    value = setsockopt(s, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &ipv6mreq, sizeof(ipv6mreq) );
    if (-1 == value){
        perror("\n\nmulticast_subscriber: error in setsockopt(join)\n\n");
        return -1;
    }

    // Start receiving UPD messages/data
    while(1){
        receive_msg(s);
    }

    return 0;
}

void receive_msg(int s)
{
    int numbytes = -2;
    int addrlen = sizeof(struct sockaddr_in6);
    const void *value = NULL;

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
    numbytes = recvfrom(s, buf, BUFFERSIZE, 0, (struct sockaddr *)&servaddr_in_ipv6, &addrlen);
    if (numbytes == -1) {
        perror("receive_msg: read: error receiving data");
    } else if (numbytes > 0) {
        // This will store the data in the struct and the direction in IPv6 of the emitter 
        // for that multicast group
        value = inet_ntop(AF_INET6, &servaddr_in_ipv6, multicast_group_sender, addrlen); 
        if (NULL == value) {
            perror("\n\nmulticast_subscriber: error in inet_pton()\n\n");
        } 

        // Write the message received into file
        snprintf(temp_buf, sizeof(temp_buf), "New message from %s: %s\n", multicast_group_sender, buf);
        printmtof(temp_buf, debug_file);
    }
    
}

/***************************************
*  test_args                          *
***************************************
*                                     *
*  Comprueba que los argumentos       *
*  recibidos por CLI tienen el        *
*  formato apropiado.                 *
*                                     *
***************************************/
int test_args(int argc, char * argv[])
{
	
	// There might be necessary to do some checking on host's name
	if (argv == NULL) {
		return -1;
	} else if (argc != 4) {
		
		if (argc == 2 && !strcmp(argv[1], "--help")) {
			help_msg();
			return 1;
		} else {
			short_help_msg();
			return -1;
		}
	} 
	
	return 0;
}

/************************************
*  invalid_option_msg              *
************************************
*                                  *
*  Imprime un mensaje de error     *
************************************/
int invalid_option_msg(char * opt)
{
	char msg[] = "cliente: invalid option -- '%s'\n\
Try 'cliente --help' for more information.\n";

	return printf(msg, opt);
}

/************************************
*  short_help_msg                  *
************************************
*                                  *
*  Imprime un mensaje de ayuda     *
*  corto                           *
************************************/
int short_help_msg(void)
{

	char msg[] = "Usage: cliente <IPv6multicastDir> <INTERFACE> <PORT>\n\
Try 'cliente --help' for more information.\n";

	return printf("%s", msg);

}

/************************************
*  help_msg                        *
************************************
*                                  *
*  Imprime un mensaje de ayuda     *
************************************/
int help_msg(void)
{

	char msg[] = "Usage: cliente <IPv6multicastDir> <INTERFACE> <PORT>\n\n\
    IPv6multicastDir            IPv6 multicast direction,\n\
                                the direction of the group we want to join\n\n\
    INTERFACE                   Interface where we will launch the join message\n\
                                and we will receive multicast messages\n\n\
    PORT                        Port where will listen in UDP\n\n";

	return printf("%s", msg);
}

int printmtof(char *msg, char *filename)
{
    FILE *ptr = NULL;

    if (msg == NULL || filename == NULL){
        return -1;
    }
    
    if ((ptr = fopen(filename, "a")) == NULL) {
        perror("printmtof: could not open file");
        return -1;
    }
    
    if (1 != fwrite(msg, strlen(msg), 1, ptr)) {
        return -1;
    }
    
    if (1 != fwrite("\n", sizeof(char), 1, ptr)) {
        return -1;
    }
    
    if (fclose(ptr) == EOF) {
        return -1;
    }
}
   


