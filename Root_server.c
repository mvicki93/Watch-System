
#include "Root_server.h"
#define DEBUG 0
/*structure declareaton*/
System_info_t System_info;

/* msg_queue buffer */
my_msg_queue my_msgbuf;

/*lock msg_queues */
sem_t sem_name;

/*
 * *********************************************************************************************************
 * Desc 	: This is main function for server module.
 * 		  Thread1(T1) -> Will communicate with client.
 * 		  Thread2(T2) -> Get the data from message queue and maintain the list and update to the file.
		  Thread3(T3)-> Say three threads(T1,T2,T3) allocated for Data reading from one device and writing to anotherdevice.
                             T1 - Gets the data from device and stores those data in one message_queue say M1.
                             T2 - Monitors the operation of T1 and instructs it to load the data to another message_queue for ever 30s say M2.
                                  And it also alerts T3 to read the data stored in message_queue M1 and fetch it to the aother device.
                             T3 - This reads the data from message_queue M1 and make the list and update file.

                  NOTE:

                             If T3 does not reads the data exactly or if there is any data collapse in between T2 instructs T3 to read again from the same memory location.
                  Now:
                             When T1 fills the data in message_queue M2 again T2 instructs T1 to fill data in message_queue M1 and Instructs
                             T3 to read the data from M2 and make the list and update to the file.
                             This operation is being repeated for every 30S.this operation happens which memory contains data.
                       
  
 *
 * *********************************************************************************************************
 * 
 */

char cmd[][20] = {"show",  /* currently supported -show the all system info*/ 
                  "cp",    /* future - copy the file to machine */
                  "del",   /* future - del the remote file */
                  "apphost"/* future - host the application to remote*/
                 };

int main()
{
	void *make_list_status, *Thread_handler_status, *comms_client_status;
	/* Thread id */
	pthread_t client_communication_pid, Make_list_pid, Thread_handler_pid;	
	/* Thread 2 RCV message_queue starting status */
	msg_queueRCV_status = MSG_QUEUE_UNKNOWN;
        
        char cli[20];

	/* create message queue */
	if (!create_queue())
            return -1;

	/* semaphore initialization */
	if (!init_semaphore())
            return -1;


	/* To commucicate with client server and push the data to message queue */
	if (pthread_create(&client_communication_pid, NULL, client_communication, NULL)) {
		printf("client commnunication thread create fail\n");	
                return 1;
        }
	
	/*
         * Get the data from message queue and create the sorted list, once all the messages are read from the queue,
         * the list will be written to a file once the message queue is filled with incoming messages(with or without duplicate),
         * it will be compared  with the previous contents of the list it will be sorted(desecending order), 
         * duplicate files are removed(even duplicate msgs are compared with previous msg regarding it's MEMAVAILABLE, 
         * if both are not same replace the old one will be replaced with new one), 
	 * new msg will be added.
         */

	 if (pthread_create(&Make_list_pid, NULL, make_list, NULL)) {
                printf("make list thread create fail\n");
                return 1 ;
         }
			
	/*
         * This function only decieds which message_queue id is use to T1(push client infrm) and
         * which message_queue id is used to T2(receive and make the list and update to the file)
         */

	 if (pthread_create(&Thread_handler_pid, NULL, thread_handling_fn, NULL)) {
                printf("thread handler  create fail\n");
                return 1;
         }

         while (1) {
             /* user command line*/
             fputs("monitor:", stdout);
             if(fgets (cli, 20, stdin) != NULL ) {
                if (strcmp(cli, cmd[0]) == 0)
                   display();
             }
        }
//	thread_handling_fn();
        /* currently not handline thrad status */
        #if 0
	pthread_join(Thread_handler_pid, &Thread_handler_status);
	pthread_join(client_communication_pid, &comms_client_status);
	pthread_join(Make_list_pid, &make_list_status);
        #endif
}

int init_semaphore()
{
	 if (sem_init(&sem_name, 0, 0) == -1) {
		perror("sem open fail\n");
                return 0;
         }
}

int create_queue()
{
	
	//create queue
	msg_queue_id[MSG_QUEUE0] = msgget(key1, IPC_CREAT | IPC_EXCL | 0666);
	if (msg_queue_id[MSG_QUEUE0] == -1) {
		perror(" ");
                return 0;
        }

	msg_queue_id[MSG_QUEUE1] = msgget(key2, IPC_CREAT | IPC_EXCL | 0666);
        if (msg_queue_id[MSG_QUEUE1] == -1) {
                perror(" ");
                return 0;
        }
	/* message number */
	my_msgbuf.mtype = 1;
	/* starting message_queue id use to store the client information*/
	MSG_QUEUE_CMN_ID = msg_queue_id[MSG_QUEUE0];

}

void * make_list()
{

    while(1) {
        printf("before sem wait\n");
	sem_wait(&sem_name);
	msg_queueRCV_status =  MSG_QUEUE_PROCESS;
	while(1) {
            #if DEBUG
	    printf("message is there\n");
            #endif
	    int flag  = 1;
	    if(msg == NULL) {
	        msg = malloc(sizeof(*msg));
	        if(msg == NULL) {
	            printf("malloc fail\n");
                    return 0;
                }
	        // tail = msg;
	        if (msgrcv(MSG_QUEUE_RCV_ID, msg, 
                   sizeof(*msg) - sizeof(long), 1, IPC_NOWAIT) < 0) {
	            perror("msgrcv1");
		    //msg_queueRCV_status = MSG_QUEUE_EMPTY;
		    //break;
                    exit(1); // instead of exit have to handle thread deletion later(applicaple all exit(1))
	        }					

	    }else {
	        my_msg_queue *temp, *tail1, *prev;
		temp = malloc(sizeof *temp);
		if(temp == NULL) {
		    printf("malloc fail\n");
                    exit(1);
                }			
		if (msgrcv(MSG_QUEUE_RCV_ID, temp, sizeof(*temp) - sizeof(long), 1, IPC_NOWAIT) < 0) {
		    perror("msgrcv2");
		    msg_queueRCV_status = MSG_QUEUE_EMPTY;
		    write_2_file();
		    break;
		}
		temp -> next = NULL;
		tail1 = msg;
		prev = tail1 ;

		if (!((msg -> next == NULL) && 
                    ((strcmp(msg -> pc_info.network_info.hw_addr, temp -> pc_info.network_info.hw_addr) == 0)))){
		    /* to check already this unique id is there or not */
		    check_duplicate(msg, temp -> pc_info.network_info.hw_addr,
                            temp -> pc_info.mem_details.Memavailable);
		    /*sort the list */
		    if (tail1 -> pc_info.mem_details.Memavailable < 
                            temp -> pc_info.mem_details.Memavailable) {
		        temp -> next = tail1;
			msg = temp;
			tail1 = temp;
			flag = 0;
		    } else {

		       while (tail1 != NULL) {
		           if (tail1 -> pc_info.mem_details.Memavailable < 
                                 temp -> pc_info.mem_details.Memavailable) {
			       temp -> next = tail1;
			       prev -> next = temp;
			       flag = 0;
			       break;
			   } else {
			       prev = tail1;
			       tail1 = tail1 -> next;
			   }
	              }	
		}
		if (flag != 0) 
	            prev -> next = temp;
                    #if DEBUG
		    display();	/* debug */
                    #endif
		}else {
		    free(msg);
		    msg = temp;
                    #if DEBUG
		    display(); /*debug*/
                    #endif

	        }

	    }

        } //end while
    } //end while

}

/*
 * To write the list to a file
 *
 * store the info to database permently
 */

int write_2_file()
{

	my_msg_queue *temp = msg;
	int fd = open("client_infrmtn.txt", O_CREAT|O_WRONLY|O_TRUNC, 0666);
	if (fd == -1)
		fprintf(stderr, "file open fail\n");

	while (temp -> next != NULL) {
		if (write(fd, &temp -> pc_info, sizeof temp -> pc_info) < 0)
				fprintf(stderr, "write fail\n");
		temp = temp -> next;
	}
	if (write(fd, &temp -> pc_info, sizeof temp -> pc_info) < 0)
                                fprintf(stderr, "write fail\n");

	close(fd);

}


int display()
{
        my_msg_queue *temp = msg;
        if (temp == NULL) {
            printf("currently info not available\n");
            return 0;
        }
        while (temp -> next != NULL) {
                printf("%d ->", temp -> pc_info.mem_details.Memavailable);
                temp = temp -> next;
        }
        printf("%d -> ", temp -> pc_info.mem_details.Memavailable);
}

/*
 * check duplicate(compare unique id (mac)) is there or not,
 * if duplicate is there compare current msg and
 * previous message,  both message are same 
 * it will be removed else it will be added to the list
 */
	
int check_duplicate(my_msg_queue *temp, char *uniq_addr, unsigned int Memavailable)
{
	
	my_msg_queue *prev = temp;
	while (temp -> next != NULL) {
	    if (strcmp(temp -> pc_info.network_info.hw_addr, uniq_addr) == 0) { 
	        if ((temp -> pc_info.mem_details.Memavailable != Memavailable)) {
		    prev -> next = temp -> next;
		    free(temp);
		}
				

	     } else {
	         prev = temp;
		 temp = temp -> next;
	     }
	}
	if (strcmp(temp -> pc_info.network_info.hw_addr, uniq_addr) == 0) {
	    if ((temp -> pc_info.mem_details.Memavailable != Memavailable)) {
			prev -> next = temp -> next;
	        free(temp);

	     }
	}
	
	return 0;

}
				
void *thread_handling_fn()
{
	if (signal(SIGALRM,(void *) managing_thread) ==  SIG_ERR)
		perror("signal fail\n");
	alarm(30);
	while(1)
	    usleep(10);
}

/* This function only decide which message_queue id is use to
 * T1(push client infrmt) and which message_queue id is used to
 * T2(receive and make list and update to the file)
 * handle thread :handle message_queues 
 */

int managing_thread(int signal_no)
{
    char wait_some_time_fr_Mqueue = 2, wait_some_tme_fr_RCVqueue = 2;

    printf("enter signal handler\n");
    if (signal_no == SIGALRM) {
	if (signal(SIGALRM, SIG_IGN) ==  SIG_ERR) {
	    perror("signal fail\n");
	}
	while (wait_some_time_fr_Mqueue) {
	    if (msgctl(MSG_QUEUE_CMN_ID, IPC_STAT, &buf) == -1)
	        perror("msgctl\n");
                #if DEBUG				
                printf("message count %d\n", (unsigned int)(buf.msg_qnum)); /* debug */
                #endif
		if ((unsigned int)(buf.msg_qnum) != 0) {
		    while(wait_some_tme_fr_RCVqueue) {
			if ((msg_queueRCV_status == MSG_QUEUE_EMPTY) || (msg_queueRCV_status == MSG_QUEUE_UNKNOWN)) {   
			    if (MSG_QUEUE_CMN_ID == msg_queue_id[MSG_QUEUE0]) {
			        MSG_QUEUE_CMN_ID = msg_queue_id[MSG_QUEUE1];
				MSG_QUEUE_RCV_ID = msg_queue_id[MSG_QUEUE0];
                                #if DEBUG
				printf("before sem post MSG_QUEUE_CMN_ID %d MSG_QUEUE_RCV_ID %d\n",
                                        MSG_QUEUE_CMN_ID,
                                        MSG_QUEUE_RCV_ID); /* bebug */
                                #endif 
				sem_post(&sem_name);
				wait_some_time_fr_Mqueue = 0;
				break;

			     } else if(MSG_QUEUE_CMN_ID == msg_queue_id[MSG_QUEUE1]) {
			        MSG_QUEUE_CMN_ID = msg_queue_id[MSG_QUEUE0];
				MSG_QUEUE_RCV_ID = msg_queue_id[MSG_QUEUE1];
                                #if DEBUG
				printf("1 before sem post MSG_QUEUE_CMN_ID %d MSG_QUEUE_RCV_ID %d\n",
                                        MSG_QUEUE_CMN_ID,
                                        MSG_QUEUE_RCV_ID); /* bebug */
                                #endif
				sem_post(&sem_name);
				wait_some_time_fr_Mqueue = 0;
				break;

			    }
			}else {
			    usleep(10);
			    wait_some_tme_fr_RCVqueue --;
			    continue;
			} 
		   } /* End loop2 */
	      } else {
	          usleep(10);
		  wait_some_time_fr_Mqueue --;
		  continue;
	      }

         }/*End while loop1 */

         if(signal(SIGALRM, (void *)managing_thread) ==  SIG_ERR)
            perror("signal fail\n");

         else
	    alarm(30);
    } else {
        printf("signal not match\n");
        if(signal(SIGALRM, (void *)managing_thread) ==  SIG_ERR)
	    perror("signal fail\n");
	else
	    alarm(30);
	return SIGNAL_NOT_MATCH;
    }

}


void* client_communication()
{
	int sockfd; /* socket */
	int portno; /* port to listen on */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int size, file_fd; /* message byte size */ 
	time_t rawtime;
	struct tm * timeinfo;
	/* 
	 * socket: create the parent socket 
	 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		perror("ERROR opening socket");

	/* setsockopt: Handy debugging trick that lets 
	 * us rerun the server immediately after we kill it; 
	 * otherwise we have to wait about 20 secs. 
	 * Eliminates "ERROR on binding: Address already in use" error. 
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
			(const void *)&optval , sizeof(int));

	/*
	 * build the server's Internet address
	 */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)ROOT_SERVER_PORT);
	if (bind(sockfd, (struct sockaddr *) &serveraddr,
				sizeof(serveraddr)) < 0)
		perror("ERROR on binding");

	clientlen = sizeof(clientaddr);
        while(1) {
		size = recvfrom(sockfd, &my_msgbuf.pc_info, sizeof my_msgbuf.pc_info , 0,
				(struct sockaddr *) &clientaddr, &clientlen);
		if (size < 0) {
			perror("ERROR in recvfrom");
			break;
		}
		
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL) {
			perror("ERROR on inet_ntoa\n");
			exit(EXIT_FAILURE);
		}
		printf("server received datagram from  (%s)\n", hostaddrp);
		
		/* send push the data to message queue */
		if (msgsnd(MSG_QUEUE_CMN_ID, &my_msgbuf, sizeof(my_msgbuf) - sizeof(long), IPC_NOWAIT) < 0) {
     			  printf ("%d, %ld\n", MSG_QUEUE_CMN_ID, my_msgbuf.mtype);
       			  perror("msgsnd");
        		  exit(EXIT_FAILURE);
    	}
    	
    	time (&rawtime);
		timeinfo = localtime(&rawtime);
		printf ( "Date & Time : %s\n", asctime(timeinfo));
		
	#if DEBUG	
		
		/* Debug*/	
		printf("Received bytes from client : %d \n", size);
		printf("CPU info	: %s\n", System_info.cpu_details.model);
		printf("CPU speed info	: %f\n", System_info.cpu_details.speed);
		printf("MAC Address		: %s\n", System_info.network_info.hw_addr);
	#endif 

	}	

	
	return 0;
}

