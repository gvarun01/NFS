#include "../common.h"

#define MAX_SYNC_SIZE_ALLOWED 2

// Define a linked list node for storing paths
typedef struct PathNode {
    char path[MAX_PATH_LENGTH];           // Path string
    struct PathNode *next;    // Pointer to the next node
} PathNode;

typedef struct {
    char **paths;
    size_t count;
    size_t capacity;
} PathArray;

extern int server_id;
extern int nm_sock;
extern int nm_port;
extern struct sockaddr_in nm_addr;
extern int ss_sock;
extern int ss_port;
extern char *root_directory;
extern char *nm_ip;
extern int nm_port;
extern char *ss_ip;
extern int ss_nm_port;
extern int ss_client_port;
extern ListNode *accessiblePathsHead;

void send_to_server(ErrorCode error_code, RequestType requestType, char *info, int Thread_id);
void ss_handle_read_request(int client_sock, char *file_path);
void ss_handle_write_request(int client_sock, char *file_path, bool isSync);
int ss_write(int client_sock, int file, ListNode *requestList);
void ss_handle_get_size_and_permissions(int client_sock, char *file_path);
void ss_handle_stream_audio(int client_sock, char *file_path);
void ss_handle_create_request(int client_sock, SS_Request *request);
void ss_create_file(int client_sock, char *path, char*name, int thread_id);
void ss_create_directory(int client_sock, char *path, char *name, int thread_id);
void ss_handle_delete_request(int client_sock, SS_Request *request);
void ss_delete_file(int client_sock, char *path, int thread_id);
void ss_delete_recursive_dir(int client_sock, char *path, int thread_id);
void *nm_request_listener(void *arg);
void *nm_thread_handler(void *arg);
void handle_nm_command(int nm_request);
int ss_connect_to_ss(int client_sock, char *source_ip, int source_port, int thread_id);
void ss_paste_file_or_directory(int client_sock, SS_Request *request);
int ss_paste_file(int client_sock, int source_sock, char *dest, int thread_id);
void ss_paste_directory(int client_sock, int source_sock, char *dest, int thread_id, char *base);
void ss_copy_file_or_directory(int client_sock, SS_Request *request);
int ss_copy_file(int client_sock, char *src, int start);
int ss_copy_directory(int client_sock, char *src, int start, RequestType reqType);
void add_accessible_path(char *path);
void add_directory_to_accessible_paths(char *base_path, char *current_path);
void get_all_paths(char *root_dir);
void add_accessible_path(char *path);
void get_all_paths_from_file();
void register_with_naming_server();
void make_SS_socket();
void *ss_listener(void *arg);
void *ss_thread_handler(void *arg);
void handle_ss_request(int client_sock);
void recursive_backup(int client_sock, int backup_sock, char *source_path, char *dest_path);
char **get_all_files_folders(const char *path);
int create_dirs(const char *file_path);
void pnt_ss(SS_Request recvd_request, int sock_fd);
void ss_cpy_file(int client_sock, char *file_path);
void create_dirs_and_files_from_list(const char *paths_list);
void dataToWrite(char *full_path, char *data);
bool is_path_accessible(const char *path, ListNode *head);
int ss_write_async(int client_sock, int file, ListNode *requestList);