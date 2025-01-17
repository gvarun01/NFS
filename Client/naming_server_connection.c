#include "client_headers.h"
// #include "defs.h"

#define TIMEOUT 5


int c_connect_to_ns()
{
    int sock = 0;
    struct sockaddr_in ns_server_addr;
    // fd_set write_fds;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        // NS error = {-1, ""};
        return -1;
    }
    memset(&ns_server_addr, 0, sizeof(ns_server_addr));
    ns_server_addr.sin_family = AF_INET;
    ns_server_addr.sin_port = htons(ns_port);
    if (inet_pton(AF_INET, ns_ip, &ns_server_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        close(sock);
        // NS error = {-1, ""};
        // return error;
        return -1;
    }
        // fcntl(sock, F_SETFL, O_NONBLOCK);
        // printf("HIIHIHI");
      if (connect(sock, (struct sockaddr *)&ns_server_addr, sizeof(ns_server_addr)) < 0) {
         perror("Connection failed");
            close(sock);
            return -1;
    }
    // printf("BEYBEY\n");
    // fflush(stdout);
    return sock;
}









NS c_get_ss(int opn, char* path)
{
    int sock = c_connect_to_ns();
    NS error = {-1, ""};
    if(sock < 0)
    {
        printf("CANNOT CONNECT TO NS\n");
        return error;
    }

    // First request
    Request init;
    memset(&init, 0, sizeof(init));
    init.type = opn;
    strcpy(init.info, path);
    init.error_code = OK;
    if(send(sock, &init, sizeof(init), 0) < 0)
    {
        printf("SEND ERROR\n");
        close(sock);
        return error;
    }

    // Receive ACK
    Request ack1;
    ssize_t bytes=0;
    struct timeval tv;
        tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        perror("Failed to set socket options");
        close(sock);
        return error;
    }

    memset(&ack1, 0, sizeof(ack1));
    if((bytes=recv(sock, &ack1, sizeof(ack1), MSG_WAITALL) )<= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
        printf("timeout\n");
    }
        printf("ERROR IN READING FIRST PACKET\n");
        close(sock);
        return error;
    }
        printf("bytes - %ld\n",bytes);

    // Verify ACK
    if(ack1.type != ACK || ack1.error_code != OK)
    {
        printf("Invalid ACK received\n");
        close(sock);
        return error;
    }

    printf("ACK Received: %s\n", ack1.info);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        perror("Failed to reset socket options");
        close(sock);
        return error;
    }
    // Receive IP and Port information
    NS need;
    memset(&need, 0, sizeof(need));
    Request request;
    memset(&request, 0, sizeof(request));
    bytes=0;
    if((bytes=recv(sock, &request, sizeof(request), MSG_WAITALL)) <= 0)
    {
        printf("ERROR IN READING SECOND PACKET bcccc\n");
        close(sock);
        return error;
    }
    // printf("bytes - %d size of struct - %d\n",bytes,sizeof(Request));
    printf("%d",request.error_code);
    fflush(stdout);
    printf("request.info - %s\n\n",request.info);
    if(strcmp(request.info,"")==0)
    {
        printf("ERROR: %s\n", error_names[request.error_code]);
        close(sock);
        return error;
    }
    // Verify response
    if(request.error_code != OK)
    {
        printf("ERROR: %s\n", error_names[request.error_code]);
        close(sock);
        return error;
    }
 
    // Parse IP and port
    char* token = strtok(request.info, "|");
    strcpy(need.ip, token);
    token = strtok(NULL, "|");
    need.portNo = atoi(token);

    printf("IP: %s PORT: %d\n", need.ip, need.portNo);
    
    close(sock);
    return need;
}



































///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///now have to make three opns 
void c_create_at_path(char* path,char* name)
{
    int sock= c_connect_to_ns();
    struct timeval tv;
    if (sock == -1)
    {
        printf("CANNOT CONNECT TO NS\n");
        close(sock);    
        return;
    }
    Request init ;
    memset(&init,0,sizeof(init));
    init.type=REQUEST_CREATE;
    strcpy(init.info,path);
    strcat(init.info,"|");
    strcat(init.info,name);
    init.error_code=OK;
    if(send(sock,&init,sizeof(init),0)<0)
    {
        printf("SEND ERROR\n");
        close(sock);
        return;
    }
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        perror("Failed to set socket options");
        close(sock);
        return;
    }
    Request ack1;
    memset(&ack1, 0, sizeof(ack1));
   ssize_t bytes=0;
    if((bytes=recv(sock, &ack1, sizeof(ack1), MSG_WAITALL) )<= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
        printf("timeout\n");
    }
        printf("ERROR IN READING FIRST PACKET\n");
        close(sock);
        return;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        perror("Failed to reset socket options");
        close(sock);
        return ;
    }
    if (sock < 0)
    {
        printf("CANNOT CONNECT TO NS\n");
        close(sock);
        // NS error = {-1, ""};
        return ;
    }
    Request request;
    memset(&request,0,sizeof(request));
    // strcpy(request.type,CREATE);
    // request.type=REQUEST_CREATE;
    // strcpy(request.info,path);
    // request.error_code=OK;
    // if(send(sock,&request,sizeof(request),0)<0)
    // {
    //     printf("SEND ERROR\n");
    //     close(sock);
    //     return;
    // }
    // memset(&request,0,sizeof(request));
    
    if(request.error_code==OK)
    {
        printf("FILE CREATED\n");
        close(sock);
    }
    else
    {
        printf("ERROR: %s\n",error_names[request.error_code]);   
        close(sock);
    }
}

void c_copy_at_path(char* src,char *dest)
{
    //write will be the toughest part I believe
    int sock= c_connect_to_ns();
    if (sock == -1)
    {
        printf("CANNOT CONNECT TO SS\n");
        close(sock);    
        return;
    }
    struct timeval tv;
    Request init ;
    memset(&init,0,sizeof(init));
    // strcpy(init.type,COPY);
    init.type=REQUEST_COPY;
    // copy_type=-1;
    char info[MAX_INFO_SIZE]={0};
    strcpy(info,src);
    strcat(info,"|");
    strcat(info,dest);
    strcpy(init.info,info);
    init.error_code=OK;
    if(send(sock,&init,sizeof(init),0)<0)
    {
        printf("SEND ERROR\n");
        close(sock);
        return;;
    }
    // tv.tv_sec = TIMEOUT;
    // tv.tv_usec = 0;
    // if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
    //     perror("Failed to set socket options");
    //     close(sock);
    //     return;
    // }
    Request ack1;
    memset(&ack1, 0, sizeof(ack1));
    ssize_t bytes=0;
    while(bytes=recv(sock, &ack1, sizeof(ack1), MSG_WAITALL) <= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
        printf("timeout\n");
    }
        printf("ERROR IN READING FIRST PACKET\n");
        close(sock);
        return;
    }
    if(ack1.error_code!=OK)
    {
        printf("ERROR: %s\n",error_names[ack1.error_code]);
        close(sock);
        return ;
    }
    // tv.tv_sec = 0;
    // tv.tv_usec = 0;
    // if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
    //     perror("Failed to reset socket options");
    //     close(sock);
    //     return ;
    // }
    Request request;
    memset(&request,0,sizeof(request));
    // strcpy(request.type,COPY);
    // request.type=REQUEST_COPY;
    // char path [MAX_INFO_SIZE];
    // memset(path,0,sizeof(path));
    // strcpy(path,src);
    // strcat(path,"|");
    // strcat(path,dest);
    // strcpy(request.info,path);
    // request.error_code=OK;
    // if(send(sock,&request,sizeof(request),0)<0)
    // {
    //     printf("SEND ERROR\n");
    //     close(sock);
    //     return;
    // }
    // memset(&request,0,sizeof(request));
    // int recv_status=recv(sock,&request,sizeof(request),0);
      int recv_status=recv(sock,&request,sizeof(request),MSG_WAITALL);
    if(recv_status<=0)
    {
        printf("RECV ERROR\n");
        close(sock);
        return;
    }
    if(request.error_code==OK)
    {
        printf("FILE COPIED\n");
        close(sock);
    }
    else
    {
        printf("ERROR: %s\n",error_names[request.error_code]);   
        close(sock);
        return;
    }
}

void c_delete_at_path(char* path)
{
    int sock= c_connect_to_ns();
    if (sock == -1)
    {
        printf("CANNOT CONNECT TO SS\n");
        close(sock);    
        return;
    }
    struct timeval tv;
    Request init ;
    memset(&init,0,sizeof(init));
    // strcpy(init.type,DELETE);
    init.type=REQUEST_DELETE;
    strcpy(init.info,path);
    init.error_code=OK;
    if(send(sock,&init,sizeof(init),0)<0)
    {
        printf("SEND ERROR\n");
        close(sock);
        return;;
    }
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        perror("Failed to set socket options");
        close(sock);
        return;
    }
    Request ack1;
    memset(&ack1, 0, sizeof(ack1));
    ssize_t bytes=0;
    if((bytes=recv(sock, &ack1, sizeof(ack1), MSG_WAITALL) )<= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
        printf("timeout\n");
    }
        printf("ERROR IN READING FIRST PACKET\n");
        close(sock);
        return;
    }
    if(ack1.error_code!=OK)
    {
        printf("ERROR: %s\n",error_names[ack1.error_code]);
        close(sock);
        return ;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        perror("Failed to reset socket options");
        close(sock);
        return ;
    }
    Request request;
    memset(&request,0,sizeof(request));
    // strcpy(request.type,DELETE);
    // request.type=REQUEST_DELETE;
    // strcpy(request.info,path);
    // request.error_code=OK;
    // if(send(sock,&request,sizeof(request),0)<0)
    // {
    //     printf("SEND ERROR\n");
    //     close(sock);
    //     return;
    // }
    // memset(&request,0,sizeof(request));
    int recv_status=recv(sock,&request,sizeof(request),MSG_WAITALL);
    if(recv_status<=0)
    {
        printf("RECV ERROR\n");
        close(sock);
        return;
    }
    if(request.error_code==OK)
    {
        printf("FILE DELETED\n");
        close(sock);
    }
    else
    {
        printf("ERROR: %s\n",error_names[request.error_code]);   
        close(sock);
        return;
    }   
}

void c_list_paths()
{
    int sock= c_connect_to_ns();
    if (sock == -1)
    {
        printf("CANNOT CONNECT TO SS\n");
        close(sock);    
        return;
    }
    // Request abcd;
    // ssize_t bytes=recv(sock,&abcd,sizeof(abcd),0);
    // if(bytes<=0)
    // {
    //     printf("RECV ERROR\n");
    //     close(sock);
    //     return;
    // }
    // printf("%ld %d %s\n",bytes,abcd.type,abcd.info);
        Request init ;
    init.type=REQUEST_FILE_NOT_FOUND;
    init.error_code=OK;
    memset(&init,0,sizeof(init));
    // strcpy(init.type,REQUEST_LIST_PATHS);
    init.type=REQUEST_LIST_ALL_PATHS;
    if(send(sock,&init,sizeof(init),0)<0)
    {
        printf("SEND ERROR\n");
        close(sock);
        return;;
    }
    printf("SENT?\n");
    struct timeval tv;

    // if(send(sock,&init,sizeof(init),0)<0)
    // {
    //     printf("SEND ERROR\n");
    //     close(sock);
    //     return;
    // }
    
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        perror("Failed to set socket options");
        close(sock);
        return;
    }
    Request ack1;
    memset(&ack1, 0, sizeof(ack1));
    // 
    ssize_t bytes=0;

    if((bytes=recv(sock, &ack1, sizeof(ack1), MSG_WAITALL) )<= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
        printf("timeout\n");
    }
        printf("ERROR IN READING FIRST PACKET\n");
        close(sock);
        return;
    }
    // printf("%s JAJJAJAJJA\n",ack1.info);
    if(ack1.error_code!=OK)
    {
        printf("ERROR: %s\n",error_names[ack1.error_code]);
        close(sock);
        return ;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        perror("Failed to reset socket options");
        close(sock);
        return ;
    }
    Request request;
    memset(&request,0,sizeof(request));
    // strcpy(request.type,LIST_PATHS);
    // request.type=REQUEST_LIST_PATHS;
    // request.error_code=OK;
    // if(send(sock,&request,sizeof(request),0)<0)
    // {
    //     printf("SEND ERROR\n");
    //     close(sock);
    //     return;
    // }
    // memset(&request,0,sizeof(request));
    while(1)
    {
    // printf("Ney\n");
    int recv_status=recv(sock,&request,sizeof(request),MSG_WAITALL);
    while(recv_status<=0)
    {
        recv_status=recv(sock,&request,sizeof(request),0);
    }
    if(recv_status<=0)
    {

        printf("RECV ERROR\n");
        close(sock);
        return;
    }
    // printf("%ld %d %s NYA\n",recv_status,request.type,request.info);

    char* token = strtok(request.info, "|");
    while (token != NULL)
    {
        printf("%s\n", token);
        token = strtok(NULL, "|");
    }
    if(request.type==ACK)
    {printf("DONE\n");
        break;}
    // while (1)
    // {
    //     memset(&request,0,sizeof(request));
    //     int recv_status=recv(sock,&request,sizeof(request),0);
    //     if(recv_status<=0)
    //     {
    //         printf("RECV ERROR\n");
    //         close(sock);
    //         return;
    //     }
    //     printf("%s\n",request.info);
    //     if(request.type==ACK)
    //     {
    //         break;
    //     }
    //     if(request.error_code==OK)
    //     {
    //         char* token = strtok(request.info, "|");
    //         while (token != NULL)
    //         {
    //             printf("%s\n", token);
    //             token = strtok(NULL, "|");
    //         }
    //     }
    //     else
    //     {
    //         printf("ERROR: %s\n",error_names[request.error_code]);   
    //         close(sock);
    //         return;
    //     }
    // }
    }
    close(sock);
}