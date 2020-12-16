
#ifndef _CLIENT_H 
#define _CLIENT_H

/* Header files */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "common.h"
#include <errno.h>

enum info_type_t {CPU_INFRMTN, MEM_INFRMTN, OS_INFRMTN, PROC_INFRMTN};

/* Function declarations */
char *get_data(char *databuf, char *value);
int parse_data(char *buf, enum info_type_t);
int compare_and_store_data(char *data_caption, char *databuf, enum info_type_t type);
int get_cpu_info(void);
int get_endian(void);
int get_machine_arch(void);
int store_proc_info(char *databuf);
int set_proc_info(char (*ptr)[10]);
int remove_newline(char *temp);

int print_cpu_info(void);
int print_mem_info(void);
int print_os_info(void);
int print_network_info(void);
int print_proc_info(void);

/* Symbolic Literals */
#define BUF_MAX 120


#endif /* end of _CLIENT_H */

