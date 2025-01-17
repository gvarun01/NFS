#include "header.h"
typedef struct
{
    int client_sock;
    struct sockaddr_in client_addr;
} ClientThreadArgs;

void make_SS_socket()
{
    int ss_sock; // Sockets for SS communication
    struct sockaddr_in ss_addr;

    // Initialize and bind the SS socket
    ss_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ss_sock < 0)
    {
        perror("SS socket creation failed");
        return;
    }
    memset(&ss_addr, 0, sizeof(ss_addr));
    ss_addr.sin_family = AF_INET;
    ss_addr.sin_addr.s_addr = inet_addr(ss_ip);
    ss_addr.sin_port = htons(ss_client_port + 2); // Port for SS communication
    if (bind(ss_sock, (struct sockaddr *)&ss_addr, sizeof(ss_addr)) < 0)
    {
        perror("SS socket bind failed");
        close(ss_sock);
        return;
    }
    if (listen(ss_sock, 500) < 0)
    {
        perror("SS socket listen failed");
        close(ss_sock);
        return;
    }

    pthread_t ss_listener_thread;
    if (pthread_create(&ss_listener_thread, NULL, ss_listener, &ss_sock) != 0)
    {
        perror("Failed to create SS listener thread");
        close(ss_sock);
        return;
    }

    pthread_detach(ss_listener_thread);
}

// Function to listen for connections and spawn threads
void *ss_listener(void *arg)
{
    int ss_sock = *(int *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t client_thread;

    printf("Listening for SS connections...\n");

    while (1)
    {
        int *client_sock_ptr = malloc(sizeof(int));
        if (client_sock_ptr == NULL)
        {
            perror("Failed to allocate memory for client socket");
            continue;
        }

        *client_sock_ptr = accept(ss_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (*client_sock_ptr < 0)
        {
            perror("Failed to accept SS connection");
            free(client_sock_ptr);
            continue;
        }

        printf("New SS connected.\n");

        pthread_t ss_thread;
        if (pthread_create(&ss_thread, NULL, ss_thread_handler, client_sock_ptr) != 0)
        {
            perror("Failed to create SS thread");
            close(*client_sock_ptr);
            free(client_sock_ptr);
        }
        else
        {
            pthread_detach(ss_thread);
        }
    }

    close(ss_sock);
    pthread_exit(NULL);
}

// Thread handler for ss requests
void *ss_thread_handler(void *arg)
{
    int client_socket = *(int *)arg;
    // free(arg); // Free dynamically allocated memory

    printf("SS connected\n");
    handle_ss_request(client_socket);
    close(client_socket);
    pthread_exit(NULL);
}

void handle_ss_request(int client_sock)
{
    SS_Request client_request;
    memset(&client_request, 0, sizeof(SS_Request));
    if (recv(client_sock, &client_request, sizeof(SS_Request), MSG_WAITALL) <= 0)
    {
        printf("Error receiving client request\n");
        return;
    }

    if (client_request.error_code != OK)
    {
        printf("Error code: %d\n", client_request.error_code);
        return;
    }

    if (client_request.type == REQUEST_PASTE)
    {
        printf("HUDBCSHJB\n\n\n\n");
        // Handle READ request by reading the specified file and sending it to the client
        ss_copy_file_or_directory(client_sock, &client_request);
        // pnt_ss(client_request, client_sock);
    }
    else
    {
        printf("Unknown operation requested by client.\n");
        Packet error_packet = {.error_code = UNKNOWN_OPERATION};
        send(client_sock, &error_packet, sizeof(error_packet), 0);
    }
}

int ss_connect_to_ss(int client_sock, char *source_ip, int source_port, int thread_id)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(source_port);
    inet_pton(AF_INET, source_ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        send_to_server(CONNECTION_FAILED, COPY_COMPLETED, "Connection to source SS failed", thread_id);
        close(sock);
        return -1;
    }
    return sock;
}

// void ss_paste_file_or_directory(int client_sock, SS_Request *request)
// {
//     int srcPortNo;
//     char src[MAX_PATH_LENGTH], dest[MAX_PATH_LENGTH], srcIp[MAX_IP_SIZE];
//     strcpy(src, strtok(request->info, "|"));
//     strcpy(dest, strtok(NULL, "|"));
//     strcpy(srcIp, strtok(NULL, "|"));
//     srcPortNo = atoi(strtok(NULL, "|"));

//     send_to_server(OK, COPY_STARTED, src, -1);
//     struct stat src_stat;

//     printf("Copying %s to %d\n", srcIp, srcPortNo);

//     int source_sock = ss_connect_to_ss(client_sock, srcIp, srcPortNo, request->Thread_id);
//     if (source_sock < 0)
//     {
//         return;
//     }

//     SS_Request packet;
//     memset(&packet, 0, sizeof(packet));
//     packet.error_code = OK;
//     packet.type = REQUEST_PASTE;
//     strcpy(packet.info, src);
//     send(source_sock, &packet, sizeof(SS_Request), 0);

//     memset(&packet, 0, sizeof(packet));
//     recv(source_sock, &packet, sizeof(packet), 0);

//     if (packet.error_code != OK)
//     {
//         printf("Error %s\n", error_names[packet.error_code]);
//         send_to_server(packet.error_code, COPY_COMPLETED, src, request->Thread_id);
//         close(source_sock);
//         return;
//     }

//     if (packet.info[strlen(packet.info) - 1] == '/')
//     {
//         ss_paste_directory(client_sock, source_sock, dest, request->Thread_id);
//     }
//     else
//     {
//         if (ss_paste_file(client_sock, source_sock, dest, request->Thread_id) < 0)
//         {
//             return;
//         }
//         else
//         {
//             send_to_server(OK, COPY_COMPLETED, src, request->Thread_id);
//         }
//     }

//     close(source_sock);
// }

// int ss_paste_file(int client_sock, int source_sock, char *dest, int thread_id)
// {
//     SS_Request packet;
//     int dest_fd = open(dest, O_CREAT | O_WRONLY | O_APPEND, 0644);
//     if (dest_fd < 0)
//     {
//         perror("Error creating destination file");
//         send_to_server(FILE_NOT_FOUND, COPY_COMPLETED, dest, thread_id);
//         return -1;
//     }

//     lseek(dest_fd, 0, SEEK_SET);

//     while (1)
//     {
//         memset(&packet, 0, sizeof(packet));
//         recv(source_sock, &packet, sizeof(packet), 0);

//         if (packet.error_code != OK)
//         {
//             printf("Error %s\n", error_names[packet.error_code]);
//             send_to_server(packet.error_code, COPY_COMPLETED, dest, thread_id);
//             close(source_sock);
//             return -1;
//         }

//         if (packet.type == ACK)
//         {
//             break;
//         }

//         write(dest_fd, packet.info, strlen(packet.info));
//     }
//     close(dest_fd);
// }

// void ss_paste_directory(int client_sock, int source_sock, char *dest, int thread_id)
// {
//     struct stat st;
//     if (stat(dest, &st) == -1)
//     {
//         mkdir(dest, 0755);
//     }

//     SS_Request packet;
//     while (1)
//     {
//         memset(&packet, 0, sizeof(packet));
//         recv(source_sock, &packet, sizeof(packet), 0);

//         if (packet.error_code != OK)
//         {
//             printf("Error %s\n", error_names[packet.error_code]);
//             send_to_server(packet.error_code, COPY_COMPLETED, dest ,thread_id);
//             close(source_sock);
//             return;
//         }

//         if (packet.type == ACK)
//         {
//             break;
//         }

//         char file_path[MAX_PATH_LENGTH];
//         snprintf(file_path, sizeof(file_path), "%s/%s", dest, packet.info);

//         if (packet.info[strlen(packet.info) - 1] == '/')
//         {
//             mkdir(file_path, 0755);
//         }
//         else
//         {
//             ss_paste_file(client_sock, source_sock, file_path, thread_id);
//         }
//     }
// }

void ss_paste_file_or_directory(int client_sock, SS_Request *request)
{
    int srcPortNo;
    char src[MAX_PATH_LENGTH], dest[MAX_PATH_LENGTH], srcIp[MAX_IP_SIZE];
    strcpy(src, strtok(request->info, "|"));
    strcpy(dest, strtok(NULL, "|"));
    strcpy(srcIp, strtok(NULL, "|"));
    srcPortNo = atoi(strtok(NULL, "|"));

    send_to_server(OK, COPY_STARTED, src, -1);
    printf("Copying from %s:%d to %s\n", srcIp, srcPortNo, dest);

    int source_sock = ss_connect_to_ss(client_sock, srcIp, srcPortNo, request->Thread_id);
    if (source_sock < 0)
    {
        send_to_server(CONNECTION_FAILED, COPY_COMPLETED, src, request->Thread_id);
        return;
    }

    SS_Request packet;
    memset(&packet, 0, sizeof(packet));
    packet.error_code = OK;
    packet.type = REQUEST_PASTE;
    strcpy(packet.info, src);
    send(source_sock, &packet, sizeof(SS_Request), 0);

    memset(&packet, 0, sizeof(packet));
    recv(source_sock, &packet, sizeof(packet), MSG_WAITALL);

    char cwd[MAX_PATH_LENGTH];
    getcwd(cwd, sizeof(cwd));

    chdir(root_directory);
    chdir(dest);

    if (packet.error_code != OK)
    {
        printf("Error: %s\n", error_names[packet.error_code]);
        send_to_server(packet.error_code, COPY_COMPLETED, src, request->Thread_id);
        close(source_sock);
        return;
    }

    if (packet.info[strlen(packet.info) - 1] == '/')
    {
        // Source is a directory
        ss_paste_directory(client_sock, source_sock, packet.info, request->Thread_id, dest);
    }
    else
    {
        // Source is a file
        if (ss_paste_file(client_sock, source_sock, packet.info, request->Thread_id) < 0)
        {
            close(source_sock);
            return;
        }

        char pth[MAX_PATH_LENGTH];
        memset(pth, 0, sizeof(pth));
        strcpy(pth, dest);
        strcat(pth, packet.info);

        printf("ADD %s\n", pth);

        // send_to_server(OK, REQUEST_ADD_PATH, pth, -1);
        if(strstr(pth, "Backup") == NULL){
            send_to_server(OK, REQUEST_ADD_PATH, pth, -1);
        }
    }

    chdir(cwd);

    send_to_server(OK, COPY_COMPLETED, src, request->Thread_id);
    close(source_sock);
}

int ss_paste_file(int client_sock, int source_sock, char *dest, int thread_id)
{
    SS_Request packet;
    memset(&packet, 0, sizeof(packet));

    // Open destination file (create or truncate)
    printf("FILE   %s\n", dest);
    int dest_fd = open(dest, O_CREAT | O_WRONLY, 0644);
    if (dest_fd < 0)
    {
        perror("Error creating destination file 2");
        send_to_server(FILE_NOT_FOUND, COPY_COMPLETED, dest, thread_id);
        return -1;
    }

    while (1)
    {
        memset(&packet, 0, sizeof(packet));
        ssize_t received = recv(source_sock, &packet, sizeof(packet), MSG_WAITALL);
        if (received <= 0)
        {
            perror("Error receiving data from source");
            send_to_server(REQ, COPY_COMPLETED, dest, thread_id);
            close(dest_fd);
            return -1;
        }

        // printf("%s\n", packet.info);

        if (write(dest_fd, packet.info, strlen(packet.info)) < 0)
        {
            perror("Error writing to destination file");
            send_to_server(WRITE_FAILED, COPY_COMPLETED, dest, thread_id);
            close(dest_fd);
            return -1;
        }

        if (packet.type == FILE_COMPLETE)
        {
            // End of file transfer
            break;
        }
    }

    close(dest_fd);
    return 0;
}

void ss_paste_directory(int client_sock, int source_sock, char *dest, int thread_id, char *base)
{
    printf("DIR   %s\n", dest);
    struct stat st;
    if (stat(dest, &st) == -1)
    {
        if (mkdir(dest, 0755) < 0)
        {
            perror("Error creating destination directory");
            send_to_server(DIR_CREATION_FAILED, COPY_COMPLETED, dest, thread_id);
            return;
        }
    }

    char pth[MAX_PATH_LENGTH];
    memset(pth, 0, sizeof(pth));
    strcpy(pth, base);
    strcat(pth, dest);

    if(strstr(pth, "Backup") == NULL){
        send_to_server(OK, REQUEST_ADD_PATH, pth, -1);
    }

    SS_Request packet;
    memset(&packet, 0, sizeof(packet));

    while (1)
    {
        memset(&packet, 0, sizeof(packet));
        ssize_t received = recv(source_sock, &packet, sizeof(packet), MSG_WAITALL);
        printf("DIR   %s\n", packet.info);
        if (received <= 0)
        {
            perror("Error receiving data from source");
            send_to_server(REQ, COPY_COMPLETED, dest, thread_id);
            return;
        }

        if (packet.type == ACK)
        {
            // End of directory transfer
            break;
        }

        char file_path[MAX_PATH_LENGTH];
        snprintf(file_path, sizeof(dest) + sizeof(packet.info), "%s", packet.info);

        printf("%s\n", file_path);

        if (packet.info[strlen(packet.info) - 1] == '/')
        {
            // Handle subdirectory
            if (stat(file_path, &st) == -1)
            {
                if (mkdir(file_path, 0755) < 0)
                {
                    perror("Error creating subdirectory");
                    send_to_server(DIR_CREATION_FAILED, COPY_COMPLETED, file_path, thread_id);
                    continue;
                }
            }
        }
        else
        {
            // Handle file
            if (ss_paste_file(client_sock, source_sock, file_path, thread_id) < 0)
            {
                send_to_server(REQ, COPY_COMPLETED, file_path, thread_id);
                return;
            }
        }

        char pth[MAX_PATH_LENGTH];
        memset(pth, 0, sizeof(pth));
        strcpy(pth, base);
        strcat(pth, file_path + 2);

        printf("ADD %s\n", pth);

        if(strstr(pth, "Backup") == NULL){
            send_to_server(OK, REQUEST_ADD_PATH, pth, -1);
        }
        // send_to_server(OK, REQUEST_ADD_PATH, pth, -1);
    }
}

// Copy file or directory based on type
void ss_copy_file_or_directory(int client_sock, SS_Request *request)
{
    send_to_server(OK, PASTE_STARTED, request->info, -1);

    struct stat src_stat;
    char path[MAX_PATH_LENGTH];
    if(request->info[0] == '\\'){
        snprintf(path, sizeof(path), "%s", root_directory); // Adjust the root directory
    } else {
        snprintf(path, sizeof(path), "%s%s", root_directory, request->info + 2); // Adjust the root directory
    }

    printf("%s\n", path);

    if (stat(path, &src_stat) != 0)
    {
        perror("Error getting stat of source path 2");
        send_to_server(STAT_FAILED, PASTE_COMPLETED, request->info, request->Thread_id);
        return;
    }

    char *full_path = request->info;
    char *name = strrchr(full_path, '/'); // Find the last '/'
    if (name && *(name + 1))
    {           // Ensure there is something after the '/'
        name++; // Move past the '/' to get the name
    }
    else
    {
        // Handle the error: no '/' or nothing after '/'
        name = ""; // Sending an empty string
    }

    if (S_ISDIR(src_stat.st_mode))
    {
        // SS_Request name_packet = {.error_code = OK, .info = "", .type = REQUEST_PASTE};
        // strncpy(name_packet.info, name, sizeof(name_packet.info) - 1); // Copy the name
        // name_packet.info[strlen(name_packet.info)] = '/';        // Null-terminate
        // name_packet.info[strlen(name_packet.info) + 1] = '\0';        // Null-terminate
        // send(client_sock, &name_packet, sizeof(name_packet), 0);
        ss_copy_directory(client_sock, path, -1, request->type);
    }
    else
    {
        // SS_Request name_packet = {.error_code = OK, .info = "", .type = REQUEST_PASTE};
        // strncpy(name_packet.info, name, sizeof(name_packet.info) - 1); // Copy the name
        // name_packet.info[sizeof(name_packet.info)] = '\0';        // Null-terminate
        // send(client_sock, &name_packet, sizeof(name_packet), 0);
        ss_copy_file(client_sock, path, 0);
    }

    SS_Request packet;
    memset(&packet, 0, sizeof(packet));
    packet.error_code = OK;
    packet.type = ACK;
    strcpy(packet.info, request->info);
    send(client_sock, &packet, sizeof(packet), 0);
    send_to_server(OK, PASTE_COMPLETED, request->info, request->Thread_id);
}

// int ss_copy_file(int client_sock, char *src)
// {
//     SS_Request packet;
//     memset(&packet, 0, sizeof(packet));
//     printf("%s\n", src);
//     int dest_fd = open(src, O_RDONLY);
//     if (dest_fd < 0)
//     {
//         perror("Error creating destination file");
//         SS_Request error_packet = {.error_code = FILE_NOT_FOUND};
//         send(client_sock, &error_packet, sizeof(error_packet), 0);
//         return -1;
//     }

//     printf("NO\n");

//     off_t file_size = lseek(dest_fd, 0, SEEK_END);
//     lseek(dest_fd, 0, SEEK_SET);
//     int total_packets = (file_size + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;
//     int packet_number = 0;

//     char *file_name = malloc(MAX_PATH_LENGTH);
//     char *name;
//     strcpy(file_name, src);
//     name = strrchr(file_name, '/');
//     if (name != NULL)
//     {
//         // Move pointer past the '/'
//         name++;
//     }
//     else
//     {
//         // If no '/' is found, the path itself is the file name
//         name = file_name;
//     }

//     SS_Request size_packet = {.error_code = OK, .type = REQUEST_PASTE_FILE};
//     strcpy(size_packet.info, name);
//     send(client_sock, &size_packet, sizeof(size_packet), 0);

//     ssize_t bytes_read;

//     while ((bytes_read = read(dest_fd, packet.info, BUFFER_SIZE)) > 0)
//     {
//         packet.error_code = OK; // No error

//         // Send the packet to the client
//         ssize_t bytes_sent = send(client_sock, &packet, sizeof(SS_Request), 0);
//         if (bytes_sent < 0)
//         {
//             perror("Error sending packet to client");
//             return -1;
//         }

//         memset(&packet, 0, sizeof(packet));
//     }

//     close(dest_fd);
//     return 0;
// }

// Copy file to client
int ss_copy_file(int client_sock, char *src, int start)
{
    SS_Request packet;
    memset(&packet, 0, sizeof(packet));
    SS_Request name_packet = {.error_code = OK, .info = "", .type = REQUEST_PASTE};

    if(start != 0){
        strncpy(packet.info, src + start, BUFFER_SIZE - 1);
        int len = strlen(packet.info);
        memmove(packet.info + 2, packet.info, len + 1); // +1 to include the null terminator

        // Prepend "./" at the beginning
        packet.info[0] = '.';
        packet.info[1] = '/';
        strncpy(name_packet.info, packet.info, sizeof(name_packet.info) - 1);
    }
    else{
        char * save_source = (char *)malloc(MAX_PATH_LENGTH);
        strcpy(save_source, src);

        char *full_path = save_source;

        if(full_path[strlen(full_path)-1] == '/'){
            full_path[strlen(full_path)-1] = '\0';
        }

        char *name = strrchr(full_path, '/'); // Find the last '/'
        char *last_name;
        if (name && *(name + 1)) {            // Ensure there is something after the '/'
            last_name = name;
            name++;                           // Move past the '/' to get the name
        } else {
            // Handle the error: no '/' or nothing after '/'
            name = ""; // Sending an empty string
        }

        strncpy(name_packet.info, name, sizeof(name_packet.info) - 1);
    }

    name_packet.info[sizeof(name_packet.info)] = '\0';                    // Null-terminate
    send(client_sock, &name_packet, sizeof(name_packet), 0);

    printf("Sent file: %s\n", name_packet.info);

    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0)
    {
        perror("Error opening source file");
        SS_Request error_packet = {.error_code = FILE_NOT_FOUND};
        send(client_sock, &error_packet, sizeof(error_packet), 0);
        return -1;
    }

    struct stat file_stat;
    if (fstat(src_fd, &file_stat) < 0)
    {
        perror("Error retrieving file stats");
        close(src_fd);
        return -1;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, packet.info, MAX_PACKET_SIZE)) > 0)
    {
        packet.error_code = OK; // No error
        if (send(client_sock, &packet, sizeof(SS_Request), 0) < 0)
        {
            perror("Error sending packet to client");
            close(src_fd);
            return -1;
        }
        memset(&packet, 0, sizeof(packet)); // Clear packet buffer
    }

    packet.type = FILE_COMPLETE;
    packet.error_code = OK; // No error
    if (send(client_sock, &packet, sizeof(SS_Request), 0) < 0)
    {
        perror("Error sending packet to client");
        close(src_fd);
        return -1;
    }

    close(src_fd);
    return 0;
}

// Copy directory to client
// int ss_copy_directory(int client_sock, char *src) {
//     SS_Request packet;
//     memset(&packet, 0, sizeof(packet));

//     DIR *dir = opendir(src);
//     if (dir == NULL) {
//         perror("Error opening source directory");
//         SS_Request error_packet = {.error_code = DIR_DOESNT_EXIST};
//         send(client_sock, &error_packet, sizeof(error_packet), 0);
//         return -1;
//     }

//     struct dirent *entry;
//     char src_path[BUFFER_SIZE];
//     while ((entry = readdir(dir)) != NULL) {
//         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
//             continue; // Skip current and parent directories
//         }

//         snprintf(src_path, BUFFER_SIZE, "%s%s", src, entry->d_name);

//         struct stat entry_stat;
//         if (stat(src_path, &entry_stat) < 0) {
//             perror("Error retrieving entry info");
//             continue;
//         }

//         if (S_ISDIR(entry_stat.st_mode)) {
//             ss_copy_directory(client_sock, src_path); // Recursive copy
//         } else if (S_ISREG(entry_stat.st_mode)) {
//             ss_copy_file(client_sock, src_path); // Copy file
//         }
//     }

//     closedir(dir);
//     return 0;
// }

int ss_copy_directory(int client_sock, char *src, int start, RequestType reqType)
{
    SS_Request packet;
    memset(&packet, 0, sizeof(packet));

    printf("YES\n");
    DIR *dir = opendir(src);
    if (dir == NULL)
    {
        perror("Error opening source directory");
        SS_Request error_packet = {.error_code = DIR_DOESNT_EXIST};
        send(client_sock, &error_packet, sizeof(error_packet), 0);
        return -1;
    }

    struct dirent *entry;
    struct stat entry_stat;

    if (start == -1)
    {
        char *last_dir;

        size_t len = strlen(src);
        if (len > 0 && src[len - 1] == '/' && reqType != REQUEST_BACKUP_COPY)
        {
            src[len - 1] = '\0';
        }

        last_dir = strrchr(src, '/');
        if (last_dir != NULL)
        {
            // Move pointer past the '/'
            start = last_dir - src + 1;
            last_dir++;
        }
        else
        {
            // If no '/' is found, the path itself is the last directory
            last_dir = src;
            start = 0;
        }

        if(reqType == REQUEST_BACKUP_COPY) {
            start = last_dir - src + 1;
            last_dir++;
            start = strlen(root_directory) - 1;
        }
        
        strncpy(packet.info, last_dir, BUFFER_SIZE - 1);

    }
    else
    {
        strncpy(packet.info, src + start, BUFFER_SIZE - 1);
        int len = strlen(packet.info);
        memmove(packet.info + 2, packet.info, len + 1); // +1 to include the null terminator

        // Prepend "./" at the beginning
        packet.info[0] = '.';
        packet.info[1] = '/';
    }

    // Send the directory name as a packet
    packet.error_code = OK;
    packet.info[strlen(packet.info)] = '/';
    packet.info[strlen(packet.info) + 1] = '\0'; // Indicate it's a directory name
    packet.type = RESPONSE;
    if(is_path_accessible(packet.info + strlen(root_directory) - 1, accessiblePathsHead)){
        if (send(client_sock, &packet, sizeof(packet), 0) < 0)
        {
            perror("Error sending directory packet");
            closedir(dir);
            return -1;
        }
    } else {
        printf("Skipping inaccessible path: %s\n", packet.info);
        memset(src, 0, sizeof(src));
    }
    printf("Sent the directory: %s\n", packet.info);

    printf("YES\n");

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue; // Skip current and parent directories
        }

        char src_path[BUFFER_SIZE];
        snprintf(src_path, BUFFER_SIZE, "%s/%s", src, entry->d_name);

        printf("%s %s\n", src_path, root_directory);

        // Check if the path is accessible
        if (!is_path_accessible(src_path + strlen(root_directory) - 1, accessiblePathsHead)) {
            printf("Skipping inaccessible path: %s\n", src_path);
            continue; // Skip inaccessible paths
        }

        printf("Copying %s\n", src_path);

        if (stat(src_path, &entry_stat) < 0)
        {
            perror("Error retrieving entry info");
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode))
        {
            // Recursively send subdirectory
            if (ss_copy_directory(client_sock, src_path, start, reqType) < 0)
            {
                perror("Error sending subdirectory");
            }
        }
        else if (S_ISREG(entry_stat.st_mode))
        {
            // Send file
            if (ss_copy_file(client_sock, src_path, start) < 0)
            {
                perror("Error sending file");
            }
        }
    }

    closedir(dir);
    return 0;
}

void cpy_ss(SS_Request recvd_request, int sock_fd)
{
    if (recvd_request.type == REQUEST_COPY)
    {
        printf(YELLOW("Copy request for file or directory received from Subsystem.\n"));
        char *path = recvd_request.info;

        // Check if the path is a file or directory
        if (path[strlen(path) - 1] != '/')
        {
            printf(YELLOW("Path is a file: %s\n"), path);

            // Read file content
            FILE *fptr = fopen(path, "r");
            if (fptr == NULL)
            {
                fprintf(stderr, RED("fopen: could not open file for reading: %s\n"), strerror(errno));
                SS_Request error_packet = {.error_code = FILE_NOT_FOUND, .type = ACK};
                send(sock_fd, &error_packet, sizeof(error_packet), 0);
                return;
            }

            char buffer[MAX_INFO_SIZE - MAX_PATH_LENGTH - 2] = {0};
            fread(buffer, sizeof(char), sizeof(buffer), fptr);
            fclose(fptr);

            // Create message to send <file_path>|<file_content>
            SS_Request file_data_request;
            file_data_request.type = FILE_DATA_TO_BE_COPIED_SS;
            snprintf(file_data_request.info, MAX_INFO_SIZE, "%s|%s", path, buffer);

            if (send(sock_fd, &file_data_request, sizeof(SS_Request), 0) <= 0)
            {
                fprintf(stderr, RED("send: failed to send file data: %s\n"), strerror(errno));
                SS_Request error_packet = {.error_code = SEND_FAILED, .type = ACK};
                send(sock_fd, &error_packet, sizeof(error_packet), 0);
                return;
            }
        }
        else
        {
            printf(YELLOW("Path is a directory: %s\n"), path);

            char **files_folders = get_all_files_folders(path); // Get all files and subfolders recursively
            int n_entries = 0;

            while (files_folders[n_entries] != NULL)
                n_entries++;

            for (int i = 0; i < n_entries; i++)
            {
                char *curr_path = files_folders[i];
                if ((curr_path[strlen(curr_path) - 1] != '/'))
                {
                    // Send file data
                    FILE *fptr = fopen(curr_path, "r");
                    if (fptr == NULL)
                    {
                        fprintf(stderr, RED("fopen: failed to open file %s: %s\n"), curr_path, strerror(errno));
                        continue;
                    }

                    char buffer[MAX_INFO_SIZE - MAX_PATH_LENGTH - 2] = {0};
                    fread(buffer, sizeof(char), sizeof(buffer), fptr);
                    fclose(fptr);

                    SS_Request file_data_request;
                    file_data_request.type = DIRECTORY_TO_BE_COPIED_SS;
                    snprintf(file_data_request.info, MAX_INFO_SIZE, "%s|%s", curr_path, buffer);

                    if (send(sock_fd, &file_data_request, sizeof(SS_Request), 0) <= 0)
                    {
                        fprintf(stderr, RED("send: failed to send file data: %s\n"), strerror(errno));
                        SS_Request error_packet = {.error_code = SEND_FAILED, .type = ACK};
                        send(sock_fd, &error_packet, sizeof(error_packet), 0);
                        continue;
                    }
                }
                else
                {
                    // Send directory creation request
                    SS_Request dir_data_request;
                    dir_data_request.type = DIRECTORY_TO_BE_COPIED_SS;
                    snprintf(dir_data_request.info, MAX_INFO_SIZE, "%s", curr_path);

                    if (send(sock_fd, &dir_data_request, sizeof(SS_Request), 0) <= 0)
                    {
                        fprintf(stderr, RED("send: failed to send directory data: %s\n"), strerror(errno));
                        SS_Request error_packet = {.error_code = SEND_FAILED, .type = ACK};
                        send(sock_fd, &error_packet, sizeof(error_packet), 0);
                        continue;
                    }
                }
            }

            // Free allocated memory
            for (int i = 0; i < n_entries; i++)
                free(files_folders[i]);
            free(files_folders);
        }

        SS_Request error_packet = {.error_code = OK, .type = ACK};
        send(sock_fd, &error_packet, sizeof(error_packet), 0);
    }
}

void pnt_ss(SS_Request recvd_request, int sock_fd)
{
    if (recvd_request.type == REQUEST_PASTE)
    {
        printf(YELLOW("Paste request for file or directory received from Subsystem.\n"));

        if (recvd_request.type == FILE_DATA_TO_BE_COPIED_SS)
        {
            // Extract file path and content
            char *file_path = strtok(recvd_request.info, "|");
            char *file_content = strtok(NULL, "|");

            if (!file_path || !file_content)
            {
                fprintf(stderr, RED("Invalid file data format.\n"));
                SS_Request error_packet = {.error_code = PASTE_FAILED, .type = ACK};
                send(sock_fd, &error_packet, sizeof(error_packet), 0);
            }

            // Ensure directories exist
            if (create_dirs(file_path) != 0)
            {
                fprintf(stderr, RED("Error creating directories for path: %s\n"), file_path);
                SS_Request error_packet = {.error_code = PASTE_FAILED, .type = ACK};
                send(sock_fd, &error_packet, sizeof(error_packet), 0);
            }

            // Write file content
            FILE *fptr = fopen(file_path, "w");
            if (fptr == NULL)
            {
                fprintf(stderr, RED("fopen: could not open file for writing: %s\n"), strerror(errno));
                SS_Request error_packet = {.error_code = PASTE_FAILED, .type = ACK};
                send(sock_fd, &error_packet, sizeof(error_packet), 0);
            }

            fprintf(fptr, "%s", file_content);
            fclose(fptr);
        }
        else if (recvd_request.type == DIRECTORY_TO_BE_COPIED_SS)
        {
            // Extract directory path
            char *dir_path = recvd_request.info;

            if (!dir_path)
            {
                fprintf(stderr, RED("Invalid directory data format.\n"));
                SS_Request error_packet = {.error_code = PASTE_FAILED, .type = ACK};
                send(sock_fd, &error_packet, sizeof(error_packet), 0);
            }

            // Create directory
            if (mkdir(dir_path, 0777) != 0 && errno != EEXIST)
            {
                fprintf(stderr, RED("mkdir: failed to create directory %s: %s\n"), dir_path, strerror(errno));
                SS_Request error_packet = {.error_code = PASTE_FAILED, .type = ACK};
                send(sock_fd, &error_packet, sizeof(error_packet), 0);
            }
        }

        SS_Request error_packet = {.error_code = OK, .type = ACK};
        send(sock_fd, &error_packet, sizeof(error_packet), 0);
    }
}