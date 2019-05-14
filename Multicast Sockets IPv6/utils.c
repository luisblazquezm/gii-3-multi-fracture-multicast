#include <ctype.h>
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	} else if (argc != 3) {
		
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
    PORT                        Port where will listen in UDP\n\n\";

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
    
   
