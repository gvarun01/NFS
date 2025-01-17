#include "header.h"

//idea is to use struct and sort them 
//believe using a thread will make us much better lets see what oneja says

typedef struct {
    int capacity;
    int id;
} cap_storer;

int cmp(const void *a, const void *b)
{
    cap_storer *x = (cap_storer *)a;
    cap_storer *y = (cap_storer *)b;
    return x->capacity - y->capacity;
}

void initiate_backup_transfer(int source,int backup1,int backup2)
{
    g_storage_servers[source].backup1 = backup1;
    g_storage_servers[source].backup2 = backup2;
    g_storage_servers[backup1].hold_backup[g_storage_servers[backup1].backups_hold++] = source;
    g_storage_servers[backup2].hold_backup[g_storage_servers[backup2].backups_hold++] = source;
    //now will start with backup how ? 
    //will it be a cs no so why it should matter to u at all ? 
    //huh lets start doing this now 
    SS_Request backup1_request;
    backup1_request.error_code = OK;
    backup1_request.type = REQUEST_BACKUP_DATA;
    backup1_request.SS_id=source;
    backup1_request.;//put in info the root dir of it ? naha //put the 
}
void backup()
{
    //i believe I will call this and then we will keep on working forever 
    pthread_mutex_lock(&g_ss_lock);
    if(g_ss_count<=2)
    {
        pthread_mutex_unlock(&g_ss_lock);
        return;
    }
    for(int i=0; i<g_ss_count; i++) 
    {
        if(g_storage_servers[i].is_alive==1&&g_storage_servers[i].backup1==-1&&g_storage_servers[i].backup2==-1)
        {
            //will back this up 

            // pthread_mutex_lock(&g_storage_servers[i].lock);//will ask oneja if we should use this or not 
            //there is no one else to like use it for backup so no issue if i lock here 
            int bc1,bc2;
            cap_storer backup_counti[g_ss_count];
            int count=0;
            memset(backup_counti,0,sizeof(backup_counti));
            for(int j=0;j<g_ss_count;j++)
            {
                if(i==j) continue;
                if(g_storage_servers[j].is_alive)
                {
                    backup_counti[count].capacity=g_storage_servers[j].backups_hold;
                    backup_counti[count].id=j;
                    count++; 
                }
            }
            if(count<2)
            {
                printf("CANT BACKUP THIS NOW\n");
                pthread_mutex_unlock(&g_ss_lock);
                return;
            }
            //pthread_mutex_unlock(&g_storage_servers[i].lock);
            qsort(backup_counti, count, sizeof(cap_storer), cmp);
            bc1=backup_counti[0].id;
            bc2=backup_counti[1].id;
            initiate_backup_transfer(i, bc1,bc2);
        }
    }
    pthread_mutex_unlock(&g_ss_lock);  
}