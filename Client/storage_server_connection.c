#include "client_headers.h"
// #include "defs.h"

#define TIMEOUT 5

typedef struct
{
    int sock;
} AckThreadData;

void *wait_for_second_ack(void *arg)
{
    AckThreadData *data = (AckThreadData *)arg;
    int sock = data->sock;
    free(data); // Free the dynamically allocated memory for thread arguments

    // Packet secondAck;
    //printf("AAGAYE HAM IS THREAD ME \n");
    Request secondAck;
    memset(&secondAck, 0, sizeof(secondAck));
    if (recv(sock, &secondAck, sizeof(secondAck), MSG_WAITALL) < 0)
    {
        perror("RECV ERROR: Failed to receive second acknowledgment");
        close(sock);
        return NULL;
    }
    // printf("HELOO RECBD?\n %s %d %d\n\n",secondAck.info,secondAck.type,secondAck.error_code);
    // if (secondAck.error_code != OK)
    // {
    //     printf("ERROR: %s\n", error_names[secondAck.error_code]);
    //     close(sock);
    //     return NULL;
    // }
    if (secondAck.type == ACK_ASYNC)
    {
        // printf("ASYNC COMPLETED\n");
        printf("%s\n",secondAck.info);
    }
    // printf()
    close(sock); // Close the socket after receiving the acknowledgment
    return NULL;
}

int c_connect_with_ss(char *ip, int port)
{
    int sock = 0;
    struct sockaddr_in ss_server_addr;
    fd_set write_fds;
    struct timeval tv;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return -1;
    }

    // Setup server address
    memset(&ss_server_addr, 0, sizeof(ss_server_addr));
    ss_server_addr.sin_family = AF_INET;
    ss_server_addr.sin_port = htons(port);

    // Convert IP address
    if (inet_pton(AF_INET, ip, &ss_server_addr.sin_addr) <= 0)
    {
        perror("Invalid address / Address not supported");
        close(sock);
        return -1;
    }

    // Set the socket to non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("Failed to set non-blocking mode");
        close(sock);
        return -1;
    }

    // Attempt to connect
    if (connect(sock, (struct sockaddr *)&ss_server_addr, sizeof(ss_server_addr)) < 0)
    {
        if (errno != EINPROGRESS)
        {
            perror("Connection failed immediately");
            close(sock);
            return -1;
        }
    }

    // return sock;
    // Use select to wait for the connection to complete
    FD_ZERO(&write_fds);
    FD_SET(sock, &write_fds);
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    int result = select(sock + 1, NULL, &write_fds, NULL, &tv);
    if (result > 0)
    {
        int so_error;
        socklen_t len = sizeof(so_error);

        // Check the connection status
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0)
        {
            perror("getsockopt error");
            close(sock);
            return -1;
        }

        if (so_error == 0)
        {
            // Successfully connected
            printf("Connected to SS Server\n");
            // Restore the socket to blocking mode
            fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
            return sock;
        }
        else
        {
            fprintf(stderr, "Connection error: %s\n", strerror(so_error));
            close(sock);
            return -1;
        }
    }
    else if (result == 0)
    {
        // Timeout
        fprintf(stderr, "Connection timeout\n");
        close(sock);
        return -1;
    }
    else
    {
        // Select error
        perror("Select error");
        close(sock);
        return -1;
    }
}

    NS error = {-1, ""};

void c_read_from_path(char *path)
{
    // bring code to bring ip from ns
    //  printf("HEHEH\n");
    NS need = c_get_ss(REQUEST_READ, path);
    if(need.portNo==-1)
    {
        // printf("CANNOT CONNECT TO SS\n");
        return;
    }
    int sock = c_connect_with_ss(need.ip, need.portNo);
    if (sock < 0)
    {
        printf("CANNOT CONNECT TO SS\n");
        close(sock);
        return;
    }
    Reply reply;
    memset(&reply, 0, sizeof(reply));
    reply.error_code = OK;
    reply.portNo = need.portNo;
    reply.isSync = false;
    // strcpy(reply.operation,REQUEST_READ);
    reply.operation = REQUEST_READ;
    strcpy(reply.src, path);
    send(sock, &reply, sizeof(reply), 0);
    Request request;
    memset(&request, 0, sizeof(request));
    int recv_status;
    while (1)
    {
        recv_status = recv(sock, &request, sizeof(request), MSG_WAITALL);
        if (recv_status <= 0)
        {
            printf("ERROR IN READING PACKET\n");
            close(sock);
            return;
        }

        if (request.error_code != OK)
        {
            printf("ERROR: %s\n", error_names[request.error_code]);
            close(sock);
            return;
        }
        if (request.type == ACK)
        {
            break;
        }
        printf("%s", request.info);
    }
    printf("\n");
    close(sock);
}

void c_get_size_and_permissions(char *path)
{
    NS need = c_get_ss(REQUEST_GET_SIZE_AND_PERMISSIONS, path);
    if(need.portNo==-1)
    {
        // printf("CANNOT CONNECT TO SS\n");
        return;
    }
    int sock = c_connect_with_ss(need.ip, need.portNo);
    if (sock < 0)
    {
        printf("CANNOT CONNECT TO SS\n");
        close(sock);
        return;
    }
    Reply reply;
    memset(&reply, 0, sizeof(reply));
    reply.error_code = OK;
    reply.portNo = need.portNo;
    reply.isSync = false;
    // reply.request_type=REQUEST_GET_SIZE_AND_PERMISSIONS;
    // strcpy(reply.operation,REQUEST_GET_SIZE_AND_PERMISSIONS);
    reply.operation = REQUEST_GET_SIZE_AND_PERMISSIONS;
    strcpy(reply.src, path);
    strcpy(reply.dest, path);
    if (send(sock, &reply, sizeof(reply), 0) < 0)
    {
        printf("SEND ERROR\n");
        close(sock);
        return;
    }
    Request request;
    memset(&request, 0, sizeof(request));
    int recv_status = recv(sock, &request, sizeof(request), MSG_WAITALL);
    if (recv_status <= 0)
    {
        printf("RECV ERROR\n");
        close(sock);
        return;
    }
    // printf("%d\n",packet.error_code);
    if (request.error_code == OK)
    {
        char *token = strtok(request.info, "|");
        printf("SIZE: %s\n", token);
        token = strtok(NULL, "|");
        printf("PERMISSIONS: %s\n", token);
        // return;
    }
    else
    {
        printf("ERROR: %s\n", error_names[request.error_code]);
        close(sock);
        return;
    }
    memset(&request, 0, sizeof(request));
    if (recv(sock, &request, sizeof(request), MSG_WAITALL) <= 0)
    {
        printf("RECV ERROR\n");
        close(sock);
        return;
    }
    if (request.type == ACK)
    {
        close(sock);
    }
    else
    {
        printf("ERROR: %s\n", error_names[request.error_code]);
        close(sock);
        return;
    }
}

void c_write_to_path(char *path, int sync)
{
    int ns_sock = c_connect_to_ns();
    
    if (ns_sock == -1)
    {
        printf("CANNOT CONNECT TO NS\n");
        close(ns_sock);
        return;
    }
    struct timeval tv;
    Request init;
    memset(&init, 0, sizeof(init));
    // strcpy(init.type,WRITE);
    init.type = REQUEST_WRITE;
    strcpy(init.info, path);
    init.error_code = OK;
    if (send(ns_sock, &init, sizeof(init), 0) < 0)
    {
        printf("SEND ERROR\n");
        close(ns_sock);
        return;
    }
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(ns_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0)
    {
        perror("Failed to set socket options");
        close(ns_sock);
        return;
    }
    Request ack1;
    memset(&ack1, 0, sizeof(ack1));
    if (recv(ns_sock, &ack1, sizeof(ack1), MSG_WAITALL) <= 0)
    {
        printf("ERROR IN READING PACKET\n");
        close(ns_sock);
        return;
    }
    if (ack1.error_code != OK)
    {
        printf("ERROR: %s\n", error_names[ack1.error_code]);
        close(ns_sock);
        return;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(ns_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0)
    {
        perror("Failed to reset socket options");
        close(ns_sock);
        return;
    }
    Request request;
    memset(&request, 0, sizeof(request));
    // strcpy(request.type,WRITE);
    request.type = REQUEST_WRITE;
    strcpy(request.info, path);
    request.error_code = OK;
    if (send(ns_sock, &request, sizeof(request), 0) < 0)
    {
        printf("SEND ERROR\n");
        close(ns_sock);
        return;
    }
    memset(&request, 0, sizeof(request));

    int recv_status = recv(ns_sock, &request, sizeof(request), MSG_WAITALL);
    if (recv_status <= 0)
    {
        printf("RECV ERROR\n");
        close(ns_sock);
        return;
    }
    if (request.error_code != OK)
    {
        printf("hhihihihihh ERROR: %s\n", error_names[request.error_code]);
        close(ns_sock);
        return;
    }
    char ip[49] = {0};
    int port;
    char *token = strtok(request.info, "|");
    strcpy(ip, token);
    token = strtok(NULL, "|");
    port = atoi(token);
    // dont close ns!
    int sock = c_connect_with_ss(ip, port);
    if (sock < 0)
    {
        printf("CANNOT CONNECT TO SS\n");
        close(sock);
        return;
    }
    Reply reply;
    memset(&reply, 0, sizeof(reply));
    reply.error_code = OK;
    reply.portNo = port;
    reply.isSync = sync;
    // strcpy(reply.operation,REQUEST_WRITE);
    reply.operation = REQUEST_WRITE;
    strcpy(reply.src, path);
    strcpy(reply.dest, path);
    if (send(sock, &reply, sizeof(reply), 0) < 0)
    {
        printf("SEND ERROR\n");
        close(sock);
        return;
    }
    // size_t bytes_read;
    // int packet_number = 0;
    // char buffer[MAX_INFO_SIZE];
    // printf("Sending file to storage server...\n");
    // int fd = 0; 
    // while (1) {
    //     // Read input using read
    //     ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
        
    //     if (bytes_read < 0) {
    //         perror("Read error");
    //         break;
    //     }
    //     // Check if Enter (newline) is pressed
    //     if (memchr(buffer, '\n', bytes_read)) {
    //         printf("Enter key pressed, breaking the loop.\n");

    //         Request r;
    //         memset(&r, 0, sizeof(r));
    //         r.error_code = OK;
    //         r.type = REQ;
    //         strncpy(r.info, buffer, bytes_read);
    //         printf("%s\n", r.info);
    //         if (send(sock, &r, sizeof(r), 0) < 0)
    //         {
    //             printf("SEND ERROR\n");
    //             close(sock);
    //             return;
    //         }
    //         memset(buffer, 0, sizeof(buffer));

    //         break;  // Exit the loop if Enter is pressed
    //     }

    //     Request r;
    //     memset(&r, 0, sizeof(r));
    //     r.error_code = OK;
    //     r.type = REQ;
    //     strncpy(r.info, buffer, bytes_read);
    //     printf("%s\n", r.info);
    //     if (send(sock, &r, sizeof(r), 0) < 0)
    //     {
    //         printf("SEND ERROR\n");
    //         close(sock);
    //         return;
    //     }
    //     memset(buffer, 0, sizeof(buffer));

    // }
    // char file_name[256];
    // printf("Enter the file name to read data from: ");
    // scanf("%255s", file_name); // Get the file name from the user
    // printf("FILE NAME: %s\n",file_name);
    // int fd = open(file_name, O_RDONLY);
    // if (fd < 0) {
    //     perror("Failed to open the specified file");
    //     close(sock);
    //     return;
    // }

    // char buffer[MAX_INFO_SIZE+1];
    // size_t bytes_read;
    // int count=0;

    // while ((bytes_read = read(fd, buffer, MAX_INFO_SIZE - 10)) > 0) {
    //     Request r;
    //     memset(&r, 0, sizeof(r));
    //     r.error_code = OK;
    //     r.type = REQ;
    //     strncpy(r.info, buffer, bytes_read);
    //     // r.info[bytes_read] = '\0'; // Ensure null-termination for safety

    //     if (send(sock, &r, sizeof(r), 0) < 0) {
    //         printf("SEND ERROR\n");
    //         close(fd);
    //         close(sock);
    //         return;
    //     }
    //     // usleep(10000);
    //     // printf("%d ==== Sent %zd bytes %s\n\n\n\n\n", count++, bytes_read, r.info);
    //     memset(buffer, 0, sizeof(buffer));
    // }

    // if (bytes_read < 0) {
    //     perror("Error reading from file");
    // }

    // close(fd);

    // fclose(file);
    char filename[MAX_INFO_SIZE];
    char buffer[MAX_INFO_SIZE];

    printf("Enter the name of the file to send: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        perror("Error reading filename");
        return;
    }

    // Remove newline from filename if present
    size_t len = strlen(filename);
    if (len > 0 && filename[len - 1] == '\n') {
        filename[len - 1] = '\0';
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return;
    }

    printf("Sending file '%s' to storage server...\n", filename);

    while (1) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read < 0) {
            perror("Read error");
            close(fd);
            return;
        }

        if (bytes_read == 0) { // End of file
            break;
        }

        // Prepare the request
        Request r;
        memset(&r, 0, sizeof(r));
        r.error_code = OK;
        r.type = REQ;
        strncpy(r.info, buffer, bytes_read);

        // Send the request
        if (send(sock, &r, sizeof(r), 0) < 0) {
            perror("Send error");
            close(fd);
            return;
        }

        memset(buffer, 0, sizeof(buffer));
    }

    printf("File sent successfully!\n");

    close(fd);

    // Request request;
    memset(&request, 0, sizeof(request));
    request.type = ACK;
    request.error_code = OK;
    if (send(sock, &request, sizeof(request), 0) < 0)
    {
        printf("SEND ERROR\n");
        close(sock);
        return;
    }
    memset(&request, 0, sizeof(request));
    recv_status = recv(sock, &request, sizeof(request), MSG_WAITALL);
    if (recv_status <= 0)
    {
        printf("RECV ERROR\n");
        close(sock);
        return;
    }
    if (request.error_code != OK)
    {
        printf("ERROR: %s\n", error_names[request.error_code]);
        close(sock);
        return;
    }
    if (request.type == ACK)
    {
        // send(ns_sock,&request,sizeof(request),0);
        printf("SYNC REQUEST TAKEN\n");
        close(ns_sock);
        // memset(&request, 0, sizeof(request));
        // recv_status = recv(sock, &request, sizeof(request), 0);
        // if (recv_status <= 0)
        // {
        //     printf("RECV ERROR\n");
        //     close(sock);
        //     return;
        // }
        // if (request.error_code != OK)
        // {
        //     printf("ERROR: %s\n", error_names[request.error_code]);
        //     close(sock);
        //     return;
        // }
        // if (request.type == ACK)
        // {
        //     printf("WRITE COMPLETED\n");
        // }
        close(sock);
        printf("BYE\n");
    }
    else if (request.type == ACK_ASYNC)
    {
        // send(ns_sock,&request,sizeof(request),0);
        printf("ASYNC REQUEST TAKEN\n");
        close(sock);
        AckThreadData *data = (AckThreadData *)malloc(sizeof(AckThreadData));
        data->sock = ns_sock;
        pthread_t ack_thread;
        if (pthread_create(&ack_thread, NULL, wait_for_second_ack, data) != 0)
        {
            perror("pthread_create");
            close(ns_sock);
            return;
        }
        pthread_detach(ack_thread);
        // close(ns_sock);
    }
}

void *audio_stream_handler(void *arg)
{
    // int sockfd = *(int*)arg;
    // FILE *mpv_pipe = popen("mpv --no-terminal --quiet --ao=alsa -", "w");
    // if (!mpv_pipe) {
    //     perror("popen");
    //     close(sockfd);
    //     pthread_exit(NULL);
    // }
    // Request p;
    // memset(&p,0,sizeof(p));
    // ssize_t bytes_received;
    // int packet_number = 0;
    // while ((bytes_received = recv(sockfd, &p, sizeof(p), 0)) > 0) {
    //     packet_number++;
    //     printf("Received packet %d\n", packet_number);
    //     if(p.error_code!=OK)
    //     {
    //         printf("ERROR: %s\n",error_names[p.error_code]);
    //         break;
    //     }
    //     if(p.type==ACK)
    //     {
    //         break;
    //     }
    //     fwrite(p.info, 1, bytes_received, mpv_pipe);
    //     fflush(mpv_pipe);
    //     memset(&p,0,sizeof(p));
    // }
    //     if (bytes_received < 0) {
    //     perror("recv");
    // }
    // pclose(mpv_pipe);
    // close(sockfd);
    // pthread_exit(NULL);

    int sockfd = *(int *)arg;
    FILE *mpv_pipe = popen("mpv --no-terminal --quiet --ao=alsa -", "w");
    if (!mpv_pipe)
    {
        perror("popen");
        close(sockfd);
        pthread_exit(NULL);
    }

    Request p;
    memset(&p, 0, sizeof(p));
    ssize_t bytes_received;

    while (1)
    {
        ssize_t total_received = 0;

        // Receive the complete Request structure
        while (total_received < sizeof(Request))
        {
            ssize_t r = recv(sockfd, ((char *)&p) + total_received, sizeof(Request) - total_received, MSG_WAITALL);
            if (r <= 0)
            {
                if (r == 0)
                {
                    printf("Server closed connection.\n");
                }
                else
                {
                    perror("recv");
                }
                break;
            }
            total_received += r;
        }

        // Handle received packet
        if (p.error_code != OK)
        {
            printf("ERROR: %s\n", error_names[p.error_code]);
            break;
        }

        if (p.type == ACK)
        {
            printf("ACK Received. Exiting...\n");
            break;
        }

        printf("Received packet. Bytes: %zd\n", total_received);
-
        // Write data to mpv_pipe

        fwrite(p.info, 1, total_received, mpv_pipe);
        fflush(mpv_pipe);

        memset(&p, 0, sizeof(p));
    }

    pclose(mpv_pipe);
    close(sockfd);
    pthread_exit(NULL);
}

void c_play_audio(char *path)
{
    NS need = c_get_ss(REQUEST_STREAM_AUDIO, path);
    if(need.portNo==-1)
    {
        // printf("CANNOT CONNECT TO SS\n");
        return;
    }
    int sock = c_connect_with_ss(need.ip, need.portNo);
    if (sock < 0)
    {
        printf("CANNOT CONNECT TO SS\n");
        close(sock);
        return;
    }
    Reply reply;
    memset(&reply, 0, sizeof(reply));
    reply.error_code = OK;
    reply.portNo = need.portNo;
    reply.isSync = false;
    reply.operation = REQUEST_STREAM_AUDIO;
    // strcpy(reply.operation,REQUEST_STREAM_AUDIO);
    strcpy(reply.src, path);
    strcpy(reply.dest, path);
    if (send(sock, &reply, sizeof(reply), 0) < 0)
    {
        printf("SEND ERROR\n");
        close(sock);
        return;
    }

    pthread_t audio_thread;
    if (pthread_create(&audio_thread, NULL, audio_stream_handler, &sock) != 0)
    {
        perror("pthread_create");
        close(sock);
        return;
    }
    pthread_join(audio_thread, NULL);
    printf("Audio playback finished. Closing client.\n");
    return;
}