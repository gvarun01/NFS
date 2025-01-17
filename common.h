#ifndef __HEADER__
#define __HEADER__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/select.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/wait.h> 

#define BUFFER_SIZE 1024
#define INPUT_SIZE 1024
#define MAX_PACKET_SIZE 1024
#define OPERATION_SIZE 1024
#define MAX_PATH_LENGTH 1024
#define MAX_IP_SIZE 16
#define MAX_INFO_SIZE 10024

#define READ "READ"
#define WRITE "WRITE"
#define GET_SIZE_AND_PERMISSIONS "GET_SIZE_AND_PERMISSIONS"
#define STREAM_AUDIO "STREAM_AUDIO"
#define CREATE "CREATE"
#define DELETE "DELETE"
#define COPY "COPY"
#define LIST_PATHS "LIST_PATHS"

// ===========================Request Types===========================
typedef enum RequestType {
    ACK,
    ACK_ASYNC,
    REQ,
    RESPONSE,
    REQUEST_READ,
    REQUEST_WRITE,
    REQUEST_GET_SIZE_AND_PERMISSIONS,
    REQUEST_STREAM_AUDIO,
    REQUEST_CREATE,
    REQUEST_DELETE,
    REQUEST_COPY,
    REQUEST_PASTE,
    REQUEST_CREATE_FILE,
    REQUEST_CREATE_DIRECTORY,
    REQUEST_DELETE_FILE,
    REQUEST_DELETE_DIRECTORY,
    REQUEST_COPY_FILE,
    REQUEST_COPY_DIRECTORY,
    REQUEST_LIST_PATHS,
    REQUEST_ASYNC_WRITE,
    REQUEST_SYNC_WRITE,
    REQUEST_CONCURRENT_ACCESS,
    REQUEST_ERROR_CODE,
    REQUEST_SEARCH,
    REQUEST_BACKUP_DATA,
    REQUEST_BACKUP_READ,
    REQUEST_BACKUP_WRITE,
    REQUEST_BACKUP_APPEND,
    REQUEST_BACKUP_DELETE_FILE,
    REQUEST_BACKUP_DELETE_DIRECTORY,
    REQUEST_BACKUP_CREATE_FILE,
    REQUEST_BACKUP_CREATE_DIRECTORY,
    REQUEST_RECOVERY,
    REQUEST_LOGGING,
    REQUEST_IP_PORT_RECORDING,
    REQUEST_REGESTER,
    REQUEST_LIST_ALL_PATHS,
    REQUEST_FILE_NOT_FOUND,
    REQUEST_READ_STARTED,
    REQUEST_READ_COMPLETED, 
    REQUEST_WRITE_STARTED, 
    REQUEST_WRITE_STARTED_SYNC,
    REQUEST_WRITE_STARTED_ASYNC,
    REQUEST_WRITE_COMPLETED,
    REQUEST_GET_SIZE_PERMISSIONS_STARTED, 
    REQUEST_GET_SIZE_PERMISSIONS_COMPLETED,
    REQUEST_AUDIO_STARTED, 
    REQUEST_AUDIO_COMPLETED,
    CREATE_STARTED,
    CREATE_COMPLETED,
    DELETE_STARTED,
    PARTIAL_DELETE_COMPLETED,
    DELETE_COMPLETED,
    COPY_STARTED,
    COPY_COMPLETED,
    PASTE_STARTED,
    PASTE_COMPLETED,
    REQUEST_PASTE_FILE,
    REQUEST_ADD_PATH,
    REQUEST_DELETE_PATH,
    PING,
    PASTE,
    FILE_DATA_TO_BE_COPIED_SS,
    DIRECTORY_TO_BE_COPIED_SS,
    WAITING,
    REQUEST_FILE_CONTENT,
    COPY_ADD_DATA,
    COPY_ADD_PATH,
    INVALID_INPUT,
    FILE_COMPLETE, 
    REQUEST_BACKUP_COPY,
} RequestType;

// =========================== COLORS ===========================
#define YELLOW_CODE "\033[1;33m"
#define PINK_CODE "\033[1;35m"
#define RESET_CODE "\033[0m"
#define RED_CODE "\033[1;31m"
#define GREEN_CODE "\033[1;32m"
#define BLUE_CODE "\033[1;34m"
#define CYAN_CODE "\033[1;36m"
#define BOLD_CODE "\033[1m"
#define UNDERLINE_CODE "\033[4m"
#define STRIKETHROUGH_CODE "\033[9m"

#define YELLOW(str) YELLOW_CODE str RESET_CODE
#define PINK(str) PINK_CODE str RESET_CODE
#define RED(str) RED_CODE str RESET_CODE
#define GREEN(str) GREEN_CODE str RESET_CODE
#define BLUE(str) BLUE_CODE str RESET_CODE
#define CYAN(str) CYAN_CODE str RESET_CODE
#define BOLD(str) BOLD_CODE str RESET_CODE
#define UNDERLINE(str) UNDERLINE_CODE str RESET_CODE
#define STRIKETHROUGH(str) STRIKETHROUGH_CODE str RESET_CODE

#define YELLOW_BOLD(str) YELLOW_CODE BOLD_CODE str RESET_CODE
#define PINK_BOLD(str) PINK_CODE BOLD_CODE str RESET_CODE
#define RED_BOLD(str) RED_CODE BOLD_CODE str RESET_CODE
#define GREEN_BOLD(str) GREEN_CODE BOLD_CODE str RESET_CODE
#define BLUE_BOLD(str) BLUE_CODE BOLD_CODE str RESET_CODE
#define CYAN_BOLD(str) CYAN_CODE BOLD_CODE str RESET_CODE
#define UNDERLINE_BOLD(str) UNDERLINE_CODE BOLD_CODE str RESET_CODE
#define STRIKETHROUGH_BOLD(str) STRIKETHROUGH_CODE BOLD_CODE str RESET_CODE

// =========================== Error Codes ===========================
typedef enum ErrorCode{
    OK,
    RECIEVED_ERROR,
    SOCKET_ERROR,
    CONNECTION_CLOSED,
    FILE_NOT_FOUND,
    READ_FAILED,
    PERMISSION_DENIED,
    UNKNOWN_OPERATION,
    WRITE_FAILED,
    STAT_FAILED,
    DIR_DOESNT_EXIST,
    UNABLE_TO_CHANGE_DIR,
    FILE_CREATION_FAILED,
    DIR_CREATION_FAILED,
    FILE_DELETION_FAILED,
    DIR_DELETION_FAILED,
    NOT_FILE_OR_DRECTORY,
    CONNECTION_FAILED,
    REGISTERATION_FAILED,
    CACHING_FAILED, 
    FORK_FAILED, 
    WAIT_FAILED,
    RM_FAILED,
    PASTE_FAILED,
    SEND_FAILED,
    INVALID_PATH,
    NOT_STREAMABLE
}ErrorCode;

static const char *error_names[] = {
    "OK",
    "RECIEVED_ERROR",
    "SOCKET_ERROR",
    "CONNECTION_CLOSED",
    "FILE_NOT_FOUND",
    "READ_FAILED",
    "PERMISSION_DENIED",
    "UNKNOWN_OPERATION",
    "WRITE_FAILED",
    "STAT_FAILED",
    "DIR_DOESNT_EXIST",
    "UNABLE_TO_CHANGE_DIR",
    "FILE_CREATION_FAILED",
    "DIR_CREATION_FAILED",
    "FILE_DELETION_FAILED",
    "DIR_DELETION_FAILED",
    "NOT_FILE_OR_DRECTORY",
    "CONNECTION_FAILED",
    "REGISTERATION_FAILED",
    "CACHING_FAILED"
};

// ============================ Linked List ============================
typedef struct ListNode {
    void *data;
    struct ListNode *next;
} ListNode;

ListNode* create_list_node(void *data);
void append_to_list(ListNode **head, void *data);
void delete_list_node(ListNode **head, void *data, int (*cmp)(const void *, const void *));
void free_list(ListNode *head);



// =========================== General Request Structure ===========================
typedef struct Request {
    RequestType type;
    ErrorCode error_code;
    char info[MAX_INFO_SIZE]; // Additional information formatted according to the request type
} Request;

typedef struct SS_Request {
    RequestType type;
    ErrorCode error_code;
    int SS_id;
    int Thread_id;
    char info[MAX_INFO_SIZE]; // Additional information formatted according to the request type
} SS_Request;

typedef struct stHandleRequest {
    int client_id;
    // Request request;
    int type;
    char info[MAX_INFO_SIZE];
} stHandleRequest;

typedef struct stHandleRequest *HandleRequest;

typedef struct Packet{
    RequestType operation;
    char data[MAX_PACKET_SIZE];
    ErrorCode error_code;
}Packet;

typedef struct Reply{
    RequestType operation;
    char src[MAX_PACKET_SIZE];
    char dest[MAX_PACKET_SIZE];
    char name[MAX_PACKET_SIZE];
    char sourceIP[MAX_IP_SIZE];
    int portNo;
    bool isSync;
    ErrorCode error_code;
} Reply;

typedef struct {
    int client_sock;
    int source_sock;
    const char *dest;
} CopyArgs;

typedef struct {
    char src[512];
    char dest[512];
} PasteRequest;


#endif
