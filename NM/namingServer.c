#include "header.h"

// Global variables for server state
TrieNode* g_directory_trie = NULL;
LRUCache* g_cache = NULL;
ss_t g_storage_servers[MAX_STORAGE_SERVERS];
ss_connection_t g_ss_connections[MAX_STORAGE_SERVERS];
int g_ss_count = 0;
file_lock_t* g_file_locks = NULL;

// Global mutexes
pthread_mutex_t g_ss_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_trie_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_file_lock = PTHREAD_MUTEX_INITIALIZER;

// Network-related globals
char g_nm_ip[MAX_IP_SIZE];
int g_server_listen_port;
int g_client_listen_port;
int g_nm_socket = -1;
int g_client_socket = -1;
pthread_t g_nm_thread;
pthread_t g_client_thread;
pthread_t *g_client_threads;
SS_Request* g_requests;
pthread_mutex_t g_request_threads = PTHREAD_MUTEX_INITIALIZER;

void get_server_ip(char *ip_buffer) {
    struct ifaddrs *ifaddr, *ifa;
    int family;
    char host[MAX_IP_SIZE];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // Loop through each network interface
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;

        // Only look for IPv4 addresses
        if (family == AF_INET) {
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, MAX_IP_SIZE, NULL, 0, NI_NUMERICHOST) == 0) {
                // Filter out "lo" (localhost) interface and get the actual network interface
                if (strcmp(ifa->ifa_name, "lo") != 0) {
                    strncpy(ip_buffer, host, MAX_IP_SIZE);
                    break;
                }
            }
        }
    }

    freeifaddrs(ifaddr);
}

// Register a new storage server
int ns_register_storage_server(const char* ip, int nm_port, int client_port, int ss_port) {
    pthread_mutex_lock(&g_ss_lock);
    
    if (g_ss_count >= MAX_STORAGE_SERVERS) {
        pthread_mutex_unlock(&g_ss_lock);
        return -1;
    }
    for(int i=0;i<g_ss_count;i++){
        if(strcmp(g_storage_servers[i].ip,ip)==0 && g_storage_servers[i].client_port==client_port){
            printf("WELCOME BACK\n");
            g_storage_servers[i].is_alive=true;
            g_storage_servers[i].last_ping=time(NULL);
            pthread_mutex_unlock(&g_ss_lock);

            return i;
        }
    }
    int ss_id = g_ss_count++;
    ss_t* ss = &g_storage_servers[ss_id];
    
    strncpy(ss->ip, ip, sizeof(ss->ip) - 1);
    ss->nm_port = nm_port;
    printf("%d\n", client_port);
    ss->client_port = client_port;
    ss->ss_port = ss_port;
    ss->id = ss_id;
    ss->is_alive = true;
    ss->last_ping = time(NULL);
    ss->backup1=-1;
    ss->backup2=-1;
    memset(ss->hold_backup,-1,sizeof(ss->hold_backup));
    ss->backups_hold=0;
   // backup();
    //pthread_create(&ss->ping_thread, NULL, ping_storage_server, ss);

    pthread_mutex_unlock(&g_ss_lock);

    return ss_id;
}

// Handle client requests
// int ns_handle_client_request(char* request, const char* path,
//                            int client_id, char* response, size_t response_size) {
//     pthread_mutex_lock(&g_trie_lock);
//     int ss_id = searchPath(g_directory_trie, path);
//     pthread_mutex_unlock(&g_trie_lock);

//     if (ss_id == 0) return NFS_ERR_PATH_NOT_FOUND;

//     storage_server_t* ss = &g_storage_servers[ss_id];
//     pthread_mutex_lock(&ss->lock);
    
//     if (!ss->is_alive) {
//         pthread_mutex_unlock(&ss->lock);
//         return NFS_ERR_SERVER_DOWN;
//     }

//     bool need_write_lock = (type == REQ_WRITE || type == REQ_WRITE_ASYNC || 
//                            type == REQ_DELETE_FILE || type == REQ_DELETE_DIR);
    
//     if (!acquire_file_lock(path, client_id, need_write_lock)) {
//         pthread_mutex_unlock(&ss->lock);
//         return NFS_ERR_FILE_LOCKED;
//     }

//     snprintf(response, response_size, "%s:%d", ss->ip, ss->client_port);
//     pthread_mutex_unlock(&ss->lock);

//     return NFS_OK;
// }

// File locking functions
bool_pair check_file_lock(const char* path, bool check_write) {
    pthread_mutex_lock(&g_file_lock);
    bool_pair result;
    file_lock_t* current = g_file_locks;
    while (current) {
        if (strcmp(current->path, path) == 0) {
            if (check_write || current->is_write_lock) {
                result.three = current->syncsync;
                result.two = current->sync;
                result.one = false;
                pthread_mutex_unlock(&g_file_lock);
                return result;
            }
        }
        current = current->next;
    }
    pthread_mutex_unlock(&g_file_lock);
    result.one = true;
    result.two = true;
    result.three = true;
    return result;
}

void acquire_file_lock(const char* path, bool check_write) {
    pthread_mutex_lock(&g_file_lock);
    file_lock_t* current = g_file_locks;
    bool flag = true;
    while (current) {
        if (strcmp(current->path, path) == 0) {
            flag = false;
            break;
        }
        current = current->next;
    }
    if(flag){
        file_lock_t* new_lock = malloc(sizeof(file_lock_t));
        strncpy(new_lock->path, path, MAX_PATH_LENGTH - 1);
        new_lock->is_write_lock = check_write;
        new_lock->next = g_file_locks;
        new_lock->read_lock_count = 0;
        new_lock->sync = true;
        new_lock->syncsync = false;
        g_file_locks = new_lock;
    }
    else if(!current->is_write_lock){
        current->read_lock_count++;
    }
    pthread_mutex_unlock(&g_file_lock);
}

void set_file_lock_sync(const char* path, bool sync){
    pthread_mutex_lock(&g_file_lock);
    file_lock_t* current = g_file_locks;
    while (current) {
        if (current->is_write_lock) {
            if (strcmp(current->path, path) == 0) {
                current->sync = sync;
                current->syncsync = true;
                printf("Setting sync %d %d for %s\n", sync,current->syncsync, path);
                pthread_mutex_unlock(&g_file_lock);
                return;
            }
        }
        current = current->next;
    }
    pthread_mutex_unlock(&g_file_lock);
}

void release_file_lock(const char* path) {
    pthread_mutex_lock(&g_file_lock);
    
    file_lock_t* current = g_file_locks;
    file_lock_t* prev = NULL;

    while (current) {
        if (strcmp(current->path, path) == 0) {
            if(!current->is_write_lock && current->read_lock_count > 0)
                current->read_lock_count--;
            else{
                if (prev) {
                    prev->next = current->next;
                } else {
                    g_file_locks = current->next;
                }
                free(current);
            }
            break;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&g_file_lock);
}

void ns_cleanup() {
    printf("\n\n");
    // Cleanup storage servers
    for (int i = 0; i < g_ss_count; i++) {
        printf("Cancelling SS %d\n", i);
        pthread_mutex_destroy(&g_storage_servers[i].lock);
        if(g_storage_servers[i].is_alive)
            pthread_cancel(g_storage_servers[i].ping_thread);
    }
    // Cleanup file locks
    printf("Bu-bye file locks!!\n");
    file_lock_t* current = g_file_locks;
    while (current) {
        file_lock_t* next = current->next;
        free(current);
        current = next;
    }
    // Cleanup cache
    printf("Toodles Cache!!\n");
    destroy_cache(g_cache);
    // Cleanup trie
    printf("Live your life free, Mr.Trie!!\n");
    delete_trie(g_directory_trie);
    // Cleanup mutexes
    printf("Caught y'all in my hexes, you fickle mutexes!!!\n");
    pthread_mutex_destroy(&g_ss_lock);
    pthread_mutex_destroy(&g_trie_lock);
    pthread_mutex_destroy(&g_file_lock);

    // Cleanup network
    printf("Boy were you a jerk, Mr.Network!!\n");
    if (g_nm_socket >= 0) close(g_nm_socket);
    if (g_client_socket >= 0) close(g_client_socket);

    // Goodbye message
    printf("See y'all chumps again! (NEVER)\n");
}

static void handle_signal(int sig) {
    ns_cleanup();
    exit(0);
}

int main(int argc, char* argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTSTP, print_log);

    // initialize NS
    get_server_ip(g_nm_ip);
    g_server_listen_port = atoi(argv[1]);
    g_client_listen_port = g_server_listen_port + 1;
    g_directory_trie = create_trie_node(NULL);
    if(g_directory_trie == NULL){
        perror("Failed to initialize directory trie");
        return 1;
    }
    g_cache = init_lru_cache(CACHE_SIZE);
    if(g_cache == NULL){
        perror("Failed to initialize cache");
        return 1;
    }

    g_ss_count = 0;
    for (int i = 0; i < MAX_STORAGE_SERVERS; i++) {
        pthread_mutex_init(&g_storage_servers[i].lock, NULL);
    }

    g_client_threads = (pthread_t*)malloc(sizeof(pthread_t) * MAX_CLIENTS);
    g_requests = (SS_Request*)malloc(sizeof(SS_Request) * MAX_CLIENTS);
    pthread_mutex_lock(&g_request_threads);
    for(int i = 0; i<MAX_CLIENTS; i++) {
        g_requests[i].Thread_id = -1;
        g_requests[i].type = WAITING;
    }
    pthread_mutex_unlock(&g_request_threads);


    printf("Naming server running on ip: %s\n", g_nm_ip);
    //Start listening.
    
    pthread_create(&g_nm_thread, NULL, start_network_listener, NULL);
    pthread_t backup_thread;
    pthread_create(&backup_thread, NULL, backup, NULL);

    // Register cleanup handler

    // Wait for network thread
    pthread_join(g_nm_thread, NULL);
    pthread_join(backup_thread, NULL);

    return 0;
}
