
#include "client.h"
#define DEBUG 1 

System_info_t System_info;
enum info_type_t objtype;

int get_cpu_info(void)
{
	FILE *fp;
	char buf[BUF_MAX];
	objtype = CPU_INFRMTN; 
	fp = popen(OPEN_PROC_CPU_INFO, "r");
	if (fp == NULL) {
		fprintf(stderr, "Failed to open (%s) for getting cpuinfo: %s\n", OPEN_PROC_CPU_INFO, strerror(errno)); 
		return INFO_GET_FAIL; 
	}
		

	while (fgets(buf, sizeof buf, fp) != NULL) {
		if (parse_data(buf, objtype) == TERMINATE)
			break;
		memset(buf, 0, sizeof(buf));
	}

	pclose(fp);
	get_machine_arch();
	get_endian();
#ifdef DEBUG
	print_cpu_info();
#endif
	return 1;
}

int print_cpu_info (void)
{
    printf("\n **** cpu info **** \n");
    printf("cpu model: %s\ncpu core: %d\ncpu speed: %f\n"
            "cpu cache: %d\nendian: %d\narch: %d\n", 
            System_info.cpu_details.model,
            System_info.cpu_details.core,
            System_info.cpu_details.speed,
            System_info.cpu_details.cache,
            System_info.cpu_details.endian,
            System_info.cpu_details.arch);
}

int get_endian (void)
{
    int data = 1;
    char *ptr;

    ptr = (char *) &data;
    if (*ptr) {
        System_info.cpu_details.endian = LITTLE_Endian;
    } else {
        System_info.cpu_details.endian = BIG_Endian;
    }
}

int get_machine_arch(void)
{
    System_info.cpu_details.arch = sizeof(int *) * 8;

    return (sizeof(int *) * 8);
} 

int parse_data (char *buf, enum info_type_t type)
{   

    char temp[128], i = 0;
    char ret;

    while (*buf != ':' ) {
        if (*buf == '	') {
            buf++;
            continue;
        }

        if (*buf == '=') 
            break;

        *(temp + i) = *buf;
        i++, buf++;
    }

    buf++;
    *(temp + i) = '\0';
    ret = compare_and_store_data(temp, buf, type);

    return ret;
}

char *get_data (char *buf, char *value)
{
    char  i = 0;

   if ((buf == NULL) || (value == NULL)) {
        perror("Invalid args to get_data");
        return NULL;
    }

    while (*buf != '\0') {
        if (*buf == '\n') {
            buf++;
            continue;
        }
        *(value + i++) = *buf;
        buf++;
    }

    *(value + i) = '\0';

    return value;
}


int compare_and_store_data (char *data_caption, 
        char *databuf, 
        enum info_type_t type)
{
    int ret = -1;
    char value[512];
    switch (type) {
        case CPU_INFRMTN: /* CPU INFO */
            if (strcmp(data_caption, "model name") == 0) {
                get_data(databuf, value);
                strcpy(System_info.cpu_details.model, value);     

            }
            if (strcmp(data_caption, "flags") == 0) {
                ret = TERMINATE;
            } 
            if (strcmp(data_caption, "cpu MHz") == 0) {
                get_data(databuf, value);
                System_info.cpu_details.speed = atof(value);
            }
            if (strcmp(data_caption, "cache size") == 0) {
                get_data(databuf, value);
                System_info.cpu_details.cache = atoi(value);    
            }
            if (strcmp(data_caption, "cpu cores") == 0) {
                get_data(databuf, value);
                System_info.cpu_details.core = atoi(value);
            }

            break;

        case MEM_INFRMTN: /* MEMORY INFO */
            if (strcmp(data_caption, "MemTotal") == 0) {
                get_data(databuf, value);
                System_info.mem_details.Memtotal = atoi(value);
            }
            if (strcmp(data_caption, "MemFree") == 0) {
                get_data(databuf, value);
                System_info.mem_details.Memfree = atoi(value);
            } 
            if (strcmp(data_caption, "MemAvailable") == 0) {
                get_data(databuf, value);
                System_info.mem_details.Memavailable = atoi(value);
            }
            if (strcmp(data_caption, "Active") == 0) {
                get_data(databuf, value);
                System_info.mem_details.Active = atoi(value);
            }
            if (strcmp(data_caption, "Inactive") == 0) {
                get_data(databuf, value);
                System_info.mem_details.Inactive = atoi(value);
            }
            if (strcmp(data_caption, "Active(file)") == 0) {
                get_data(databuf, value);
                System_info.mem_details.Active_file = atoi(value);
            }
            if (strcmp(data_caption, "Inactive(file)") == 0) {
                get_data(databuf, value);
                System_info.mem_details.Inactive_file = atoi(value);
            }
            if (strcmp(data_caption, "SwapTotal") == 0) {
                get_data(databuf, value);
                System_info.mem_details.Swaptotal = atoi(value);
            }
            if (strcmp(data_caption, "SwapFree") == 0) {
                get_data(databuf, value);
                System_info.mem_details.Swapfree = atoi(value);
            }
            if (strcmp(data_caption, "HardwareCorrupted") == 0) {
                get_data(databuf, value);
                System_info.mem_details.HardwareCorrupted = atoi(value);
            }
            if (strcmp(data_caption, "DirectMap2M") == 0) {
                ret = TERMINATE;
            }

            break;
        case OS_INFRMTN: /* OS INFO */

            if (strcmp(data_caption, "DISTRIB_ID") == 0) {
                get_data(databuf, value);
                strcpy(System_info.os_details.DISTRIB_ID, value);
            }
            if (strcmp(data_caption, "DISTRIB_RELEASE") == 0) {
                get_data(databuf, value);
                System_info.os_details.DISTRIB_RELEASE = atof(value);
            }
            break;
        case PROC_INFRMTN: /* PROCESS INFO */
            if (strcmp(data_caption, "Tasks") == 0) {
                store_proc_info(databuf);
            }
            break;

        default: /* ERROR */
            printf("Invalid option\n");
            exit(1);
            break;
    }

    return ret;
}
int strcmp1(char *data, char *src)
{
	while (*data != *src) {
		printf("data %c\n", *data);
		data++, src++;
	}

}

int store_proc_info (char *databuf)
{
    char details[6][10], i = 0, j = 0;

    while (*databuf != '\0') {
        if (*databuf == ',') {
            details[i][j] = '\0';
            i++, j = 0;
            databuf++;
            continue;
        }
        details[i][j++] = *databuf++;
    }

    set_proc_info(details);

} 

int set_proc_info(char (*ptr)[10])
{
    System_info.process_details.Total_Proc = atoi(ptr[0]);
    System_info.process_details.Runing_Proc = atoi(ptr[1]);         
    System_info.process_details.Sleep_Proc = atoi(ptr[2]);
    System_info.process_details.Stopped_Proc = atoi(ptr[3]);
    System_info.process_details.Zombie = atoi(ptr[4]);

}


int print_proc_info()
{
    printf("\n **** process info ****\n");
    printf("Total_Proc: %d\nRunning_Proc: %d\n"
            "Sleep_Proc: %d\nStopped_Proc: %d\n"
            "Zombie: %d\n",
            System_info.process_details.Total_Proc,
            System_info.process_details.Runing_Proc,
            System_info.process_details.Sleep_Proc,
            System_info.process_details.Stopped_Proc,
            System_info.process_details.Zombie);

}  

	
int get_mem_info()
{
	FILE *fp;
        char buf[100], i = 0;
	char cmd[][50] = {"df -h --total | grep total | awk '{print$2}'", "df -h --total | grep total | awk '{print$4}'", "lsblk | grep SWAP |awk '{print$4}'"} ;
	float value[5];
	objtype = MEM_INFRMTN;	

        fp = popen(OPEN_PROC_MEM_INFO, "r");
        if (fp == NULL) {
              fprintf(stderr, "fopen: failed to open (%s) to get mem info: %s", OPEN_PROC_MEM_INFO, strerror(errno));
		return INFO_GET_FAIL;
	}

        while (fgets(buf, sizeof buf, fp)  !=  NULL) {
                parse_data(buf, objtype);
                memset(buf, 0, sizeof(buf));
        }

	pclose(fp);
	
	while (i < 3) {
       		fp = popen(cmd[i], "r");
       		if (fp == NULL)
                	perror(" ");

        	memset(buf, 0, 20);
        	if (fgets(buf, sizeof buf, fp)  ==  NULL) {
                	perror(" ");
        	}
		value[i] = atof(buf);	
        	pclose(fp);
		i++;
	}
	System_info.mem_details.Harddisk_size_used = value[0];
	System_info.mem_details.Harddisk_size_avail = value[1];
	System_info.mem_details.Swap_size = value[2];
#ifdef DEBUG
        print_mem_info();
#endif

}

int print_mem_info()
{
    printf("\n **** memory info **** \n");
    printf("mem total: %d\nmem free: %d\n"
            "mem available: %d\nactive: %d\n"
            "inactive: %d\nactive_file: %d\n"
            "inactive_file: %d\nswaptotal: %d\n"
            "swapfree: %d\nHardwareCorrupted: %d\n\n"
            " **** Harddisk Info **** \nHarddisk Size Used: %f"
            "\nHarddisk Size Avail: %f\nSwap size: %f\n",
            System_info.mem_details.Memtotal,
            System_info.mem_details.Memfree,
            System_info.mem_details.Memavailable,
            System_info.mem_details.Active,
            System_info.mem_details.Inactive,
            System_info.mem_details.Active_file,
            System_info.mem_details.Inactive_file,
            System_info.mem_details.Swaptotal,
            System_info.mem_details.Swapfree,
            System_info.mem_details.HardwareCorrupted,
            System_info.mem_details.Harddisk_size_used,
            System_info.mem_details.Harddisk_size_avail,
            System_info.mem_details.Swap_size);

    return 1;
}

int get_os_info()
{
        FILE *fp;
        char buf[100], n = 3;
	
        objtype = CPU_INFRMTN;
        fp = popen(OPEN_ETC_RELEASE_INFO, "r");
	if (fp == NULL) {
        fprintf(stderr, "Error in opening (%s) to get release: %s", OPEN_ETC_RELEASE_INFO);
        return INFO_GET_FAIL;
    }

	while (fgets(buf, sizeof buf, fp) != NULL) {
		parse_data(buf, objtype);
		memset(buf, 0, sizeof(buf));
	}
			
	pclose(fp);
	memset(buf, 0, sizeof(buf));

        fp = popen("uname -r", "r");
        if (fp == NULL)
                perror("popen uname ");

    	if (fgets(buf, sizeof buf, fp) == NULL) {
                    perror("uname fgets ");
        }
	strcpy(System_info.os_details.kernal_version, buf);
        memset(buf, 0, sizeof(buf));
        pclose(fp);
#ifdef DEBUG
        print_os_info();
#endif
}

int print_os_info(void)
{
    printf("\n **** OS_INFO **** \n");
    printf("OS NAME %s\nversion %f\nkernal version %s\n",
            System_info.os_details.DISTRIB_ID,
            System_info.os_details.DISTRIB_RELEASE,
            System_info.os_details.kernal_version);
}

int get_proc_info()
{
	FILE *fp;
        char buf[100];

	objtype = PROC_INFRMTN; 
        fp = popen("top -b -n 1 | head -5", "r");
        if (fp == NULL) {
                perror("popoen:get_proc_info ");
		return INFO_GET_FAIL; 
	}

	while (fgets(buf, sizeof buf, fp) != NULL){
		parse_data(buf, objtype);
		memset(buf, 0, sizeof(buf));
	}

	pclose(fp);
#ifdef DEBUG
    print_proc_info();
#endif
}
int get_ip_info()
{
    FILE *fp;
    char buf[100], i = 0;
    char cmd[][60] = {"ifconfig | grep MTU | awk 'NR==1{print$5}' | tail -c-5", "ifconfig -a | grep HWaddr | awk '{print$5}'", "hostname -I"} ;
    char data[5][40] = {0};

    while (i < 3) {
        fp = popen(cmd[i], "r");
        if (fp == NULL)
            perror("popen: get_ip_info ");

        //memset(buf, 0, 20);
        memset(buf, 0, sizeof buf);
        if (fgets(buf, sizeof buf, fp) < 0) {
            perror("fgets: get_ip_info ");
        }
        strcpy(data[i], buf);
        pclose(fp);
        i++;
    }
    System_info.network_info.mtu = atoi(data[0]);
    strcpy(System_info.network_info.hw_addr, data[1]);
    remove_newline(System_info.network_info.hw_addr);
    strcpy(System_info.network_info.ip, data[2]); 

#ifdef DEBUG
    print_network_info();
#endif
}
int remove_newline(char *temp)
{
	while (*temp != '\0') {
		if (*temp == '\n')
			*temp = '\0';
		temp++;
	}
}

int print_network_info()
{
    printf("\n **** Network info ****\n");
    printf("mtu: %d\nMac_address: %s\nIp_addr: %s",
            System_info.network_info.mtu,
            System_info.network_info.hw_addr,
            System_info.network_info.ip);

    return 1;
}


/*
 * set_server_info
 *
 * Setting server information
 */
static void 
set_server_info (struct sockaddr_in *server)
{
    server->sin_family = AF_INET;
    server->sin_port = htons(2000);
    server->sin_addr.s_addr = inet_addr("172.17.2.18");

    return ;
}

/*
 * main
 *
 * Main process started for client comms.
 */
int main ()
{
    int fd = -1;
    int len = 0, recvlen, n;
    struct sockaddr_in server = {0};
    struct timeval tv;  
    char buf[BUF_MAX];

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Failed to create socket\n");
        exit(1);
    }

    set_server_info(&server);
    tv.tv_sec = 5;  /* 30 Secs Timeout */
    tv.tv_usec = 0;  // Not init'ing this can cause strange errors
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));


    while (1) {
        get_cpu_info();
        get_mem_info();
        get_proc_info();
        get_ip_info();

        if ((len = sendto(fd, &System_info, sizeof System_info, 0, (struct sockaddr *)&server, sizeof server)) == -1) {
            perror("sendto");
            exit(1);
        }
	
	
		//RECEIVE WAIT 5SEC FOR ACK,
	n = recvfrom(fd, buf/*&System_info*/, sizeof buf, 0,
                                (struct sockaddr *)&server, &recvlen);

	if (n < 0)
		perror(" client recv ack");

        printf("%d Bytes written successfully.\n", len );
        sleep(2);
    }

    printf("Data sent successfully..\n");

    return 0;
}

	
