#include "header.h"

// Function to handle the BACKUP request
void handle_backup_request(int client_sock, Reply *request, char *root_directory) {
    printf("Received BACKUP request for directory: %s\n", root_directory);

    // Connect to the backup server
    int backup_sock = ss_connect_to_ss(client_sock, request->sourceIP, request->portNo, -1);
    if (backup_sock < 0) {
        perror("Failed to connect to backup server");
        return;
    }

    // Start recursive backup of the root directory
    recursive_backup(client_sock, backup_sock, root_directory, request->dest);
    printf("Backup complete\n");
    close(backup_sock);
}

// Recursive function to traverse directories and back up files/folders
void recursive_backup(int client_sock, int backup_sock, char *source_path, char *dest_path) {
    struct stat st;

    // Check if source is a file or directory
    if (stat(source_path, &st) == -1) {
        perror("Stat failed on source path");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        // If it's a directory, create it on the backup server
        printf("Backing up directory: %s -> %s\n", source_path, dest_path);
        mkdir(dest_path, 0755);

        // Open the directory for traversal
        DIR *dir = opendir(source_path);
        if (!dir) {
            perror("Failed to open directory");
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Construct full paths for source and destination
            char src_entry_path[MAX_PATH_LENGTH];
            char dest_entry_path[MAX_PATH_LENGTH];
            snprintf(src_entry_path, sizeof(src_entry_path), "%s/%s", source_path, entry->d_name);
            snprintf(dest_entry_path, sizeof(dest_entry_path), "%s/%s", dest_path, entry->d_name);

            // Recursively back up each entry
            recursive_backup(client_sock, backup_sock, src_entry_path, dest_entry_path);
        }

        closedir(dir);
    } else if (S_ISREG(st.st_mode)) {
        // If it's a regular file, back it up
        printf("Backing up file: %s -> %s\n", source_path, dest_path);
        ss_paste_file(client_sock, backup_sock, dest_path, -1);
    }
}