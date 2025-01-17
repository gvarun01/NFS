#include "header.h"

typedef struct
{
    int capacity;
    int id;
} cap_storer;

int cmp(const void *a, const void *b)
{
    cap_storer *x = (cap_storer *)a;
    cap_storer *y = (cap_storer *)b;
    return x->capacity - y->capacity;
}

void *backup(void *arg)
{
    // i believe I will call this and then we will keep on working forever
    //  pthread_mutex_lock(&g_ss_lock);
    while (1)
    {
        sleep(1);
        if (g_ss_count <= 2)
        {
            // pthread_mutex_unlock(&g_ss_lock);
            continue;
        }
        for (int i = 0; i < g_ss_count; i++)
        {
            sleep(1);
            usleep(10000);
            pthread_mutex_lock(&g_ss_lock);
            
            if (g_storage_servers[i].is_alive == 1 && g_storage_servers[i].backup1 == -1 && g_storage_servers[i].backup2 == -1)
            {
                // will back this up

                // there is no one else to like use it for backup so no issue if i lock here
                printf("LASSIBC %d\n\n\n\n\n\n",i);
                printf("%d %d %d", i,g_storage_servers[i].backup1, g_storage_servers[i].backup2);
                int bc1 = -1, bc2 = -1;
                cap_storer backup_counti[g_ss_count];
                int count = 0;
                memset(backup_counti, 0, sizeof(backup_counti));
                pthread_mutex_lock(&g_storage_servers[i].lock); // will ask oneja if we should use this or not

                for (int j = 0; j < g_ss_count; j++)
                {
                    if (i == j)
                        continue;
                    if (g_storage_servers[j].is_alive)
                    {
                        backup_counti[count].capacity = g_storage_servers[j].backups_hold;
                        backup_counti[count].id = j;
                        count++;
                    }
                }
                if (count < 2)
                {
                    pthread_mutex_unlock(&g_storage_servers[i].lock);
                    printf("CANT BACKUP THIS NOW\n");
                    // pthread_mutex_unlock(&g_ss_lock);
                    continue;;
                }
                qsort(backup_counti, count, sizeof(cap_storer), cmp);
                bc1 = backup_counti[0].id;
                bc2 = backup_counti[1].id;
                g_storage_servers[i].backup1 = bc1;
                g_storage_servers[i].backup2 = bc2;
                g_storage_servers[bc1].hold_backup[g_storage_servers[bc1].backups_hold++] = i;
                g_storage_servers[bc2].hold_backup[g_storage_servers[bc2].backups_hold++] = i;
                pthread_mutex_unlock(&g_storage_servers[i].lock);

                // pthread_t thread_id;

                if (g_storage_servers[bc1].is_alive == 1 && g_storage_servers[bc2].is_alive == 1 && bc1 != i && bc2 != i)
                {
                    // printf("Backing ")
                    // ListNode *head = list_all_paths(g_directory_trie, i);
                    // char **paths = (char **)malloc(sizeof(char *) * 1000);
                    // for (int i = 0; i < 1000; i++)
                    // {
                    //     paths[i] = (char *)malloc(sizeof(char) * MAX_PATH_LENGTH);
                    // }
                    // int idx = 0;
                    // ListNode *current = head;
                    // yaha par create kar do or fir dekhte h
                    // create karo or fir copy kr do
                    //  handle_create(,-1);
                    SS_Request create_backup1;
                    create_backup1.type = REQUEST_CREATE;
                    create_backup1.error_code = OK;
                    create_backup1.SS_id = bc1;
                    create_backup1.Thread_id = -1;
                    strcpy(create_backup1.info, "");
                    strcat(create_backup1.info, "/|");
                    strcat(create_backup1.info, "Backup/");
                    connect_and_send_to_SS(create_backup1);
                    // Request req = send_request_to_ss(i, create_backup1);
                    // printf("Error code create backup1: %d\n", req.error_code);

                    usleep(10000);
                    // now have to copy entire na ?
                    // so just copy the root directory
                    SS_Request copy_backup1;
                    copy_backup1.type = REQUEST_BACKUP_COPY;
                    copy_backup1.error_code = OK;
                    copy_backup1.SS_id = bc1;
                    copy_backup1.Thread_id = -1;
                    strcpy(copy_backup1.info, "");
                    // printf("paths[0] : %s\n paths[1]: %s\n paths[2]: %s\n", paths[0],paths[1],paths[2]);
                    strcat(copy_backup1.info, "\\");
                    strcat(copy_backup1.info, "|");
                    strcat(copy_backup1.info, "./Backup/"); // dest in backup we have to add these files
                    strcat(copy_backup1.info, "|");
                    strcat(copy_backup1.info, g_storage_servers[i].ip);
                    strcat(copy_backup1.info, "|");
                    char port[10];
                    sprintf(port, "%d", g_storage_servers[i].ss_port);
                    strcat(copy_backup1.info, port);
                    // Request req1 = send_request_to_ss(i, copy_backup1);
                    // printf("Error code copy backup1: %d\n", req1.error_code);
                    connect_and_send_to_SS(copy_backup1);
                    usleep(10000);
                    // strcat(create_backup1.info, "/");
                    // strcat(create_backup1.info, g_storage_servers[i].);
                    SS_Request create_backup2;
                    create_backup2.type = REQUEST_CREATE;
                    create_backup2.error_code = OK;
                    create_backup2.SS_id = bc2;
                    create_backup2.Thread_id = -1;
                    strcpy(create_backup2.info, "");
                    strcat(create_backup2.info, "/|");
                    strcat(create_backup2.info, "Backup/");
                    // Request req2 = send_request_to_ss(i, create_backup2);
                    // printf("Error code create backup2: %d\n", req2.error_code);
                    connect_and_send_to_SS(create_backup2);
                    usleep(10000);
                    // now have to copy entire na ?
                    // so just copy the root directory
                    SS_Request copy_backup2;
                    copy_backup2.type = REQUEST_BACKUP_COPY;
                    copy_backup2.error_code = OK;
                    copy_backup2.SS_id = bc2;
                    copy_backup2.Thread_id = -1;
                    strcpy(copy_backup2.info, "");
                    strcat(copy_backup2.info, "\\");
                    strcat(copy_backup2.info, "|");
                    strcat(copy_backup2.info, "./Backup/"); // dest in backup we have to add these files
                    strcat(copy_backup2.info, "|");
                    strcat(copy_backup2.info, g_storage_servers[i].ip);
                    strcat(copy_backup2.info, "|");
                    char port2[10];
                    sprintf(port2, "%d", g_storage_servers[i].ss_port
                    );
                    strcat(copy_backup2.info, port2);
                    // Request req3 = send_request_to_ss(i, copy_backup2);
                    // printf("Error code copy backup2: %d\n", req3.error_code);
                    connect_and_send_to_SS(copy_backup2);
                    usleep(10000);
                    // while (current)
                    // {
                    //     printf("LODU?\n");
                    //     if (!paths[idx])
                    //     {
                    //         strcpy(paths[idx], current->data);

                    //         pthread_mutex_lock(&g_storage_servers[bc1].lock);
                    //         pthread_mutex_lock(&g_storage_servers[bc1].backup_trie_lock);
                    //         int rc = search_path(g_storage_servers[bc1].backup_trie, paths[idx]);
                    //         if (rc == -1)
                    //         {
                    //             insert_path(g_storage_servers[bc1].backup_trie, paths[idx], i);
                    //         }
                    //         pthread_mutex_unlock(&g_storage_servers[bc1].backup_trie_lock);
                    //         pthread_mutex_unlock(&g_storage_servers[bc1].lock);

                    //         pthread_mutex_lock(&g_storage_servers[bc2].lock);
                    //         pthread_mutex_lock(&g_storage_servers[bc2].backup_trie_lock);
                    //         rc = search_path(g_storage_servers[bc2].backup_trie, paths[idx]);
                    //         if (rc == -1)
                    //         {
                    //             insert_path(g_storage_servers[bc2].backup_trie, paths[idx], i);
                    //         }
                    //         pthread_mutex_unlock(&g_storage_servers[bc2].backup_trie_lock);
                    //         pthread_mutex_unlock(&g_storage_servers[bc2].lock);

                    //         idx++;
                    //         current = current->next;
                    //     }
                    // }
                }
            }
            pthread_mutex_unlock(&g_ss_lock);
            // else
            // {
            //     printf("BACKUP HO GYA MITRA %d\n",i);
            // }
        }
    }
}