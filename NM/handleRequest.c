#include "header.h"

SS_Request handle_ss_request(int ss_id, SS_Request req)
{
    int rc;
    char *token;
    char **path;
    int index;
    switch (req.type)
    {
        // case REQUEST_REGESTER:
        //     int bytes_read = strlen(req.info);
        //     if (bytes_read <= 0)
        //     {
        //         close(ss_sock);
        //         return NULL;
        //     }
        //     request.info[bytes_read] = '\0';

        //     char ip[MAX_IP_SIZE];
        //     int nm_port, client_port, ss_port;
        //     char paths[MAX_INFO_SIZE];

        //     strcpy(ip, strtok(request.info, "|"));
        //     nm_port = atoi(strtok(NULL, "|"));
        //     client_port = atoi(strtok(NULL, "|"));
        //     ss_port = atoi(strtok(NULL, "|"));

        //     printf("%s %d %d\n", ip, nm_port, client_port);
        //     int ss_id = ns_register_storage_server(ip, nm_port, client_port, ss_port);

        //     if (ss_id >= 0)
        //     {
        //         printf("Storage server %d connected\n", ss_id);
        //         g_ss_connections[ss_id].socket = ss_sock;
        //         g_ss_connections[ss_id].ss_id = ss_id;
        //         pthread_mutex_init(&g_ss_connections[ss_id].send_lock, NULL);

        //         send_response(ss_sock, REQUEST_REGESTER, OK, NULL, ss_id);

        //         // Start communication thread
        //         pthread_t comm_thread;
        //         pthread_create(&comm_thread, NULL, handle_ss_communication, &g_ss_connections[ss_id]);
        //         pthread_detach(comm_thread);

        //         // Start heartbeat monitoring for this SS
        //         // pthread_t heartbeat_thread;
        //         // int* ss_id_ptr = malloc(sizeof(int));
        //         // *ss_id_ptr = ss_id;
        //         // pthread_create(&heartbeat_thread, NULL, ping_storage_server, ss_id_ptr);
        //         // pthread_detach(heartbeat_thread);
        //     }
        //     else
        //     {
        //         send_response(ss_sock, REQUEST_REGESTER, CONNECTION_FAILED, NULL, MAX_STORAGE_SERVERS);
        //         close(ss_sock);
        //     }
        //     break;
        // for handling add paths
    // in single request
    // format :. paths separated by '|'
    case REQUEST_ADD_PATH:
        printf("Adding the paths sent by the storage server");
        // printf("%s\n", req.info);
        path = (char **)malloc(sizeof(char *) * MAX_PATH_LENGTH);
        index = 0;
        token = strtok(req.info, "|");
        printf("%s\n", token);
        while (token != NULL)
        {
            path[index] = (char *)malloc(MAX_PATH_LENGTH);
            memset(path[index], 0, MAX_PATH_LENGTH);
            strcpy(path[index], token);

            printf("P0 : %s %s\n", path[0], token);
            index += 1;
            token = strtok(NULL, "|");
        }

        printf("%s\n", path[0]);

        for (int i = 0; i < index; i++)
        {
            pthread_mutex_lock(&g_trie_lock);
            if (i == 0)
                printf("0 : %s", path[i]);
            int rc = search_path(g_directory_trie, path[i]);
            if (i == 0)
                printf("0 : %s", path[i]);
            if (rc == -1)
            {
                printf("%s\n", path[i]);
                insert_path(g_directory_trie, path[i], ss_id);
            }
            pthread_mutex_unlock(&g_trie_lock);
        }

        break;
        // send this ack to storage server about paths are being added

    case REQUEST_DELETE_PATH:
        printf("Deleting the paths sent by the storage server");
        path = (char **)malloc(sizeof(char *) * MAX_CLIENTS);
        index = 0;
        bool able_to_delete = false;
        token = strtok(req.info, "|");
        while (token != NULL)
        {
            path[index] = (char *)malloc(MAX_PATH_LENGTH);
            strcpy(path[index], token);
            index += 1;
            token = strtok(NULL, "|");
        }
        for (int i = 0; i < index; i++)
        {
            pthread_mutex_lock(&g_trie_lock);
            rc = search_path(g_directory_trie, path[i]);
            if (rc != -1)
            {
                int tick = 0;
                while (tick < 10 && !able_to_delete)
                {
                    tick++;
                    usleep(100000); // 100ms
                    able_to_delete = delete_path(g_directory_trie, path[i]);
                }

                if (able_to_delete == true)
                {
                    cache_error_t err = delete_from_cache(g_cache, path[i]);
                    if (err == CACHE_SUCCESS)
                    {
                        printf("Entry deleted successfully\n");
                    }
                    else if (err == CACHE_ERROR_NOT_FOUND)
                    {
                        printf("Entry not found in cache\n");
                    }
                    else
                    {
                        printf("Failed to delete entry: %d\n", err);
                    }
                }
                else{
                    printf("Failed to delete path: %s\n", path[i]);
                }
            }
            pthread_mutex_unlock(&g_trie_lock);
        }
    

    // send this ack to ss informing deletion of path

    break;

case REQUEST_WRITE_STARTED:
    acquire_file_lock(req.info, true);
    break;
case REQUEST_WRITE_STARTED_SYNC:
    set_file_lock_sync(req.info, true);
    break;
case REQUEST_WRITE_STARTED_ASYNC:
    set_file_lock_sync(req.info, false);
    break;
case REQUEST_READ_STARTED:
    printf("Hello 1\n");
    acquire_file_lock(req.info, false);
    printf("Hello 2\n");
    break;
case REQUEST_AUDIO_STARTED:
    acquire_file_lock(req.info, false);
    break;
case REQUEST_GET_SIZE_PERMISSIONS_STARTED:
    acquire_file_lock(req.info, false);
    break;
case REQUEST_WRITE_COMPLETED:
    release_file_lock(req.info);
    break;
case REQUEST_READ_COMPLETED:
    printf("Hello 3\n");
    release_file_lock(req.info);
    printf("Hello 4\n");
    break;
case REQUEST_AUDIO_COMPLETED:
    release_file_lock(req.info);
    break;
case REQUEST_GET_SIZE_PERMISSIONS_COMPLETED:
    release_file_lock(req.info);
    break;
case PING:
    if (req.error_code == OK && strcmp(req.info, "Ping") == 0)  
    {
        pthread_mutex_lock(&g_ss_lock);
        ss_t *ss = &g_storage_servers[ss_id];
        pthread_mutex_lock(&ss->lock);
        ss->is_alive = true;
        ss->last_ping = time(NULL);
        pthread_mutex_unlock(&ss->lock);
        pthread_mutex_unlock(&g_ss_lock);
    }
    break;

default:
    printf("Unknown request type\n");
    // me kya krun?
    return req;
    break;
}
return req;
}

Request handle_request(Request req, int thread_id, int client_sock)
{
    char *path;
    char *name;
    switch (req.type)
    {

    case REQUEST_READ:
        printf("Handling READ request: %s\n", req.info);
        path = req.info;
        return handle_read(path);
        break;

    case REQUEST_WRITE:
        printf("Handling WRITE request: %s\n", req.info);
        path = req.info;
        return handle_write(path);
        break;

    case REQUEST_GET_SIZE_AND_PERMISSIONS:
        printf("Handling GET_SIZE_AND_PERMISSIONS request: %s\n", req.info);
        path = req.info;
        return handle_get_info(path);
        break;

    case REQUEST_STREAM_AUDIO:
        printf("Handling STREAM_AUDIO request: %s\n", req.info);
        path = req.info;
        return handle_stream_audio(path);
        break;

    case REQUEST_LIST_ALL_PATHS:
        printf("Handling LIST_ALL_PATHS request: %s\n", req.info);
        return handle_list_paths(client_sock);
        break;

    case REQUEST_CREATE:
        printf("Handling CREATE request: %s\n", req.info);
        path = strtok(req.info, "|");
        name = strtok(NULL, "|");
        return handle_create(path, name, thread_id);
        break;

        // case REQUEST_CREATE_DIRECTORY:
        //     printf("Handling CREATE request: %s\n", req.info);
        //     path = strtok(req.info, "|");
        //     name = strtok(NULL, "|");
        //     return handle_create(path, name);
        //     break;

        // case REQUEST_DELETE_FILE:
        //     printf("Handling DELETE request: %s\n", req.info);
        //     return handle_delete(req.info);
        //     break;

        // case REQUEST_DELETE_DIRECTORY:
        //     printf("Handling DELETE request: %s\n", req.info);
        //     return handle_delete(req.info);
        //     break;

    case REQUEST_DELETE:
        printf("Handling DELETE request: %s\n", req.info);
        return handle_delete(req.info);
        break;

    case REQUEST_COPY:
        printf("Handling COPY request: %s\n", req.info);
        return handle_copy(req.info);
        break;

        // case REQUEST_COPY_DIRECTORY:
        //     printf("Handling COPY request: %s\n", req.info);
        //     return handle_copy(req.info);
        //     break;

    default:
        printf("Unknown request type\n");
        // me kya krun?
        return req;
        break;
    }
    return req;
}
