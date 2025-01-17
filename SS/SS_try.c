#include "header.h"

// Initialize PathArray
void init_path_array(PathArray *arr, size_t initial_capacity) {
    arr->paths = malloc(initial_capacity * sizeof(char *));
    if (!arr->paths) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    arr->count = 0;
    arr->capacity = initial_capacity;
}

// Add a path to the array
void add_path(PathArray *arr, const char *path) {
    if (arr->count == arr->capacity) {
        arr->capacity *= 2;
        arr->paths = realloc(arr->paths, arr->capacity * sizeof(char *));
        if (!arr->paths) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
    }
    arr->paths[arr->count] = strdup(path); // Duplicate string
    if (!arr->paths[arr->count]) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    arr->count++;
}

// Free PathArray
void free_path_array(PathArray *arr) {
    for (size_t i = 0; i < arr->count; i++) {
        free(arr->paths[i]);
    }
    free(arr->paths);
}

// Recursive function to get all files and folders
void fetch_files_folders(const char *path, PathArray *arr) {
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (!dp) {
        fprintf(stderr, "opendir: failed to open directory %s: %s\n", path, strerror(errno));
        return;
    }

    while ((entry = readdir(dp)) != NULL) {
        // Ignore "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat statbuf;
        if (stat(full_path, &statbuf) == -1) {
            fprintf(stderr, "stat: failed to stat %s: %s\n", full_path, strerror(errno));
            continue;
        }

        // Add to array
        add_path(arr, full_path);

        // Recurse into directories
        if (S_ISDIR(statbuf.st_mode)) {
            fetch_files_folders(full_path, arr);
        }
    }

    closedir(dp);
}

// Wrapper function to return all files and folders
char **get_all_files_folders(const char *path) {
    PathArray arr;
    init_path_array(&arr, 10); // Start with a capacity of 10

    fetch_files_folders(path, &arr);

    // Add NULL terminator for compatibility
    add_path(&arr, NULL);

    return arr.paths;
}

int create_dirs(const char *file_path)
{
    char temp_path[MAX_PATH_LENGTH];
    strcpy(temp_path, file_path);

    char *dir = strtok(temp_path, "/");
    while (dir != NULL)
    {
        if (mkdir(dir, 0777) != 0 && errno != EEXIST)
        {
            return -1; // Error in creating directory
        }
        chdir(dir); // Navigate into the directory
        dir = strtok(NULL, "/");
    }

    // Return to original path
    chdir(root_directory);
    return 0;
}
