#ifndef CLIENT_HEADERS_H
#define CLIENT_HEADERS_H

#include "../common.h"

typedef struct NS
{
    int portNo;
    char ip[MAX_IP_SIZE];
}NS;

extern char ns_ip[4096];
extern int ns_port;
int c_connect_with_ss(char* ip, int port);
int c_connect_to_ns();
void c_read_from_path(char* path);
void c_get_size_and_permissions(char* path);
void c_play_audio(char* path);    
int c_connect_to_ns();
void c_create_at_path(char* path,char* name);
void c_copy_at_path(char* src,char *dest);
void c_delete_at_path(char* path);
void c_write_to_path(char* path,int sync);
NS c_get_ss(int opn,char* path);
void c_list_paths();
#endif