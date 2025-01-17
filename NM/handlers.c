#include "header.h"
#include <stdio.h>
#include <string.h>

// Assuming cache and trie are globally accessible

Request handle_read(char *path)
{
    if(path == NULL || path[strlen(path)-1] == '/'){
        fprintf(stderr, RED_BOLD("Invalid path: %s\n"), path);
        return (Request){.type = RESPONSE,.error_code = INVALID_PATH };
    }
    Request req;
    req.type = RESPONSE;
    req.error_code = OK;
    cache_error_t err;
    int ss_id;
    err = get_from_cache(g_cache, path, &ss_id);
    if (err != CACHE_SUCCESS)
    {
        printf("YES\n");
        pthread_mutex_lock(&g_trie_lock);
        ss_id = search_path(g_directory_trie, path);
        pthread_mutex_unlock(&g_trie_lock);
    }

    if (ss_id != -1)
    {   
        printf("Server Id: %d\n", ss_id);
        err = put_in_cache(g_cache, path, ss_id);
        printf("Server Id: %d\n", ss_id);
        if (err != CACHE_SUCCESS)
        {
            fprintf(stderr, RED("Failed to add the entry to cache :%d\n"), err);
        }
        printf("Server Id: %d\n", ss_id);
        if(!check_file_lock(path, false).one){
            req.error_code = PERMISSION_DENIED;
            snprintf(req.info, MAX_INFO_SIZE, "Someone else is using the file!");
            printf(RED_BOLD("Someone else is using the file!\n"));
            return req;
        }
        char server_ip[MAX_IP_SIZE];
        int server_port;
        ss_t *ss = &g_storage_servers[ss_id];
        printf("Server Id: %d\n", ss_id);
        // pthread_mutex_lock(&ss->lock);
        server_port = ss->client_port;
        strcpy(server_ip, ss->ip);
        char *data = (char *)malloc(sizeof(MAX_INFO_SIZE));
        snprintf(req.info, MAX_INFO_SIZE, "%s|%d", server_ip, server_port);

        printf("Server Id: %d\n", ss_id);
        printf("%s %d\n", server_ip, server_port);
        printf("CYKA\n");
        if (!ss->is_alive)
        {
            printf("BLYADD\n");
            // pthread_mutex_unlock(&ss->lock);

            // check for backup and see what to do next
            int bc1 = ss->backup1;
            int bc2 = ss->backup2;
            ss_id = bc1;
            printf("ssid from backup1: %d\n", ss_id);

            ss = &g_storage_servers[ss_id];
            server_port = ss->client_port;
            printf("ssid from backup1: %d\n", ss_id);
            strcpy(server_ip, ss->ip);
            req.error_code = OK;
            snprintf(req.info, MAX_INFO_SIZE, "%s|%d", server_ip, server_port);
            if (!ss->is_alive)
            {
                ss_id = bc2;
                            printf("ssid from backup2: %d\n", ss_id);
                ss = &g_storage_servers[ss_id];
                server_port = ss->client_port;
                strcpy(server_ip, ss->ip);
                                            printf("ssid from backup2: %d\n", ss_id);

                req.error_code = OK;
                snprintf(req.info, MAX_INFO_SIZE, "%s|%d", server_ip, server_port);
                if (!ss->is_alive)
                {
                    // if nothing works in backup and still that path can not be accessed......>
                    req.error_code = FILE_NOT_FOUND;
                    snprintf(req.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
                    printf(RED_BOLD("File Not Found reading!\n"));
                    return req;
                }
            }
        }
        printf("NAHUIIIIII\n");
        return req;
    }
    else
    {
        req.error_code = FILE_NOT_FOUND;
        snprintf(req.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
        printf(RED_BOLD("File Not Found!\n"));
        printf("File Not Found!\n");
        return req;
    }
}

Request handle_write(char *path)
{
    if(!path || path[strlen(path)-1] == '/'){
        fprintf(stderr, RED_BOLD("Invalid path: %s\n"), path);
        return (Request){.type = RESPONSE,.error_code = INVALID_PATH };
    }
    Request req;
    req.type = RESPONSE;
    req.error_code = OK;
    cache_error_t err;
    int ss_id;
    err = get_from_cache(g_cache, path, &ss_id);
    if (err != CACHE_SUCCESS)
    {
        pthread_mutex_lock(&g_trie_lock);
        ss_id = search_path(g_directory_trie, path);
        pthread_mutex_unlock(&g_trie_lock);
    }

    if (ss_id != -1)
    {
        err = put_in_cache(g_cache, path, ss_id);
        if (err != CACHE_SUCCESS)
        {
            fprintf(stderr, RED("Failed to add the entry to cache :%d\n"), err);
        }
        if(!check_file_lock(path, true).one){
            req.error_code = PERMISSION_DENIED;
            snprintf(req.info, MAX_INFO_SIZE, "Someone else is using the file!");
            return req;
        }
        char server_ip[MAX_IP_SIZE];
        int server_port;
        ss_t *ss = &g_storage_servers[ss_id];
        // pthread_mutex_lock(&ss->lock);
        server_port = ss->client_port;
        strcpy(server_ip, ss->ip);
        char *data = (char *)malloc(sizeof(MAX_INFO_SIZE));
        snprintf(req.info, MAX_INFO_SIZE, "%s|%d", server_ip, server_port);

        if (!ss->is_alive)
        {
            // pthread_mutex_unlock(&ss->lock);

            // check for backup and see what to do next

            // if nothing works
            req.error_code = FILE_NOT_FOUND;
            snprintf(req.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
            printf(RED_BOLD("File Not Found!\n"));
            return req;
        }
        return req;
    }
    else
    {
        req.error_code = FILE_NOT_FOUND;
        snprintf(req.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
        printf(RED_BOLD("File Not Found!\n"));
        return req;
    }
}

Request handle_get_info(char *path)
{
    Request req;
    req.type = RESPONSE;
    req.error_code = OK;
    cache_error_t err;
    int ss_id;
    err = get_from_cache(g_cache, path, &ss_id);
    if (err != CACHE_SUCCESS)
    {
        pthread_mutex_lock(&g_trie_lock);
        ss_id = search_path(g_directory_trie, path);
        pthread_mutex_unlock(&g_trie_lock);
    }

    if (ss_id != -1)
    {
        err = put_in_cache(g_cache, path, ss_id);
        if (err != CACHE_SUCCESS)
        {
            fprintf(stderr, RED("Failed to add the entry to cache :%d\n"), err);
        }
        if(!check_file_lock(path, false).one){
            req.error_code = PERMISSION_DENIED;
            snprintf(req.info, MAX_INFO_SIZE, "Someone else is using the file!");
            return req;
        }
        char server_ip[MAX_IP_SIZE];
        int server_port;
        ss_t *ss = &g_storage_servers[ss_id];
        // pthread_mutex_lock(&ss->lock);
        server_port = ss->client_port;
        strcpy(server_ip, ss->ip);
        char *data = (char *)malloc(sizeof(MAX_INFO_SIZE));
        snprintf(req.info, MAX_INFO_SIZE, "%s|%d", server_ip, server_port);

        if (!ss->is_alive)
        {
            // pthread_mutex_unlock(&ss->lock);

            // check for backup and see what to do next

            // if nothing works
            req.error_code = FILE_NOT_FOUND;
            snprintf(req.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
            printf(RED_BOLD("File Not Found!\n"));
            return req;
        }
        return req;
    }
    else
    {
        req.error_code = FILE_NOT_FOUND;
        snprintf(req.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
        printf(RED_BOLD("File Not Found!\n"));
        return req;
    }
}

Request handle_stream_audio(char *path)
{
    if(path == NULL || strstr(path, ".txt") != NULL){
        printf(RED_BOLD("Error: Text files are not streamable.\n"));
        Request req;
        req.error_code = NOT_STREAMABLE;
        snprintf(req.info, MAX_INFO_SIZE, "The file is not streamable.");
        printf(RED_BOLD("The file is not streamable.\n"));
        return req;
    }
    Request req;
    req.type = RESPONSE;
    req.error_code = OK;
    cache_error_t err;
    int ss_id;
    err = get_from_cache(g_cache, path, &ss_id);
    if (err != CACHE_SUCCESS)
    {
        pthread_mutex_lock(&g_trie_lock);
        ss_id = search_path(g_directory_trie, path);
        pthread_mutex_unlock(&g_trie_lock);
    }

    if (ss_id != -1)
    {
        err = put_in_cache(g_cache, path, ss_id);
        if (err != CACHE_SUCCESS)
        {
            fprintf(stderr, RED("Failed to add the entry to cache :%d\n"), err);
        }
        if(!check_file_lock(path, false).one){
            req.error_code = PERMISSION_DENIED;
            snprintf(req.info, MAX_INFO_SIZE, "Someone else is using the file!");
            return req;
        }
        char server_ip[MAX_IP_SIZE];
        int server_port;
        ss_t *ss = &g_storage_servers[ss_id];
        // pthread_mutex_lock(&ss->lock);
        server_port = ss->client_port;
        strcpy(server_ip, ss->ip);
        char *data = (char *)malloc(sizeof(MAX_INFO_SIZE));
        snprintf(req.info, MAX_INFO_SIZE, "%s|%d", server_ip, server_port);

        if (!ss->is_alive)
        {
            // pthread_mutex_unlock(&ss->lock);

            // check for backup and see what to do next

            // if nothing works
            req.error_code = FILE_NOT_FOUND;
            snprintf(req.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
            printf(RED_BOLD("File Not Found!\n"));
            return req;
        }
        return req;
    }
    else
    {
        req.error_code = FILE_NOT_FOUND;
        snprintf(req.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
        printf(RED_BOLD("File Not Found!\n"));
        return req;
    }
}

// It will return all the paths from all storage servers
// whether or not storage server is online or not
// If ss is not online or no backup it would be
// sepearately handeled in the other requests
Request handle_list_paths(int client_sock)
{
    printf("YES\n");
    ListNode *head = list_all_paths(g_directory_trie);
    Request req;
    ListNode *current = head;
    char *paths = (char *)malloc(MAX_INFO_SIZE); // Allocate MAX_INFO_SIZE bytes
    if (!paths) {
        perror("malloc failed for paths");
        exit(EXIT_FAILURE);
    }
    strcpy(paths, ""); // Initialize paths as an empty string

    printf("NO\n");
    while (current) {
        char *path = (char *)malloc(MAX_PATH_LENGTH); // Allocate space for path
        if (!path) {
            perror("malloc failed for path");
            free(paths); // Clean up previously allocated memory
            exit(EXIT_FAILURE);
        }
        
        strcpy(path, (char *)current->data); // Copy data to path
        // Check if there is enough space in paths for the new path and separator
        if (strlen(paths) + strlen(path) + 1 >= MAX_INFO_SIZE) {
            // fprintf(stderr, "Not enough space in paths to append more data\n");
            // free(path);
            // free(paths);
            // exit(EXIT_FAILURE);

            usleep(10000);
            req.type = RESPONSE;
            req.error_code = OK;
            snprintf(req.info, MAX_INFO_SIZE, "%s", paths);
            printf("SENDING\n\n\n\n\n\n\n\n\n\n");
            send_response(client_sock, RESPONSE, OK, req.info, -1);
            strcpy(paths, "");
        }

        strcat(paths, path);  // Append the path
        strcat(paths, "|");   // Append the separator
        current = current->next;
        free(path);  // Free the individual path after appending
    }

    usleep(10000);
    // Now use snprintf to copy paths into req.info
    req.type = ACK;
    req.error_code = OK;
    snprintf(req.info, MAX_INFO_SIZE, "%s", paths);

    printf("YES\n");
    // Clean up paths (no need to free the list in this snippet, assuming it's done elsewhere)
    free(paths); 

    return req;
}

Request handle_create(char *path, char *name, int thread_id)
{
    Request res;
    cache_error_t err;
    int ss_id;
    err = get_from_cache(g_cache, path, &ss_id);
    if (err != CACHE_SUCCESS)
    {
        pthread_mutex_lock(&g_trie_lock);
        ss_id = search_path(g_directory_trie, path);
        pthread_mutex_unlock(&g_trie_lock);
    }

    if (ss_id != -1)
    {
        err = put_in_cache(g_cache, path, ss_id);
        if (err != CACHE_SUCCESS)
        {
            fprintf(stderr, RED("Failed to add the entry to cache :%d\n"), err);
        }
        ss_t *ss = &g_storage_servers[ss_id];
        // pthread_mutex_lock(&ss->lock);

        if (!ss->is_alive)
        {
            // pthread_mutex_unlock(&ss->lock);
            // BACKUP

            // if nothing works in backup and still that path can not be accessed......>
            res.type = ACK;
            res.error_code = FILE_NOT_FOUND;
            snprintf(res.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
            printf(RED_BOLD("File Not Found!\n"));
            return res;
        }
        else
        {
            // pthread_mutex_unlock(&ss->lock);
            SS_Request s;
            s.error_code = OK;
            s.SS_id = ss_id;
            s.type = REQUEST_CREATE;
            s.Thread_id = thread_id;
            strcpy(s.info, "");
            strcat(s.info, path);
            strcat(s.info, "|");
            strcat(s.info, name);
            res = send_request_to_ss(ss_id, s);
        }
        return res;
    }
    else
    {
        res.error_code = FILE_NOT_FOUND;
        snprintf(res.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
        printf(RED_BOLD("File Not Found!\n"));
        return res;
    }
    return res;
}

Request handle_delete(char *path){
    Request res;
    res.type = RESPONSE;
    res.error_code = OK;
    cache_error_t err;
    int ss_id;
    err = get_from_cache(g_cache, path, &ss_id);
    if (err != CACHE_SUCCESS)
    {
        pthread_mutex_lock(&g_trie_lock);
        ss_id = search_path(g_directory_trie, path);
        pthread_mutex_unlock(&g_trie_lock);
    }

    if (ss_id != -1)
    {
        err = put_in_cache(g_cache, path, ss_id);
        if (err != CACHE_SUCCESS)
        {
            fprintf(stderr, RED("Failed to add the entry to cache :%d\n"), err);
        }
        ss_t *ss = &g_storage_servers[ss_id];
        // pthread_mutex_lock(&ss->lock);

        if (!ss->is_alive)
        {
            // pthread_mutex_unlock(&ss->lock);
            // BACKUP

            // if nothing works in backup and still that path can not be accessed......>
            res.type = ACK;
            res.error_code = FILE_NOT_FOUND;
            snprintf(res.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
            printf(RED_BOLD("File Not Found!\n"));
            return res;
        }
        else
        {
            // pthread_mutex_unlock(&ss->lock);
            SS_Request s;
            s.error_code = OK;
            s.SS_id = ss_id;
            s.type = REQUEST_DELETE;
            strcpy(s.info, path);
            res = send_request_to_ss(ss_id, s);
        }
        return res;
    }
    else
    {
        res.error_code = FILE_NOT_FOUND;
        snprintf(res.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
        printf(RED_BOLD("File Not Found!\n"));
        return res;
    }
    return res;
}

// source|dest|ip|port
Request handle_copy(char *info){
    char *source = strtok(info, "|");
    char *dest = strtok(NULL, "|");

    int source_id = -1;
    int dest_id = -1;

    cache_error_t err;
    err = get_from_cache(g_cache, source, &source_id);
    if (err != CACHE_SUCCESS)
    {
        pthread_mutex_lock(&g_trie_lock);
        source_id = search_path(g_directory_trie, source);
        pthread_mutex_unlock(&g_trie_lock);
    }
    err = get_from_cache(g_cache, dest, &dest_id);
    if (err != CACHE_SUCCESS)
    {
        pthread_mutex_lock(&g_trie_lock);
        dest_id = search_path(g_directory_trie, dest);
        pthread_mutex_unlock(&g_trie_lock);
    }

        printf("1 COPY %s -> %s\n", source, dest);


    ss_t *ss_source = &g_storage_servers[source_id];
    ss_t *ss_dest = &g_storage_servers[dest_id];

    

    // pthread_mutex_lock(&ss_source->lock);
    // pthread_mutex_lock(&ss_dest->lock);

        printf("2 COPY %s -> %s\n ", source, dest);


    if(!ss_source->is_alive || !ss_dest->is_alive || source_id == -1 || dest_id == -1){
        // pthread_mutex_unlock(&ss_source->lock);
        // pthread_mutex_unlock(&ss_dest->lock);

            printf(" 3 COPY %s -> %s\n ", source, dest);


        // maybe check for backup and see what to do next

        Request res;
        res.type = ACK;
        res.error_code = FILE_NOT_FOUND;
        snprintf(res.info, MAX_INFO_SIZE, "File you are trying to access is not found!");
        printf(RED_BOLD("File Not Found!\n"));
        return res;
    }

    printf("4 COPY %s -> %s\n ", source, dest);


    char source_ip[MAX_IP_SIZE] = "";
    strcpy(source_ip, ss_source->ip);
    int source_port = ss_source->ss_port;
    // pthread_mutex_unlock(&ss_source->lock);
    // pthread_mutex_unlock(&ss_dest->lock);

    printf("5 COPY %s -> %s\n ", source, dest);


    err = put_in_cache(g_cache, source, source_id);
    if (err != CACHE_SUCCESS)
    {
        fprintf(stderr, RED("Failed to add the entry to cache :%d\n"), err);
    }
    err = put_in_cache(g_cache, dest, dest_id);
    if (err != CACHE_SUCCESS)
    {
        fprintf(stderr, RED("Failed to add the entry to cache :%d\n"), err);
    }

     printf("6 COPY %s -> %s\n ", source, dest);

    SS_Request s;
    s.error_code = OK;
    s.SS_id = dest_id;
    s.type = REQUEST_COPY;
    strcpy(s.info, "");
    strcat(s.info, source);
    strcat(s.info, "|");
    strcat(s.info, dest);
    strcat(s.info, "|");
    strcat(s.info, source_ip);
    strcat(s.info, "|");
    char port[10];
    sprintf(port, "%d", source_port);
    strcat(s.info, port);
    printf("7 COPY %s -> %s\n ", source, dest);
    Request res = send_request_to_ss(dest_id, s);
    return res;
}

// Request handle_copy(char *info) {
//     // Extract source and destination paths
//     char *source = strtok(info, "|");
//     char *dest = strtok(NULL, "|");
    
//     // Validate inputs
//     if (!source || !dest) {
//         Request res;
//         res.type = ACK;
//         res.error_code = INVALID_INPUT;
//         snprintf(res.info, MAX_INFO_SIZE, "Invalid source or destination path!");
//         return res;
//     }
    
//     // Get source server ID
//     int source_ss_id = -1;
//     pthread_mutex_lock(&g_trie_lock);
//     source_ss_id = search_path(g_directory_trie, source);
//     pthread_mutex_unlock(&g_trie_lock);
    
//     // Validate source server
//     if (source_ss_id == -1) {
//         Request res;
//         res.type = ACK;
//         res.error_code = FILE_NOT_FOUND;
//         snprintf(res.info, MAX_INFO_SIZE, "Source path not found!");
//         return res;
//     }
    
//     // Get destination server ID
//     int dest_ss_id = -1;
//     pthread_mutex_lock(&g_trie_lock);
//     dest_ss_id = search_path(g_directory_trie, dest);
//     pthread_mutex_unlock(&g_trie_lock);
    
//     // Validate destination server
//     if (dest_ss_id == -1) {
//         Request res;
//         res.type = ACK;
//         res.error_code = FILE_NOT_FOUND;
//         snprintf(res.info, MAX_INFO_SIZE, "Destination path not found!");
//         return res;
//     }
    
//     // List all paths under source with matching source server ID
//     ListNode *paths = NULL;
//     pthread_mutex_lock(&g_trie_lock);
//     paths = list_paths_under_prefix_with_ssid(g_directory_trie, source, source_ss_id);
//     pthread_mutex_unlock(&g_trie_lock);
    
//     // If no paths found, return error
//     if (paths == NULL) {
//         Request res;
//         res.type = ACK;
//         res.error_code = FILE_NOT_FOUND;
//         snprintf(res.info, MAX_INFO_SIZE, "No paths found under the source!");
//         return res;
//     }
    
//     // Prepare COPY_ADD_PATH request
//     SS_Request add_path_req;
//     add_path_req.type = COPY_ADD_PATH;
//     add_path_req.error_code = OK;
//     add_path_req.SS_id = source_ss_id;
//     snprintf(add_path_req.info, MAX_INFO_SIZE, "%s|", dest);

//     ListNode *current = paths;
//     size_t remaining = MAX_INFO_SIZE - strlen(add_path_req.info) - 1;

//     while (current != NULL) {
//         char *path = current->data;

//         // Ensure we don't exceed the buffer size
//         if (strlen(path) + 2 > remaining) {
//             // Send the current batch
//             Request batch_res = send_request_to_ss(dest_ss_id, add_path_req);
//             if (batch_res.error_code != OK) {
//                 fprintf(stderr, "Failed to send COPY_ADD_PATH request\n");
//                 free_list(paths);
//                 return batch_res;
//             }
//             // Reset info and remaining size
//             snprintf(add_path_req.info, MAX_INFO_SIZE, "%s|", dest);
//             remaining = MAX_INFO_SIZE - strlen(add_path_req.info) - 1;
//         }

//         // Append the current path to the request
//         strncat(add_path_req.info, path, remaining);
//         strncat(add_path_req.info, "|", remaining - strlen(path) - 1);
//         remaining -= (strlen(path) + 1);

//         current = current->next;
//     }

//     // Send any remaining paths in the request
//     if (strlen(add_path_req.info) > strlen(dest) + 1) {
//         Request final_batch_res = send_request_to_ss(dest_ss_id, add_path_req);
//         if (final_batch_res.error_code != OK) {
//             fprintf(stderr, "Failed to send final COPY_ADD_PATH request\n");
//             free_list(paths);
//             return final_batch_res;
//         }
//     }

//     // Process each path individually
//     current = paths;
//     while (current != NULL) {
//         char *path = current->data;

//         if (path[strlen(path) - 1] == '/') {
//             // Handle directory creation on destination server
//             SS_Request dir_req;
//             dir_req.type = COPY_CREATE_DIR;
//             dir_req.error_code = OK;
//             dir_req.SS_id = dest_ss_id;
//             snprintf(dir_req.info, MAX_INFO_SIZE, "%s/%s", dest, path + strlen(source));
//             Request dir_res = send_request_to_ss(dest_ss_id, dir_req);
//             if (dir_res.error_code != OK) {
//                 fprintf(stderr, "Failed to create directory: %s\n", dir_req.info);
//             }
//         } else {
//             // Handle file copy
//             SS_Request content_req;
//             content_req.type = REQUEST_FILE_CONTENT;
//             content_req.error_code = OK;
//             content_req.SS_id = source_ss_id;
//             strncpy(content_req.info, path, MAX_INFO_SIZE);

//             Request content_res = send_request_to_ss(source_ss_id, content_req);
//             if (content_res.error_code != OK) {
//                 fprintf(stderr, "Failed to retrieve content for file: %s\n", path);
//                 current = current->next;
//                 continue;
//             }

//             // Send file data to destination
//             SS_Request data_req;
//             data_req.type = COPY_ADD_DATA;
//             data_req.error_code = OK;
//             snprintf(data_req.info, MAX_INFO_SIZE, "%s/%s|%s", 
//                      dest, 
//                      path + strlen(source), // relative path
//                      content_res.info);

//             Request data_res = send_request_to_ss(dest_ss_id, data_req);
//             if (data_res.error_code != OK) {
//                 fprintf(stderr, "Failed to copy file data for: %s\n", path);
//             }
//         }

//         current = current->next;
//     }

//     // Free the paths list
//     free_list(paths);

//     // Send final ACK response
//     Request final_res;
//     final_res.type = ACK;
//     final_res.error_code = OK;
//     snprintf(final_res.info, MAX_INFO_SIZE, "Copy operation completed successfully");
//     return final_res;
// }