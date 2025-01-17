#include "header.h"

char *root_directory = NULL;

int server_id;
int nm_sock;
int nm_port;
struct sockaddr_in nm_addr;
int ss_sock;
int ss_port;
char *root_directory;
char *nm_ip;
int nm_port;
char *ss_ip;
int ss_nm_port;
int ss_client_port;

// Function to handle client requests: Read, Write, Get Metadata, Stream Audio
void handle_client_request(int client_sock)
{
    Reply client_request;
    memset(&client_request, 0, sizeof(Reply));
    if (recv(client_sock, &client_request, sizeof(Reply), MSG_WAITALL) <= 0)
    {
        printf("Error receiving client request\n");
        return;
    }

    if (client_request.error_code != OK)
    {
        printf("Error code: %d\n", client_request.error_code);
        return;
    }

    printf("%s\n", client_request.src);

    if (client_request.operation == REQUEST_READ)
    {
        // Handle READ request by reading the specified file and sending it to the client
        ss_handle_read_request(client_sock, client_request.src);
    }
    else if (client_request.operation == REQUEST_WRITE)
    {
        // Code to handle WRITE request would go here
        ss_handle_write_request(client_sock, client_request.src, client_request.isSync);
    }
    else if (client_request.operation == REQUEST_GET_SIZE_AND_PERMISSIONS)
    {
        // Code to handle GET_SIZE_AND_PERMISSIONS request would go here
        ss_handle_get_size_and_permissions(client_sock, client_request.src);
    }
    else if (client_request.operation == REQUEST_STREAM_AUDIO)
    {
        // Code to handle STREAM_AUDIO request would go here
        ss_handle_stream_audio(client_sock, client_request.src);
    } 
    // else if (client_request.operation == REQUEST_PASTE)
    // {
    //     printf("HUDBCSHJB\n\n\n\n");
    //     // Handle READ request by reading the specified file and sending it to the client
    //     ss_copy_file_or_directory(client_sock, &client_request);
    //     // pnt_ss(client_request, client_sock);
    // }
    else
    {
        printf("Unknown operation requested by client.\n");
        Packet error_packet = {.error_code = UNKNOWN_OPERATION};
        send(client_sock, &error_packet, sizeof(error_packet), 0);
        close(client_sock);
    }
}

// Thread handler for client requests
void *client_thread_handler(void *arg)
{
    int client_socket = *(int *)arg; // Free allocated memory for the client socket
    printf("Client connected\n");
    handle_client_request(client_socket);
    pthread_exit(NULL);
}

void get_local_ip(char *buffer, size_t buffer_size) {
    int sock;
    struct sockaddr_in server_address;
    struct sockaddr_in local_address;
    socklen_t address_length = sizeof(local_address);

    // Create a UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    // Connect to a dummy address (8.8.8.8 is Google's public DNS server)
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(53); // Port 53 for DNS
    inet_pton(AF_INET, "8.8.8.8", &server_address.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection to dummy address failed");
        close(sock);
        return;
    }

    // Get the local address used by the socket
    if (getsockname(sock, (struct sockaddr *)&local_address, &address_length) < 0) {
        perror("getsockname failed");
        close(sock);
        return;
    }

    // Convert the local address to a string
    inet_ntop(AF_INET, &local_address.sin_addr, buffer, buffer_size);

    // Close the socket
    close(sock);
}


int main(int argc, char *argv[]) {
    if (argc != 5) { // Adjusted to expect six arguments
        fprintf(stderr, "Usage: %s <NM_IP> <NM_Port> <SS_Port> <Accessible_Paths>\n", argv[0]);
        return 0;
    }

    nm_ip = strdup(argv[1]);
    nm_port = atoi(argv[2]);
    
    char ip_address[MAX_IP_SIZE];
    get_local_ip(ip_address, sizeof(ip_address));
    ss_ip = strdup(ip_address); // IP address of the SS

    ss_client_port = atoi(argv[3]); // Client connection port
    root_directory = malloc(MAX_PATH_LENGTH);
    strcpy(root_directory, argv[4]); // Root directory for the SS
    // get_all_paths(root_directory);
    get_all_paths_from_file();

    int nm_sock, client_sock, ss_sock; // Sockets for NM, clients, and SS communication
    struct sockaddr_in nm_addr, client_addr, ss_addr;
    int opt = 1;

    nm_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (nm_sock < 0)
    {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    if (setsockopt(nm_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set socket options (SO_REUSEADDR)");
        close(nm_sock);
        return -1;
    }

    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(ss_client_port);  // Use nm_port for binding
    nm_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(nm_sock, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) == -1)
    {
        perror("Binding failed");
        close(nm_sock);
        pthread_exit(NULL);
    }

    if (listen(nm_sock, 500) == -1)
    {
        perror("Listening failed");
        close(nm_sock);
        pthread_exit(NULL);
    }
    printf("Listening for Naming Server requests on port: %d\n", ss_client_port);
    pthread_t nm_request_thread;
    pthread_create(&nm_request_thread, NULL, (void *)nm_request_listener, (void *)&nm_sock);

    // Initialize and bind the client socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Client socket creation failed");
        return 0;
    }

    if (setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set socket options (SO_REUSEADDR)");
        close(client_sock);
        return -1;
    }

    // Zero out the address structure
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;

    // Use inet_pton for better compatibility and safety
    if (inet_pton(AF_INET, ss_ip, &client_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(client_sock);
        return 0;
    }

    // Bind to the specified port
    client_addr.sin_port = htons(ss_client_port + 1); // Port for clients
    if (bind(client_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Client socket bind failed");
        close(client_sock);
        return 0;
    }

    // Start listening for connections
    if (listen(client_sock, 10) < 0) {  // Increased backlog from 5 to 10
        perror("Client socket listen failed");
        close(client_sock);
        return 0;
    }

    printf("Storage Server is running:\n");
    printf(" - Listening for clients on port %d\n", ss_client_port + 1);

    // Register server with Naming Server

    make_SS_socket();
    register_with_naming_server();


// Main loop to handle connections
while (1) {
    socklen_t addr_len = sizeof(client_addr);
    int *client_sock_new = malloc(sizeof(int));  // Allocate memory for the new client socket
    if (!client_sock_new) {
        perror("Memory allocation failed");
        continue;
    }

    *client_sock_new = accept(client_sock, (struct sockaddr *)&client_addr, &addr_len);
    if (*client_sock_new < 0) {
        perror("Failed to accept connection");
        free(client_sock_new);  // Free memory if accept fails
        continue;
    }

    printf("New client connected.\n");

    pthread_t ss_thread;
    if (pthread_create(&ss_thread, NULL, client_thread_handler, client_sock_new) != 0) {
        perror("Failed to create SS thread");
        close(*client_sock_new);
        free(client_sock_new);  // Free memory in case of thread creation failure
    } else {
        pthread_detach(ss_thread);
    }
}

    // Close all sockets
    close(nm_sock);
    close(client_sock);
    close(ss_sock);
    return 0;
}
