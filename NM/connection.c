#include "header.h"

#define MAX_PENDING_CONNECTIONS 100

// Global variables for SS connection
// Initialize a TCP socket for listening
static int init_socket(int port)
{
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("Failed to create socket");
        return -1;
    }

    // Enable address reuse
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(sock_fd);
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind failed");
        close(sock_fd);
        return -1;
    }

    if (listen(sock_fd, MAX_PENDING_CONNECTIONS) < 0)
    {
        perror("Listen failed");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

static void *ping_storage_server(void *arg)
{
    printf("Starting heartbeat.\n");
    int ss_id = *(int *)arg;
    SS_Request req;
    req.error_code = OK;
    req.type = PING;
    req.SS_id = ss_id;
    while (1)
    {
        pthread_mutex_lock(&g_ss_lock);
        ss_t* ss = &g_storage_servers[ss_id];
        if(difftime(time(NULL), ss->last_ping) > 9 ){
            pthread_mutex_lock(&ss->lock);
            ss->is_alive = false;
            pthread_mutex_unlock(&ss->lock);
            pthread_mutex_unlock(&g_ss_lock);
            printf("SS %d died!\n", ss_id);
            break;
        }
        pthread_mutex_unlock(&g_ss_lock);
        printf("\nSS %d is alive!", ss_id);
        connect_and_send_to_SS(req);
        printf("Sent PING to %d!\n", ss_id);
        sleep(5); // Ping every 5 seconds
    }
}

void connect_and_send_to_SS(SS_Request request)
{
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        // Some error occured while creating socket
        fprintf(stderr, RED("socket : unable to create socket for sending update paths request : %s\n"), strerror(errno));
        return ;
    }
    int port = g_storage_servers[request.SS_id].nm_port;
    if(port == 12000 || port == 12001){
        return;
    }
    address.sin_port = htons(port); // port on which server side process is listening
    address.sin_family = AF_INET;

    if (inet_pton(AF_INET, g_storage_servers[request.SS_id].ip, &address.sin_addr.s_addr) <= 0)
    {
        fprintf(stderr, RED("inet_pton : unable to convert ip to short int in send update paths request : %s\n"), strerror(errno));
        close(socket_fd);
        return ;
    }

    // Waiting for us to connect to the SS
    if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        // Could not connect
        perror("Could not connect");
        close(socket_fd);
        return;
    }

    printf("%s\n", request.info);
    send(socket_fd, &request, sizeof(request), 0);
    insert_log(STORAGE_SERVER_COMMUNICATION, request.SS_id, port, request.type, request.info, request.error_code);
    close(socket_fd);
}

// Response handling
void send_response(int sock, RequestType type, ErrorCode error, const char *info, int ss_id)
{
    if (ss_id >= 0)
    {
        SS_Request response = {
            .type = type,
            .error_code = error,
            .SS_id = ss_id};
        strncpy(response.info, info ? info : "", MAX_INFO_SIZE - 1);
        // send(sock, &response, sizeof(SS_Request), 0);
        connect_and_send_to_SS(response);
    }
    else
    {
        Request response = {
            .type = type,
            .error_code = error,
        };
        strncpy(response.info, info ? info : "", MAX_INFO_SIZE - 1);
        printf("%s %ld\n", response.info, strlen(response.info));
        int bytes_sent = send(sock, &response, sizeof(Request), 0);
        printf("Sent %d bytes to client\n", bytes_sent);
        insert_log(CLIENT_COMMUNICATION, 69, 69, response.type, response.info, response.error_code);
    }
}

// Helper function to send request to storage server
Request send_request_to_ss(int ss_id, SS_Request request)
{
    int thread_id = request.Thread_id;
    Request c_res;
    c_res.type = request.type;
    memset(c_res.info, 0, sizeof(c_res.info));

    ss_connection_t *conn = &g_ss_connections[ss_id];
    connect_and_send_to_SS(request);

    while(g_requests[thread_id].type == WAITING) {
        usleep(10000);
    }

    g_requests[thread_id].info[sizeof(g_requests[thread_id])] = '\0';
    c_res.type = g_requests[thread_id].type;
    c_res.error_code = g_requests[thread_id].error_code;
    strcpy(c_res.info, g_requests[thread_id].info);
    printf("ErrorCode : -> %d\n", c_res.error_code);
    return c_res;
}

// Thread function to handle client requests
static void *handle_client_connection(void *arg)
{
    client_thread_args *args = (client_thread_args *)arg;
    int client_sock = args->client_socket;
    free(args);
    
    Request request_m;
    // request_m.type = REQUEST_FILE_NOT_FOUND;
    // request_m.error_code = OK;
   // strcpy(request_m.info, "hahahahahahahahaKKKKKKK");
    // usleep(10000);
    // if(send(client_sock, &request_m, sizeof(Request), 0) < 0){
    //     printf("ERRRROR CYKA\n");
    //     close(client_sock);
    //     return NULL;
    // }
    Request request;
    memset(&request, 0, sizeof(Request));
    ssize_t bytes_read = recv(client_sock, &request, sizeof(Request), MSG_WAITALL);
    insert_log(CLIENT_COMMUNICATION, 96, 120001, request.type, request.info, request.error_code);

    if (bytes_read <= 0)
    {
        close(client_sock);
        return NULL;
    }

    printf("Received request: (%d) %s\n", request.type, request.info);
    printf("\n\n\n\n\n");
    send_response(client_sock, ACK,OK,"NULL",-1);
    Request response = handle_request(request, args->thread_id, client_sock);

    // usleep(10000);
    printf("to send : %s %ld\n", response.info, strlen(response.info));
    if(send(client_sock, &response, sizeof(Request), 0) < 0){
        printf("ERRRROR CYKA\n");
        close(client_sock);
        return NULL;
    }
    //send_response(client_sock, response.type, response.error_code, response.info, -1);

    if (request.type == REQUEST_WRITE && response.error_code == OK){
        char *token = strtok(response.info, "|");
        int ss_id = -1;
        for(int i = 0; i < g_ss_count; i++){
            if(strcmp (g_storage_servers[i].ip, token) == 0){
                ss_id = i;
                break;
            }
        }
        bool_pair check;
        int count = 0;
        //wait for write to start
        while(check_file_lock(request.info, true).one == true ){}
        printf("Write ready to be started\n");
        while(check_file_lock(request.info, true).three == false){}
        check = check_file_lock(request.info, true);
        if(check.two){
            printf("It was sync write!\n");
            close(client_sock);
            return NULL;
        }
        while(check.one == false && g_storage_servers[ss_id].is_alive){
            usleep(10000);
            // printf("Async write going on\n");
            check = check_file_lock(request.info, true);
        }
        if(!g_storage_servers[ss_id].is_alive){
            printf("Server died during writing\n");
            release_file_lock(request.info);
            send_response(client_sock, ACK_ASYNC, WRITE_FAILED, strcat(request.info, ": Async write failed."), -1);
            close(client_sock);
            return NULL;
        }
        printf("Async Write ended\n");
        send_response(client_sock, ACK_ASYNC, OK, strcat(request.info, ": Async write completed."), -1);
    }
    close(client_sock);

    pthread_mutex_lock(&g_request_threads);

    // Validate the thread ID
    if (args->thread_id < 0 || args->thread_id >= MAX_CLIENTS) {
        fprintf(stderr, "Invalid thread ID: %d\n", args->thread_id);
        pthread_mutex_unlock(&g_request_threads);
        return NULL;
    }

    // Reset the slot to default values
    g_requests[args->thread_id].type = WAITING; // Or use a clearer default state like IDLE
    g_requests[args->thread_id].error_code = OK;
    memset(g_requests[args->thread_id].info, 0, sizeof(g_requests[args->thread_id].info));
    g_requests[args->thread_id].Thread_id = -1;

    pthread_mutex_unlock(&g_request_threads);
    return NULL;
}

static void *handle_ss_communication(void *arg)
{
    ss_connection_t *conn = (ss_connection_t *)arg;
    
    while (1)
    {
        SS_Request request;
        memset(request.info, 0, sizeof(request.info));
        ssize_t bytes_read = recv(conn->socket, &request, sizeof(SS_Request), MSG_WAITALL);
        insert_log(STORAGE_SERVER_COMMUNICATION, conn->ss_id, 12000, request.type, request.info, request.error_code);
        // if (bytes_read <= 0)
        // {
        //     // pthread_mutex_lock(&g_ss_lock);
        //     // g_storage_servers[conn->ss_id].is_alive = false;
        //     // pthread_mutex_unlock(&g_ss_lock);
        //     // break;
        //     continue;
        // }

        if(request.Thread_id != -1) {
            pthread_mutex_lock(&g_request_threads);
            strcpy(g_requests[request.Thread_id].info, request.info);
            g_requests[request.Thread_id].error_code = request.error_code;
            g_requests[request.Thread_id].type = request.type;
            pthread_mutex_unlock(&g_request_threads);
        }

        SS_Request response;
        response = handle_ss_request(conn->ss_id, request);
        // send_request_to_ss(conn->ss_id, response);

        if(!g_storage_servers[conn->ss_id].is_alive)
            break;
    }

    close(conn->socket);
    pthread_mutex_destroy(&conn->send_lock);
    return NULL;
}

// Thread function to handle storage server registration and updates
static void *handle_storage_server_connection(void *arg)
{
    client_thread_args *args = (client_thread_args *)arg;
    int ss_sock = args->client_socket;
    free(args);

    SS_Request request;
    memset(request.info, 0, sizeof(request.info));
    ssize_t bytes_read = recv(ss_sock, &request, sizeof(SS_Request), MSG_WAITALL);
    insert_log(STORAGE_SERVER_COMMUNICATION, request.SS_id, 12000, request.type, request.info, request.error_code);

    if(request.Thread_id != -1) {
        pthread_mutex_lock(&g_request_threads);
        strcpy(g_requests[request.Thread_id].info, request.info);
        g_requests[request.Thread_id].error_code = request.error_code;
        g_requests[request.Thread_id].type = request.type;
        pthread_mutex_unlock(&g_request_threads);
    } else if(request.type == REQUEST_REGESTER){
        if (bytes_read <= 0 || request.type != REQUEST_REGESTER)
        {
            close(ss_sock);
            return NULL;
        }
        
        char ip[MAX_IP_SIZE];
        int nm_port, client_port, ss_port;
        char paths[MAX_INFO_SIZE];

        strcpy(ip, strtok(request.info, "|"));
        nm_port = atoi(strtok(NULL, "|"));
        client_port = atoi(strtok(NULL, "|"));
        ss_port = atoi(strtok(NULL, "|"));

        printf("%s %d %d\n", ip, nm_port, client_port);
        int ss_id = ns_register_storage_server(ip, nm_port, client_port, ss_port);

        if (ss_id >= 0)
        {
            printf("Storage server %d connected\n", ss_id);
            g_ss_connections[ss_id].socket = ss_sock;
            g_ss_connections[ss_id].ss_id = ss_id;
            pthread_mutex_init(&g_ss_connections[ss_id].send_lock, NULL);
            //backup();
            send_response(ss_sock, REQUEST_REGESTER, OK, NULL, ss_id);

            
            // Start communication thread
            pthread_t comm_thread;
            pthread_create(&comm_thread, NULL, handle_ss_communication, &g_ss_connections[ss_id]);
            pthread_detach(comm_thread);

            // Start heartbeat monitoring for this SS
            pthread_mutex_lock(&g_ss_lock);
            int* ss_id_ptr = malloc(sizeof(int));
            *ss_id_ptr = ss_id;
            pthread_create(&g_storage_servers[ss_id].ping_thread, NULL, ping_storage_server, ss_id_ptr);
            pthread_detach(g_storage_servers[ss_id].ping_thread);

            // SS_Request create_backup2;
            // create_backup2.type = REQUEST_CREATE;
            // create_backup2.error_code = OK;
            // create_backup2.SS_id = ss_id;
            // create_backup2.Thread_id = -1;
            // strcpy(create_backup2.info, "");
            // strcat(create_backup2.info, "/|");
            // strcat(create_backup2.info, "Backup/");
            // // Request req2 = send_request_to_ss(ss_id, create_backup2);
            // connect_and_send_to_SS(create_backup2);
            // printf("Error code create backup2: %d\n", req2.error_code);
            pthread_mutex_unlock(&g_ss_lock);
        }
        else
        {
            send_response(ss_sock, REQUEST_REGESTER, CONNECTION_FAILED, NULL, MAX_STORAGE_SERVERS);
            close(ss_sock);
        }
    } else {
        SS_Request response;
        response = handle_ss_request(request.SS_id, request);
        connect_and_send_to_SS(response);
    }
}

// Main network listening function
void *start_network_listener(void *arg)
{
    // Initialize client and naming server sockets
    g_nm_socket = init_socket(g_server_listen_port); // Storage server port
    g_client_socket = init_socket(g_client_listen_port);

    if (g_client_socket < 0 || g_nm_socket < 0)
    {
        fprintf(stderr, "Failed to initialize sockets\n");
        return NULL;
    }

    printf("Naming server listening on ports %d (clients) and %d (storage servers)\n",
           g_client_listen_port, g_server_listen_port);

    while (1)
    {
        // printf("wht the fuck is happening\n");
        fd_set readfds;
        // printf("wht the fuck is happening 2\n");
        FD_ZERO(&readfds);
        // printf("wht the fuck is happening 3\n");
        FD_SET(g_client_socket, &readfds);
        // printf("wht the fuck is happening 4\n");
        FD_SET(g_nm_socket, &readfds);
        // printf("wht the fuck is happening 5\n");
        int max_fd = (g_client_socket > g_nm_socket) ? g_client_socket : g_nm_socket;
        //printf("wht the fuck is happening 1\n");
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("Select failed");
            continue;
        }
        //printf("wht the fuck is happening 2\n");

        if (FD_ISSET(g_client_socket, &readfds))
        {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_socket = accept(g_client_socket, (struct sockaddr *)&client_addr, &addr_len);

            if (new_socket >= 0)
            {
                int j = -1;

                pthread_mutex_lock(&g_request_threads);
                for(int i = 0; i<MAX_CLIENTS; i++) {
                    if(g_requests[i].Thread_id == -1) {
                        j = i;
                        g_requests[i].Thread_id = i;
                        break;
                    }
                }
                pthread_mutex_unlock(&g_request_threads);

                if(j == -1) {
                    // decline somehow
                } else {
                    client_thread_args *args = malloc(sizeof(client_thread_args));
                    args->client_socket = new_socket;
                    args->thread_id = j;
                    pthread_create(&g_client_threads[j], NULL, handle_client_connection, args);
                    pthread_detach(g_client_threads[j]);
                }
            }
        }
        //printf("wht the fuck is happening 3\n");

        if (FD_ISSET(g_nm_socket, &readfds))
        {
            printf("ss registeration attempted\n");
            struct sockaddr_in ss_addr;
            socklen_t addr_len = sizeof(ss_addr);
            int new_socket = accept(g_nm_socket, (struct sockaddr *)&ss_addr, &addr_len);

            if (new_socket >= 0)
            {
                client_thread_args *args = malloc(sizeof(client_thread_args));
                args->client_socket = new_socket;

                pthread_t ss_thread;
                pthread_create(&ss_thread, NULL, handle_storage_server_connection, args);
                pthread_detach(ss_thread);
            }
        }
    }

    return NULL;
}

// Periodic lock cleanup function
// static void* cleanup_lock_thread(void* arg) {
//     while (1) {
//         sleep(LOCK_CLEANUP_INTERVAL);
//         cleanup_locks(g_lock_manager, LOCK_MAX_AGE);
//     }
//     return NULL;
// }

// // Main connection handler
// void* handle_connections(void* arg) {
//     int server_sock = *(int*)arg;
//     struct sockaddr_in client_addr;
//     socklen_t addr_len = sizeof(client_addr);

//     // Start lock cleanup thread
//     pthread_t cleanup_thread;
//     pthread_create(&cleanup_thread, NULL, cleanup_lock_thread, NULL);
//     pthread_detach(cleanup_thread);

//     while (1) {
//         int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
//         if (client_sock < 0) continue;

//         // Create thread to handle client connection
//         pthread_t handler_thread;
//         int* sock_ptr = malloc(sizeof(int));
//         *sock_ptr = client_sock;

//         // First message determines if this is a SS registration or client request
//         Request peek_request;
//         ssize_t peek_size = recv(client_sock, &peek_request, sizeof(Request), MSG_PEEK);

//         if (peek_size == sizeof(Request)) {
//             if (peek_request.type == REQUEST_REGISTER) {
//                 pthread_create(&handler_thread, NULL,
//                              (void* (*)(void*))handle_storage_server_registration,
//                              sock_ptr);
//             } else {
//                 pthread_create(&handler_thread, NULL,
//                              (void* (*)(void*))handle_client_request,
//                              sock_ptr);
//             }
//             pthread_detach(handler_thread);
//         } else {
//             close(client_sock);
//             free(sock_ptr);
//         }
//     }

//     return NULL;
// }
