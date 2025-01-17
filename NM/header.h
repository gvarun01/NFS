#ifndef __NAMING_SERVER_H__
#define __NAMING_SERVER_H__

#include "../common.h"

#define MAX_STORAGE_SERVERS 20
#define MAX_CLIENTS 100

typedef struct {
    int id;
    char ip[MAX_IP_SIZE];
    int nm_port;
    int socket;
    int client_port;
    int ss_port;
    bool is_alive;
    time_t last_ping;
    pthread_mutex_t lock;
    pthread_t ping_thread;
    int backup1;
    int backup2;
    int hold_backup[MAX_STORAGE_SERVERS];
    int backups_hold;
} ss_t;

typedef struct bool_pair {
    bool one;
    bool two;
    bool three;
} bool_pair;

typedef struct file_lock {
    char path[MAX_PATH_LENGTH];
    bool is_write_lock;
    int read_lock_count;
    bool sync;
    bool syncsync;
    struct file_lock* next;
} file_lock_t;


// =========================== Trie ===========================

typedef struct TrieNode {
    char *key; // String key for the node
    struct TrieNode *children[256]; // Assuming ASCII characters
    int storage_server_id; // ID of the storage server
    bool is_end_of_path; // Indicates if the node represents the end of a path
} TrieNode;

TrieNode* create_trie_node(const char *key);
void insert_path(TrieNode *root, const char *path, int storage_server_id);
int search_path(TrieNode *root, const char *path);
bool delete_path(TrieNode *root, const char *path);
void print_paths(TrieNode *root, char *buffer, int depth);
void delete_trie(TrieNode *root);
ListNode* list_all_paths(TrieNode *root);
void adding_paths(TrieNode *root, ListNode **head);

// ============================ BOOK KEEPING ========================
#define LOG_FILE "record.txt"
#define CLIENT_COMMUNICATION -1
#define STORAGE_SERVER_COMMUNICATION -2

int insert_log(int communication_type, int id, int port, RequestType type, char *info, int status_code);
void print_log();
void signal_handler(int signal);
void setup_signal_handler();

// ============================ BOOK KEEPING ========================
#define CACHE_SIZE 30  // Fixed cache size
#define MIN_CACHE_SIZE 16  // Minimum cache size
#define MAX_CACHE_SIZE 200  // Maximum cache size

// ============================ LRU Cache ============================
typedef enum {
    CACHE_SUCCESS = 0,
    CACHE_ERROR_FULL = -1,
    CACHE_ERROR_NOT_FOUND = -2,
    CACHE_ERROR_INVALID_ARG = -3,
    CACHE_ERROR_MEMORY = -4
} cache_error_t;

// Node structure for doubly linked list
typedef struct CacheNode {
    char* path;           // Dynamically allocated path
    int ss_id;
    struct CacheNode* prev;
    struct CacheNode* next;
} CacheNode;

// Hash table entry structure
typedef struct HashEntry {
    char* path;           // Dynamically allocated path
    CacheNode* node;
    struct HashEntry* next;
} HashEntry;

// LRU Cache structure
typedef struct {
    const int capacity;   // Made constant to enforce fixed size
    int size;
    CacheNode* head;
    CacheNode* tail;
    HashEntry** hash_table;
    int hash_size;
    pthread_mutex_t lock;
    int collision_count;  // For monitoring hash efficiency
} LRUCache;

typedef struct {
    int client_socket;
    int thread_id;
} client_thread_args;

typedef struct ss_connection_t{
    int socket;
    int ss_id;
    pthread_mutex_t send_lock;  // For thread-safe sending
} ss_connection_t;

cache_error_t delete_from_cache(LRUCache *cache, const char *path);
// =========================== NS globals ===========================

// Global variables for server state
extern TrieNode* g_directory_trie;
extern LRUCache* g_cache;
extern ss_t g_storage_servers[MAX_STORAGE_SERVERS];
extern ss_connection_t g_ss_connections[MAX_STORAGE_SERVERS];
extern int g_ss_count;
extern file_lock_t* g_file_locks;

// Global mutexes
extern pthread_mutex_t g_ss_lock;
extern pthread_mutex_t g_trie_lock;
extern pthread_mutex_t g_file_lock;

// Network-related globals
extern char g_nm_ip[MAX_IP_SIZE];
extern int g_server_listen_port;
extern int g_client_listen_port;
extern int g_nm_socket;
extern int g_client_socket;
extern pthread_t g_nm_thread;
extern pthread_t g_client_thread;
extern pthread_t *g_client_threads;
extern SS_Request* g_requests;
extern pthread_mutex_t g_request_threads;

static uint32_t hash_function(const char* path);
static char* safe_strdup(const char* str);
static CacheNode* create_cache_node(const char* path, int ss_id);
LRUCache* init_lru_cache(int requested_capacity);
static void add_node(LRUCache* cache, CacheNode* node);
static void remove_node(CacheNode* node);
static void move_to_head(LRUCache* cache, CacheNode* node);
cache_error_t get_from_cache(LRUCache* cache, const char* path, int* ss_id);
static CacheNode* evict_lru(LRUCache* cache);
cache_error_t put_in_cache(LRUCache* cache, const char* path, int ss_id);
void get_cache_stats(LRUCache* cache, int* size, int* capacity, float* collision_rate);
void destroy_cache(LRUCache* cache);
ListNode* list_paths_under_prefix_with_ssid(TrieNode *root, const char *prefix, int target_ss_id);
void adding_paths_under_prefix_with_ssid(TrieNode *root,
                                         ListNode **head, int target_ss_id);
TrieNode* find_node_for_prefix(TrieNode *root, const char *prefix);

// ============================ Handlers ========================
Request handle_request(Request req, int thread_id, int client_sock);
SS_Request handle_ss_request(int ss_id, SS_Request req);

Request handle_read(char *path);
Request handle_write(char *path);
Request handle_get_info(char *path);
Request handle_stream_audio(char *path);
Request handle_list_paths(int client_sock);
Request handle_create(char *path,char *name, int thread_id);
Request handle_delete(char *path);
Request handle_copy(char *path);

// ============================ Naming Server =====================
void connect_and_send_to_SS(SS_Request request);
int ns_register_storage_server(const char* ip, int nm_port, int client_port, int ss_port);
void* start_network_listener(void* arg);
void send_response(int sock, RequestType type, ErrorCode error, const char* info, int ss_id);
Request send_request_to_ss(int ss_id, SS_Request request);

static void* ping_storage_server(void* arg);
static void* handle_client_connection(void* arg);
static void* handle_ss_communication(void* arg);
static void* handle_storage_server_connection(void* arg);
void* start_network_listener(void* arg);

void release_file_lock(const char* path);
bool_pair check_file_lock(const char* path, bool check_write);
void set_file_lock_sync(const char* path, bool sync);
void acquire_file_lock(const char* path, bool check_write);



// ============================ BACKUP ========================
void *backup(void *arg);
#endif