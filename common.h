#if ! defined _COMMON_H_
#define _COMMON_H_

/*String literals*/
#define TERMINATE   9
#define FALSE       0
#define TRUE        1
#define ACK "1"

/*structure declaration*/
typedef enum endian {
    LITTLE_Endian = 1,
    BIG_Endian,
} ENDIAN;


typedef struct cpu_details {
    char model[50];
    char core;
    float speed;
    int cache;
    ENDIAN endian;
    int arch;
} cpu_info_t;

typedef struct mem_details {
    unsigned int Memtotal;
    unsigned int Memfree;
    unsigned int Memavailable;
    unsigned int Active;
    unsigned int Inactive;
    unsigned int Active_file;
    unsigned int Inactive_file;
    unsigned int Swaptotal;
    unsigned int Swapfree;
    unsigned int HardwareCorrupted;
    //GB
    float Harddisk_size_used;
    float Harddisk_size_avail;
    float Swap_size;

} mem_info_t;

typedef struct os_details {
    char DISTRIB_ID[10];
    float DISTRIB_RELEASE;
    char kernal_version[10];
} os_info_t;

typedef struct process_details {
    int Total_Proc;
    int Runing_Proc;
    int Sleep_Proc;
    int Stopped_Proc;
    int Zombie;
} process_info_t;

typedef struct network_info {
    int mtu;
    char hw_addr[23];
    char ip[15];
} network_info_t;

typedef struct details {
    cpu_info_t cpu_details;
    mem_info_t mem_details;
    os_info_t os_details;
    process_info_t process_details;
    network_info_t network_info;
} System_info_t;

#define OPEN_PROC_CPU_INFO "cat /proc/cpuinfo"
#define OPEN_PROC_MEM_INFO "cat /proc/meminfo"
#define OPEN_ETC_RELEASE_INFO "cat /etc/*-release"

#define INFO_GET_SUCCESS 1 
#define INFO_GET_FAIL (-1)

/*Extern declarations */
extern int errno;

#endif /* end of _COMMON_H_ */

