
#ifndef _ROOT_SERVER_H_
#define _ROOT_SERVER_H_



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
#include <semaphore.h>
#include <signal.h>
#include <errno.h>

#include "common.h"

/* function declaration */
void *client_communication();
void *store_clnt_info();
int create_queue();
//void root_server_infrmtn();
void *thread_handling_fn();
int managing_thread(int signal_no);
int init_semaphore();
void *make_list();



/*global variable*/
int msg_queue_id[2], root_fd, MSG_QUEUE_CMN_ID;
int MSG_QUEUE_RCV_ID, msg_queueRCV_status;
struct sockaddr_in server;
struct msqid_ds buf;

/* Symbolic Literals */

#define BUFSIZE         1024
#define ROOT_SERVER_IP		"172.17.1.146"
#define ROOT_SERVER_PORT	7001
#define DC_PORT			2000
#define BUFSIZE 1024
#define ack "1"

#define MSG_QUEUE0 0
#define MSG_QUEUE1 1
#define MSG_QUEUE_EMPTY 2
#define MSG_QUEUE_NT_EMPTY 3
#define MSG_QUEUE_UNKNOWN 4
#define MSG_QUEUE_PROCESS 5
#define SIGNAL_NOT_MATCH 0x9

/** msg_queue key **/
#define key1 90000
#define key2 10000


/*Queue structure*/
#if 0
typedef struct mymsg {
      long      mtype;    /* message type */
      struct mymsg *next;
      System_info_t pc_info; /* message text of length MSGSZ */

}my_msg_queue;
#endif

my_msg_queue *msg = NULL;
int check_duplicate(my_msg_queue *temp, char *uniq_addr, unsigned int Memavailable);
#endif /* end of _ROOT_SERVER_H_ */

