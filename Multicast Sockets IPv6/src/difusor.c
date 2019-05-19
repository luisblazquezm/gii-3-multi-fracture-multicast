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
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <net/if.h>

#define IPV6_DIR_SIZE              80
#define ADDRNOTFOUND	           0xffffffff	/* value returned for unknown host */
#define RETRIES	                   5		    /* number of times to retry before givin up */
#define BUFFERSIZE	               1024	        /* maximum size of packets to be received */

#define DEFAULT_MESSAGE           "Holaquetal"
#define DEFAULT_PORT              5001
#define DEFAULT_MULTICAST_GROUP   "ff15::33"
#define DEFAULT_INTERFACE         "eth0"
#define DEFAULT_HOPS              15
#define DEFAULT_DELAY             10

/**
  * utils.h
  */
int test_args(int argc, char *argv[]);
int invalid_option_msg(char * opt);
int short_help_msg(void);
int help_msg(void);

int multicast_diffusor(int argc, char *argv[]);
int register_SIGINT();
void SIGINT_handler();
void send_msg(int s, char *msg, struct sockaddr_in6 diffusor_conf);

int printmtof(char *msg, char *filename);

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
    
    return multicast_diffusor(argc, argv);
}

int multicast_diffusor(int argc, char *argv[])
{
    int s, if_index;		                 /* socket descriptor */
    int value = 0;
    char group[IPV6_DIR_SIZE];
    char interface[IPV6_DIR_SIZE];  
    int port, hops, delay;
    char buf[BUFFERSIZE];

    //struct ipv6_mreq ipv6mreq;           /* IPv6 multicast request structure */
    struct sockaddr_in6 subscriber_conf, diffusor_conf; /* socket structure */

    // Fill argument data 
    value = test_args(argc, argv);
    if (-1 == value){ // Assign default values

        strncpy(buf, DEFAULT_MESSAGE, sizeof(DEFAULT_MESSAGE));
        strncpy(group, DEFAULT_MULTICAST_GROUP, sizeof(DEFAULT_MULTICAST_GROUP));
        strncpy(interface, DEFAULT_INTERFACE, sizeof(DEFAULT_INTERFACE));
        port = DEFAULT_PORT;
        hops = DEFAULT_HOPS;
        delay = DEFAULT_DELAY;

    } else {
        
        strncpy(buf, argv[1], sizeof(buf));
        strncpy(group, argv[2], sizeof(group));
        strncpy(interface, argv[3], sizeof(group));
        port = atoi(argv[4]);
        hops = atoi(argv[5]);
        delay = atoi(argv[6]);

    }

    // Emtpy both structures
    memset(&diffusor_conf, 0, sizeof(diffusor_conf));
    memset(&subscriber_conf, 0, sizeof(subscriber_conf));

    // Fill structures for IPv6 connection

        // -- Binding structure
        subscriber_conf.sin6_family = AF_INET6;      // Family = socket IPv6
        subscriber_conf.sin6_port = 0;               // Ephimeral Port
        subscriber_conf.sin6_addr = in6addr_any;     // Address

        diffusor_conf.sin6_family = AF_INET6;        // Family = socket IPv6
        diffusor_conf.sin6_port = htons(port);       // Port
    
        // Fills structure address with multicast group address (its like doing strcpy)
        value = inet_pton(AF_INET6, group, &diffusor_conf.sin6_addr);
        if (-1 == value) {
            perror("\n\nmulticast_diffusor: error in inet_pton()\n\n");
            return -1;
        }  
    
    // Get socket
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (-1 == s) {
        perror("\n\nmulticast_diffusor: error in socket()\n\n");
        return -1;
    }

    // Bind socket to our IPv6 address
    value = bind(s, (const struct sockaddr *) &subscriber_conf, sizeof(subscriber_conf));
    if (-1 == value){
        perror("\n\nmulticast_diffusor: error in bind()\n\n");
        return -1;
    }
    
    // Set interface and hops
    if_index = if_nametoindex(interface);
    value = setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&if_index, sizeof(if_index) );
    if (-1 == value){
        perror("\n\nmulticast_diffusor: error in setsockopt(enable subscribers)\n\n");
        return -1;
    }
    value = setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops));
    if (-1 == value){
        perror("\n\nmulticast_diffusor: error in setsockopt(join)\n\n");
        return -1;
    }

    // Start receiving UPD messages/data
    while(1){
        send_msg(s, buf, diffusor_conf);
        sleep(delay);
    }

    return 0;
}

void send_msg(int s, char *msg, struct sockaddr_in6 diffusor_conf)
{
    int value = -1;
    value = sendto(s, msg, strlen(msg), 0, (struct sockaddr *) &diffusor_conf, sizeof(diffusor_conf));
    if (-1 == value){
        perror("\n\nmulticast_diffusor: error in sendto(find_subscribers)\n\n");
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
    if (argv == NULL || argc != 7) {
        return -1;
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
	char msg[] = "difusor.c: invalid option -- '%s'\n\
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

	char msg[] = "Usage: difusor.c GROUP_ADDRESS INTERFACE PORT HOPS DELAY MESSAGE \n\
Try 'difusor.c --help' for more information.\n";

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

	char msg[] = "Usage: difusor.c GROUP_ADDRESS INTERFACE PORT HOPS DELAY MESSAGE\n\n\
    GROUP_ADDRESS               IPv6 multicast direction,\n\
                                the direction of the group we want to join\n\n\
    INTERFACE                   Interface where we will launch the join message\n\
                                and we will receive multicast messages\n\n\
    PORT                        Port where will listen in UDP\n\n\
    HOPS                        Max number of hops.\
    DELAY                       Delay between messages to avoid sending a new one\
                                before having received the previous.\
    MESSAGE                     Message sent to subscribers.";

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
   


