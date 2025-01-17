#include "header.h"

int sock_fd;

void send_to_server(ErrorCode error_code, RequestType requestType, char *info, int thread_id)
{

    // Prepare the request
    SS_Request request;
    memset(&request, 0, sizeof(request));
    request.SS_id = server_id;
    request.type = requestType;
    request.error_code = error_code;
    request.Thread_id = thread_id;
    strcpy(request.info, info);

    // Send the request to the Naming Server
    if (send(sock_fd, &request, sizeof(request), 0) < 0)
    {
        perror("Failed to send request");
        close(sock_fd);
        return;
    }

    printf("SENT\n\n\n");
    return;
}

// Register the Storage Server with the Naming Server dynamically
// ip|nm_port|client_port
void register_with_naming_server()
{
    // Connect to the Naming Server
    char info[MAX_INFO_SIZE];

    struct sockaddr_in nm_addr;
    memset(&nm_addr, 0, sizeof(nm_addr));
    char buffer[BUFFER_SIZE];

    // Create a socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("Socket creation failed");
        return;
    }

    // Configure Naming Server address
    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(nm_port);
    if (inet_pton(AF_INET, nm_ip, &nm_addr.sin_addr) <= 0)
    {
        perror("Invalid Naming Server IP address");
        close(sock_fd);
        return;
    }

    printf(GREEN("Started listening to NFS threads.\n"));

    // Connect to the Naming Server
    printf("Attempting to connect to %s:%d...\n", nm_ip, nm_port);
    if (connect(sock_fd, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) < 0)
    {
        perror("Connection to Naming Server failed");
        close(sock_fd);
        return;
    }

    // Prepare the registration details to send to the Naming Server
    printf("%s\n", ss_ip);
    snprintf(info, MAX_INFO_SIZE, "%s|%d|%d|%d", ss_ip, ss_client_port, ss_client_port + 1, ss_client_port + 2);
    send_to_server(OK, REQUEST_REGESTER, info, -1);
    printf("Waiting for registration response...\n");

    while (server_id == -1){
        usleep(100000);
    }

    printf("Successfully connected to NM\n");
    ListNode *current = accessiblePathsHead;
    memset(info, 0, sizeof(info));
    while (current != NULL)
    {
        char path[MAX_PATH_LENGTH];
        snprintf(path, sizeof(path), ".%s", (char *)current->data);
        if (strlen(info) + strlen(path) < MAX_INFO_SIZE - 10)
        {
            strcat(info, path);
            strcat(info, "|");
        }
        else
        {
            send_to_server(OK, REQUEST_ADD_PATH, info, -1);
            memset(info, 0, sizeof(info));
            snprintf(info + strlen(info), MAX_INFO_SIZE - strlen(info), ".%s|", path);
        }

        // send_to_server(OK, REQUEST_ADD_PATH, path, );

        usleep(10000);
        current = current->next;
    }

    send_to_server(OK, REQUEST_ADD_PATH, info, -1);
    memset(info, 0, sizeof(info));
    // Close the socket
    printf("Storage Server registered with Naming Server.\n");
}

// Function to handle incoming NM requests
void *nm_request_listener(void *arg)
{
    int nport = *(int *)arg;

    while (1)
    {
        struct sockaddr_in nfs_addr;
        socklen_t addr_size = sizeof(nfs_addr);
        int nfs_sock_fd = accept(nport, (struct sockaddr *)&nfs_addr, &addr_size);

        if (nfs_sock_fd == -1)
        {
            perror("Failed to accept connection");
            continue;
        }

        pthread_t nm_thread;
        if (pthread_create(&nm_thread, NULL, (void *)nm_thread_handler, &(nfs_sock_fd)) != 0)
        {
            perror("Failed to create NM thread");
        }
        else
        {
            pthread_detach(nm_thread);
        }
    }

    pthread_exit(NULL);
}

void *nm_thread_handler(void *arg)
{
    int reply = *(int *)arg; // Free allocated memory for the client socket
    printf("Naming Server connected\n");
    handle_nm_command(reply);
    pthread_exit(NULL);
}

// Function to handle NM commands like creating, deleting, or copying files
void handle_nm_command(int ss_client_port)
{
    SS_Request nm_request;
    memset(&nm_request, 0, sizeof(nm_request));

    // Receive data from NM
    int bytes_received = recv(ss_client_port, &nm_request, sizeof(nm_request), MSG_WAITALL);
    if (bytes_received <= 0)
    {
        perror("Error receiving data from NM");
        return;
    }

    // Print the received request
    printf("Received request from NM: %s\n", nm_request.info);

    if (nm_request.error_code != OK)
    {
        printf("Error code: %d\n", nm_request.error_code);
        return;
    }

    if (nm_request.type == REQUEST_REGESTER)
    {
        server_id = nm_request.SS_id;
    }
    else if (nm_request.type == PING)
    {
        send_to_server(OK, PING, "Ping", -1);
    }
    else if (nm_request.type == REQUEST_CREATE)
    {
        // Parse and create file or directory
        ss_handle_create_request(nm_sock, &nm_request);
    }
    else if (nm_request.type == REQUEST_DELETE)
    {
        // Parse and delete file or directory
        ss_handle_delete_request(nm_sock, &nm_request);
    }
    else if (nm_request.type == REQUEST_COPY || nm_request.type == REQUEST_BACKUP_COPY)
    {
        // Copy file from specified location
        ss_paste_file_or_directory(nm_sock, &nm_request);
        // pnt_ss(nm_request, nm_sock);
    } 
    else if (nm_request.type == COPY_ADD_PATH)
    {
        create_dirs_and_files_from_list(nm_request.info);
        // add_accessible_path(nm_request.info);
    } else if(nm_request.type == REQUEST_FILE_CONTENT){
        // add_accessible_path(nm_request.info);
        ss_cpy_file(nm_sock, nm_request.info);
    } else if(nm_request.type == COPY_ADD_DATA){
        char *path = strtok(nm_request.info, "|");
        char *data = strtok(NULL, "|");
        char *full_path = (char *)malloc(sizeof(char) * (strlen(root_directory) + strlen(path) + 2));
        snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, path + 2);
        dataToWrite(full_path, data);
        // add_accessible_path(nm_request.info);
    } 
    else
    {
        printf("%d\n", nm_request.type);
        printf("Unknown operation requested by NM.\n");
    }
}

void ss_handle_create_request(int client_sock, SS_Request *request)
{
    size_t path_len = strlen(request->info);

    char path[MAX_INFO_SIZE], name[MAX_INFO_SIZE];
    strcpy(path, strtok(request->info, "|"));
    strcpy(name, strtok(NULL, "|"));

    char cwd[MAX_PATH_LENGTH + 1];
    getcwd(cwd, sizeof(cwd));

    // Check if the path ends with '/'
    if (request->info[path_len - 1] == '/')
    {
        // Path ends with '/', so create a directory
        ss_create_directory(client_sock, path, name, request->Thread_id);
    }
    else
    {
        // Path does not end with '/', so create a file
        ss_create_file(client_sock, path, name, request->Thread_id);
    }

    chdir(cwd);
    return;
}

void ss_create_file(int client_sock, char *path, char *name, int thread_id)
{
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, path + 2);

    if (chdir(full_path) < 0)
    {
        ErrorCode error_code;
        switch (errno)
        {
        case ENOENT:
            error_code = DIR_DOESNT_EXIST;
            break;
        case EACCES:
            error_code = PERMISSION_DENIED;
            break;
        default:
            error_code = UNABLE_TO_CHANGE_DIR;
        }

        send_to_server(error_code, ACK, path, thread_id);
        return;
    }

    char add_path[MAX_PATH_LENGTH];
    snprintf(add_path, sizeof(add_path), "%s%s", path, name);

    int file = open(name, O_CREAT | O_EXCL, 0644);
    if (file < 0)
    {
        send_to_server(FILE_CREATION_FAILED, ACK, name, thread_id);
    }
    else
    {
        send_to_server(OK, ACK, name, thread_id);
        send_to_server(OK, REQUEST_ADD_PATH, add_path, -1);
        close(file);
    }
}

void ss_create_directory(int client_sock, char *path, char *name, int thread_id)
{
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, path + 2);

    if (chdir(full_path) < 0)
    {
        ErrorCode error_code;
        switch (errno)
        {
        case ENOENT:
            error_code = DIR_DOESNT_EXIST;
            break;
        case EACCES:
            error_code = PERMISSION_DENIED;
            break;
        default:
            error_code = UNABLE_TO_CHANGE_DIR;
        }

        send_to_server(error_code, ACK, path, thread_id);
    }

    char add_path[MAX_PATH_LENGTH];
    snprintf(add_path, sizeof(add_path), "%s%s", path, name);

    if (mkdir(name, 0755) < 0)
    {
        send_to_server(DIR_CREATION_FAILED, ACK, name, thread_id);
    }
    else
    {
        send_to_server(OK, ACK, name, thread_id);
        send_to_server(OK, REQUEST_ADD_PATH, add_path, -1);
    }
}

void ss_handle_delete_request(int client_sock, SS_Request *request)
{
    struct stat path_stat;

    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, request->info + 2);

    // Get path information
    if (stat(full_path, &path_stat) != 0)
    {
        perror("Error getting path stat");
        send_to_server(STAT_FAILED, DELETE_COMPLETED, request->info, request->Thread_id);
        return;
    }

    // Check if path is a directory
    if (S_ISDIR(path_stat.st_mode))
    {
        ss_delete_recursive_dir(client_sock, request->info, request->Thread_id); // Recursive delete for directories
    }
    else if (S_ISREG(path_stat.st_mode))
    {
        // If it's a file, just unlink it
        ss_delete_file(client_sock, request->info, request->Thread_id);
        send_to_server(OK, DELETE_COMPLETED, request->info, request->Thread_id);
    }
    else
    {
        send_to_server(NOT_FILE_OR_DRECTORY, DELETE_COMPLETED, request->info, request->Thread_id);
    }

    return;
}

void ss_delete_file(int client_sock, char *path, int thread_id)
{
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, path + 2);

    if (unlink(full_path) != 0)
    {
        perror("Error deleting file");
        send_to_server(FILE_DELETION_FAILED, DELETE_COMPLETED, path, thread_id);
    }
    else
    {
        send_to_server(OK, DELETE_COMPLETED, path, thread_id);
        send_to_server(OK, REQUEST_DELETE_PATH, path, -1);
    }
}

// void ss_delete_recursive_dir(int client_sock, char *path, int thread_id)
// {
//     char full_path[MAX_PATH_LENGTH];
//     snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, path + 2);

//     DIR *dir = opendir(full_path);
//     if (!dir)
//     {
//         perror("Failed to open directory");
//         send_to_server(DIR_DOESNT_EXIST, PARTIAL_DELETE_COMPLETED, path, thread_id);
//         return;
//     }

//     struct dirent *entry;
//     struct stat entry_stat;

//     while ((entry = readdir(dir)) != NULL)
//     {
//         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
//         {
//             continue; // Skip . and .. entries
//         }

//         // Construct the full path
//         char full_path2[MAX_PATH_LENGTH];
//         snprintf(full_path2, sizeof(full_path2), "%s/%s", full_path, entry->d_name);

//         // Get entry stat
//         if (stat(full_path, &entry_stat) != 0)
//         {
//             perror("Error getting entry stat");
//             send_to_server(STAT_FAILED, PARTIAL_DELETE_COMPLETED, path, thread_id);
//             closedir(dir);
//             return;
//         }

//         // Recurse if directory, unlink if file
//         if (S_ISDIR(entry_stat.st_mode))
//         {
//             memset(full_path2, 0, sizeof(full_path2));
//             snprintf(full_path2, sizeof(full_path2), "%s/%s", path, entry->d_name);
//             ss_delete_recursive_dir(client_sock, full_path2, thread_id);
//         }
//         else if (S_ISREG(entry_stat.st_mode))
//         {
//             if (unlink(full_path2) != 0)
//             {
//                 perror("Error deleting file");
//                 send_to_server(FILE_DELETION_FAILED, PARTIAL_DELETE_COMPLETED, path, thread_id);
//             }
//             else
//             {
//                 send_to_server(OK, REQUEST_DELETE_PATH, path, -1);
//             }
//         }
//     }

//     closedir(dir);

//     // Remove the directory itself
//     if (rmdir(path) != 0)
//     {
//         perror("Error deleting directory");
//         send_to_server(DIR_DELETION_FAILED, PARTIAL_DELETE_COMPLETED, path, thread_id);
//         return;
//     }
//     else
//     {
//         send_to_server(OK, REQUEST_DELETE_PATH, path, thread_id);
//     }
// }

void ss_delete_recursive_dir(int client_sock, char *path, int thread_id)
{
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, path + 2);

    // Check if the directory exists
    struct stat statbuf;
    if (stat(full_path, &statbuf) != 0)
    {
        perror("Failed to stat path");
        send_to_server(DIR_DOESNT_EXIST, PARTIAL_DELETE_COMPLETED, path, thread_id);
        return;
    }

    // Prepare arguments for execvp
    char *argv[] = {"rm", "-rf", full_path, NULL};

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Failed to fork");
        send_to_server(FORK_FAILED, PARTIAL_DELETE_COMPLETED, path, thread_id);
        return;
    }
    else if (pid == 0)
    {
        // In child process
        execvp("rm", argv);
        // If execvp fails
        perror("Failed to execute rm");
        return;
    }
    else
    {
        // In parent process
        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            perror("Failed to wait for child process");
            send_to_server(WAIT_FAILED, PARTIAL_DELETE_COMPLETED, path, thread_id);
            return;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            printf("%s\n", path);
            send_to_server(OK, DELETE_COMPLETED, path, thread_id);
            send_to_server(OK, REQUEST_DELETE_PATH, path, -1);
        }
        else
        {
            fprintf(stderr, "rm -rf failed with exit code %d\n", WEXITSTATUS(status));
            send_to_server(RM_FAILED, PARTIAL_DELETE_COMPLETED, path, thread_id);
        }
    }
}

void ss_cpy_file(int client_sock, char *file_path) {
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, file_path + 2);

    int file = open(full_path, O_RDONLY);
    if (file < 0)
    {
        // If the file can't be opened, send an error request
        Request error_request = {.error_code = FILE_NOT_FOUND};
        send(client_sock, &error_request, sizeof(error_request), 0);
        send_to_server(FILE_NOT_FOUND, REQUEST_READ_COMPLETED, file_path, -1);
        return;
    }

    char buffer[MAX_INFO_SIZE];
    ssize_t bytes_read;

    // Read and send the file in requests
    while ((bytes_read = read(file, buffer, MAX_INFO_SIZE)) > 0)
    {
        Request request;
        memset(&request, 0, sizeof(Request));
        request.type = REQ;
        request.error_code = OK;
        memcpy(request.info, buffer, bytes_read);

        printf("Sending data to client: %.*s\n", MAX_INFO_SIZE, buffer);

        int status = send(client_sock, &request, sizeof(request), 0);

        if (status < 0)
        {
            perror("Failed to send request");
            close(file);
            return;
        }

        memset(buffer, 0, MAX_INFO_SIZE); // Clear data buffer for next read
    }

    close(file);
    printf("File sent successfully.\n");

    Request request;
    memset(&request, 0, sizeof(Request));
    request.type = ACK;
    request.error_code = OK;
    send(client_sock, &request, sizeof(request), 0);
    close(client_sock);
}

// Function to ensure a directory and its parent directories exist
void create_dir(const char *dir_path) {
    char temp_path[MAX_PATH_LENGTH];
    char *p = NULL;

    snprintf(temp_path, sizeof(temp_path), "%s", dir_path);
    for (p = temp_path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0'; // Temporarily terminate the string to create intermediate directory
            if (mkdir(temp_path, 0777) == -1 && errno != EEXIST) {
                perror("mkdir");
                return;
            }
            *p = '/'; // Restore the slash
        }
    }

    if (mkdir(temp_path, 0777) == -1 && errno != EEXIST) {
        perror("mkdir");
        return;
    }
}

// Function to create directories and files from a list of paths
void create_dirs_and_files_from_list(const char *paths_list) {
    char *paths_copy = strdup(paths_list);
    if (!paths_copy) {
        perror("strdup");
        return;
    }

    char *token = strtok(paths_copy, "|");
    char *rd = malloc(MAX_PATH_LENGTH);
    snprintf(rd, MAX_PATH_LENGTH, "%s/%s", root_directory, token + 2);
    token = strtok(NULL, "|"); // Skip the root directory
    while (token) {
        char *content_delim = strchr(token, '|');
        char *path = token;

        char *full_path = (char *)malloc(MAX_PATH_LENGTH);
        snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", rd, path + 2);

        if (content_delim) {
            // File with content
            *content_delim = '\0'; // Separate path and content
            const char *content = content_delim + 1;

            // Ensure parent directories exist
            char *last_slash = strrchr(full_path, '/');
            if (last_slash) {
                *last_slash = '\0'; // Temporarily terminate full_path to extract parent directory
                create_dir(full_path);
                *last_slash = '/'; // Restore the slash
            }

            // Create the file and write content
            FILE *file = fopen(full_path, "w");
            if (!file) {
                fprintf(stderr, "fopen: failed to create file %s: %s\n", full_path, strerror(errno));
                free(paths_copy);
                return;
            }
            fprintf(file, "%s", content);
            fclose(file);
        } else {
            // Directory
            create_dir(full_path);
        }

        token = strtok(NULL, "|");
    }

    free(paths_copy);
}

// Function to construct the full path and append data
void dataToWrite(char *full_path, char *data) {
    FILE *file = fopen(full_path, "a");
    if (!file) {
        perror("fopen");
        return;
    }

    fprintf(file, "%s", data);
    fclose(file);
}