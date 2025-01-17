#include "header.h"

int insert_log(int communication_type, int id, int port, RequestType type, char *info, int status_code) {
    if(type == 58 || type == 59){
        if(strlen(info) == 0)
            return -1;
    }
    info[MAX_INFO_SIZE] = '\0';
    
    FILE *logfile = fopen(LOG_FILE, "a");
    if (logfile == NULL) {
        perror("Failed to open log file");
        return -1;
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline character

    const char *comm_type_str = (communication_type == CLIENT_COMMUNICATION) ? "Client" : "Storage Server";

    fprintf(logfile, "[%s] Communication: %s, ID: %d, Port: %d, Request Type: %d, Info: %s, Status Code: %d\n",
            timestamp, comm_type_str, id, port, type, info, status_code);

    fclose(logfile);
}

void print_log() {
    FILE *logfile = fopen(LOG_FILE, "r");
    if (logfile == NULL) {
        perror("Failed to open log file");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), logfile) != NULL) {
        printf("%s", line);
    }

    fclose(logfile);
}

void setup_signal_handler() {
    signal(SIGTSTP, print_log);
}