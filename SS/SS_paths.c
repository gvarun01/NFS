// #include "header.h"

// // Head of the linked list
// ListNode *accessiblePathsHead = NULL;

// // Function to add a path to the accessible paths list
// void add_accessible_path(char *path) {
//     char *path_copy = strdup(path); // Allocate memory and copy the path
//     if (!path_copy) {
//         perror("Memory allocation failed");
//         return;
//     }
//     append_to_list(&accessiblePathsHead, path_copy);
// }

// // Function to add all paths in a directory to the list (recursive)
// void add_directory_to_accessible_paths(char *base_path, char *current_path) {
//     DIR *dir = opendir(current_path);
//     if (!dir) {
//         perror("Failed to open directory");
//         return;
//     }

//     struct dirent *entry;
//     char full_path[1024];
//     char relative_path[1024];
//     struct stat path_stat;

//     while ((entry = readdir(dir)) != NULL) {
//         // Skip "." and ".."
//         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
//             continue;
//         }
//         if (entry->d_name[0] == '.') {
//             continue;
//         }

//         snprintf(full_path, sizeof(full_path), "%s/%s", current_path, entry->d_name);

//         // Get file/directory metadata
//         if (stat(full_path, &path_stat) < 0) {
//             perror("Failed to stat path");
//             continue;
//         }

//         if (S_ISDIR(path_stat.st_mode)) {
//             snprintf(relative_path, sizeof(relative_path), "./%s/", full_path + strlen(base_path) + 1);
//         } else {
//             snprintf(relative_path, sizeof(relative_path), "./%s", full_path + strlen(base_path) + 1);
//         }

//         // Add the relative path to the list
//         add_accessible_path(relative_path);

//         // If it's a directory, recurse
//         if (S_ISDIR(path_stat.st_mode)) {
//             add_directory_to_accessible_paths(base_path, full_path);
//         }
//     }

//     closedir(dir);
// }

#include "header.h"

// Head of the linked list
ListNode *accessiblePathsHead = NULL;

// Function to add a path to the accessible paths list
void add_accessible_path(char *path) {
    char *path_copy = strdup(path); // Allocate memory and copy the path
    if (!path_copy) {
        perror("Memory allocation failed");
        return;
    }
    append_to_list(&accessiblePathsHead, path_copy);
}

// Function to populate accessible paths from acc_paths.txt
void get_all_paths_from_file() {
    char *file_name = "acc_paths.txt";
    FILE *file = fopen(file_name, "r");
    if (!file) {
        perror("Failed to open acc_paths.txt");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character from the end of the line
        line[strcspn(line, "\n")] = '\0';
        if (line[0] != '\0') { // Skip empty lines
            add_accessible_path(line);
        }
    }

    fclose(file);
}

// Function to create a list of all accessible paths from root_dir
// void get_all_paths(char *root_dir) {
//     add_directory_to_accessible_paths(root_dir, root_dir);
//     return;
// }

bool is_path_accessible(const char *path, ListNode *head) {
    ListNode *current = head;
    while (current) {
        if (strncmp((char *)current->data, path, strlen(path)) == 0) {
            return true; // Path is accessible
        }
        current = current->next;
    }
    return false; // Path not found in the list
}