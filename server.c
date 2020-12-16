#include "server.h"
/*structure declareaton*/
System_info_t System_info_cpu;
/* lock msg_queues */
sem_t sem_name;
/* msg_queue buffer */
my_msg_queue my_msgbuf;

/*
 * *********************************************************************************************************
 * Desc 	: This is main function for server module.
 * 		  Thread1 -> Will communicate with client.
 * 		  Thread2 -> Get client information and sends to the root server.
		  Thread3 -> Say three threads(T1,T2,T3) allocated for Data reading from one device and writing to anotherdevice.
                             T1 - Gets the data from device and stores those data in one message_queue say M1.
                             T2 - Monitors the operation of T1 and instructs it to load the data to another message_queue for ever 30s say M2.
                                  And it also alerts T3 to read the data stored in message_queue M1 and fetch it to the aother device.
                             T3 - This reads the data from message_queue M1 and fetches it to another device.

                  NOTE:

                             If T3 does not reads the data exactly or if there is any data collapse in between
                             T2 instructs T3 to read again from the same memory location.
                  Now:
                             When T1 fills the data in message_queue M2 again T2 instructs T1 to fill data in
                             message_queue M1 and Instructs T3 to read the data from M2 and fetch the data to another device.
                             This operation is being repeated for every 30S.this operation happens which memory contains data.
                       
  
 *
 * *********************************************************************************************************
 * 
 */	

int main(int argc, char **argv) 
{
	pthread_t comms_client_pid, Snd_To_RootServer_pid, Thread_handler_pid;
	void *comms_client_status, *Snd_To_RootServer_status, *Thread_handler_status;
	
	/* Thread 2 RCV message_queue starting status */
	msg_queueRCV_status = MSG_QUEUE_UNKNOWN;
	/* create message queue */
	create_queue();
	/* set server details */
	root_server_infrmtn();
	/* semaphore initialization */
	init_semaphore();

	/* commucicate with client and push the data to message queue */
	if (pthread_create(&comms_client_pid, NULL, client_communication, NULL))
		printf("thread create fail\n");	
	

	/* Read the data from messgage queue and send the data to root server */
	if (pthread_create(&Snd_To_RootServer_pid, NULL, store_clnt_info, NULL))
                printf("thread create fail\n");

	/* This function only decieds which message_queue id is use to T1(push client infrm) and which 
         * message_queue id is used to T2(receive and snd data to root server) 
         */
	if (pthread_create(&Thread_handler_pid, NULL, thread_handling_fn, NULL))
                printf("thread create fail\n");

	
	/* Waiting for thread exit status */
	pthread_join(Thread_handler_pid, &Thread_handler_status);
	pthread_join(comms_client_pid, &comms_client_status);
	pthread_join(Snd_To_RootServer_pid, &Snd_To_RootServer_status);
	


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
	if (msg_queue_id[MSG_QUEUE0] == -1)
		perror(" ");

	msg_queue_id[MSG_QUEUE1] = msgget(key2, IPC_CREAT | IPC_EXCL | 0666);
        if (msg_queue_id[MSG_QUEUE1] == -1)
                perror(" ");

	/* message number */
	my_msgbuf.mtype = 1;
	/* starting message_queue id use to store the client information*/
	MSG_QUEUE_CMN_ID = msg_queue_id[MSG_QUEUE0];

}


/*
 * This function used to recv message from queue and send data to the root server
 */
void *store_clnt_info()
{

	struct mymsg *rbuf;

        rbuf = malloc(sizeof (*rbuf));
	if (rbuf ==  NULL) {
		printf("malloc fail store_clnt_info\n");
		exit(1);
	}

	while(1) {
		printf("before sem wait\n");
		sem_wait(&sem_name);
		msg_queueRCV_status =  MSG_QUEUE_PROCESS;
		while (1) {
                        #if DEBUG			
			printf("message recv from queue %d\n", MSG_QUEUE_RCV_ID); /* debug */
                        #endif
			/* recv message from queue */
			if (msgrcv(MSG_QUEUE_RCV_ID, rbuf, sizeof(*rbuf) - sizeof(long), 1, IPC_NOWAIT) < 0) {
        			perror("msgrcv");
				msg_queueRCV_status = MSG_QUEUE_EMPTY;
				break;
    			}
			/* send data to root server */
			if (sendto(root_fd, rbuf, sizeof(my_msgbuf) - sizeof(long), 0, 
                            (struct sockaddr *)&server, sizeof server) == -1) {
        	        	   perror("sendto");
         		}
                #if DEBUG
        	printf("data send %ld\n", sizeof(my_msgbuf) - sizeof(long)); /* debug */
                #endif
		memset(rbuf, 0, sizeof(*rbuf));
		}
	}
}

/* Set root server infrmtn */
void root_server_infrmtn()
{
	/* Socket : create socket */
        if ((root_fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
                printf("socket created\n");

        memset((char *) &server, 0, sizeof(server));
        //server 
        server.sin_family = AF_INET;
        server.sin_port = htons(ROOT_SERVER_PORT);
        server.sin_addr.s_addr=inet_addr(ROOT_SERVER_IP);


}

void *thread_handling_fn()
{
	if (signal(SIGALRM,(void *) managing_thread) ==  SIG_ERR)
		perror("signal fail\n");
	alarm(30);
	while(1);
}

/*
 *  This function only decieds which message_queue id is use to
 *  T1(push client infrmt) and which message_queue id is used to T2(receive and snd data to root server)
 *  handle thread :handle message_queues
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
                                            MSG_QUEUE_CMN_ID, MSG_QUEUE_RCV_ID); /* bebug */
                                        #endif 
					sem_post(&sem_name);
					wait_some_time_fr_Mqueue = 0;
					break;

				     } else if(MSG_QUEUE_CMN_ID == msg_queue_id[MSG_QUEUE1]) {
					MSG_QUEUE_CMN_ID = msg_queue_id[MSG_QUEUE0];
					MSG_QUEUE_RCV_ID = msg_queue_id[MSG_QUEUE1];
                                        #if DEBUG
					printf("1 before sem post MSG_QUEUE_CMN_ID %d MSG_QUEUE_RCV_ID %d\n",
                                            MSG_QUEUE_CMN_ID, MSG_QUEUE_RCV_ID); /* debug */
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
			    } /*** End loop2 ***/
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

void *client_communication()
{
	int sockfd; /* socket */
	int portno; /* port to listen on */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr = {0}; /* server's addr */
	struct sockaddr_in clientaddr = {0}; /* client addr */
	char buf[BUFSIZE]; /* message buf */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */
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
	serveraddr.sin_port = htons((unsigned short)DC_PORT);

	if (bind(sockfd, (struct sockaddr *) &serveraddr, 
				sizeof(serveraddr)) < 0) 
		perror("ERROR on binding");

	clientlen = sizeof(clientaddr);
	while (1) {

		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		bzero(buf, BUFSIZE);
		n = recvfrom(sockfd, &my_msgbuf.pc_info/*&System_info_cpu*/, sizeof System_info_cpu, 0,
				(struct sockaddr *) &clientaddr, &clientlen);
		if (n < 0) {
			perror("ERROR in recvfrom client in communication,  ");
			sleep(5);
			continue;
		}

		/* 
		 * gethostbyaddr: determine who sent the datag
		 */
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


		/*SEND ack packet*/
		if (n > 0) {
			if (sendto(sockfd, ack, sizeof ack, 0, (struct sockaddr *)&clientaddr, sizeof clientaddr) == -1) {                
        	                perror("sendto");
                	        exit(EXIT_FAILURE);
                	}
		}

		time (&rawtime);
		timeinfo = localtime(&rawtime);
		printf ( "Date & Time : %s\n", asctime(timeinfo));
		
		#if DEBUG
		/* Debug*/	
		printf("Received bytes from client : %d \n", n);
		printf("Host Name	: %s, Host Address : %s\n", hostp->h_name, hostaddrp);
		printf("CPU info	: %s\n", System_info.cpu_details.model);
		printf("CPU speed info	: %f\n", System_info.cpu_details.speed);
		printf("MAC Address		: %s\n", System_info.network_info.hw_addr);
		#endif   	

	}		

	
}





