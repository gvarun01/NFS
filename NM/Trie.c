#include "header.h"

TrieNode* create_trie_node(const char *key) {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    node->key = key ? strdup(key) : NULL;
    node->is_end_of_path = false;
    node->storage_server_id = -1;
    for (int i = 0; i < 256; i++) {
        node->children[i] = NULL;
    }
    return node;
}

void insert_path(TrieNode *root, const char *path, int storage_server_id) {
    TrieNode *node = root;

    for(int i = 0; path[i] != '\0'; i++) {
        if (node->children[(unsigned char)path[i]] == NULL) {
            node->children[(unsigned char)path[i]] = create_trie_node(path);
        }
        node = node->children[(unsigned char)path[i]];
    }

    printf("Inserting path: %s\n", path);
    // node->key = strdup(path);
    node->key = (void *)path;
    node->is_end_of_path = true;
    node->storage_server_id = storage_server_id;
}

int search_path(TrieNode *root, const char *path) {
    TrieNode *node = root;
    if(root == NULL) return -1;
    while (*path) {
        if (node->children[(unsigned char)*path] == NULL) {
            return -1; // Path not found
        }
        node = node->children[(unsigned char)*path];
        path++;
    }
    return node->is_end_of_path ? node->storage_server_id : -1;
}

// bool delete_path(TrieNode *root, const char *path) {
//     if (!root) return false;

//     if (*path) {
//         if (root->children[(unsigned char)*path] != NULL &&
//             delete_path(root->children[(unsigned char)*path], path + 1) &&
//             !root->is_end_of_path) {
//             if (!root->key) {
//                 free(root->children[(unsigned char)*path]);
//                 root->children[(unsigned char)*path] = NULL;
//                 return !root->is_end_of_path && !root->key;
//             }
//         }
//     }

//     if (*path == '\0' && root->is_end_of_path) {
//         if (!root->key) {
//             root->is_end_of_path = false;
//             if (!root->key) {
//                 free(root->key);
//                 root->key = NULL;
//                 return true;
//             }
//             return false;
//         }
//     }

//     return false;
// }

// bool delete_path(TrieNode *root, const char *path) {
//     if (!root) return false;

//     // Recursive case
//     if (*path) {
//         unsigned char index = (unsigned char)*path;
//         if (root->children[index] != NULL &&
//             delete_path(root->children[index], path + 1)) {
//             free(root->children[index]);
//             root->children[index] = NULL;

//             // Check if current node is now unnecessary
//             for (int i = 0; i < 256; i++) {
//                 if (root->children[i] != NULL) return false; // Still has children
//             }
//             return !root->is_end_of_path; // Can delete if not end of another path
//         }
//         return false;
//     }

//     // Base case: end of path
//     if (*path == '\0' && root->is_end_of_path) {
//         root->is_end_of_path = false;

//         // Check if current node is unnecessary
//         for (int i = 0; i < 256; i++) {
//             if (root->children[i] != NULL) return false; // Still has children
//         }
//         return true; // Can delete this node
//     }

//     return false;
// }

// Helper function to free all nodes in a subtree
void free_subtree(TrieNode *node) {
    if (!node) return;

    // Free all children recursively
    for (int i = 0; i < 256; i++) {
        if (node->children[i] != NULL) {
            free_subtree(node->children[i]);
            node->children[i] = NULL;
        }
    }

    // Free the current node
    free(node->key);
    free(node);
}

bool delete_path(TrieNode *root, const char *path) {
    if (!root) return false;

    if (*path) {
        unsigned char index = (unsigned char)*path;

        // Proceed to the child node
        if (root->children[index] != NULL &&
            delete_path(root->children[index], path + 1)) {
            
            // Delete child if no longer needed
            free(root->children[index]);
            root->children[index] = NULL;

            // Check if current node can be deleted
            for (int i = 0; i < 256; i++) {
                if (root->children[i] != NULL) return false; // Still has children
            }
            return !root->is_end_of_path; // Can delete if not a valid path
        }

        return false;
    }

    // If we reach the end of the path
    if (*path == '\0') {
        root->is_end_of_path = false;

        // Recursively delete all children
        for (int i = 0; i < 256; i++) {
            if (root->children[i] != NULL) {
                free_subtree(root->children[i]);
                root->children[i] = NULL;
            }
        }

        // Return true if current node can be deleted
        return true;
    }

    return false;
}

void print_paths(TrieNode *root, char *buffer, int depth) {
    if (root == NULL) return;

    if (root->is_end_of_path) {
        buffer[depth] = '\0';
        printf("%s\n", buffer);
    }

    for (int i = 0; i < 256; i++) {
        if (root->children[i] != NULL) {
            buffer[depth] = i;
            print_paths(root->children[i], buffer, depth + 1);
        }
    }
}

void delete_trie(TrieNode *root) {
    if (root == NULL) return;
    for (int i = 0; i < 256; i++) {
        delete_trie(root->children[i]);
    }
    if (root->key) free(root->key);
    free(root);
}

ListNode* list_all_paths(TrieNode *root) {
    ListNode *head = NULL;
    adding_paths(root, &head);
    return head;
}

void adding_paths(TrieNode *root, ListNode **head) {
    if (root == NULL) return;

    if (root->is_end_of_path) {
        append_to_list(head, root->key);
    }

    for (int i = 0; i < 256; i++) {
        if (root->children[i] != NULL) {
            adding_paths(root->children[i], head);
        }
    }
}


ListNode *list_paths_under_prefix_with_ssid(TrieNode *root, const char *prefix, int target_ss_id)
{
    // Find the node corresponding to the prefix
    TrieNode *prefix_node = find_node_for_prefix(root, prefix);

    // If no such prefix exists, return NULL
    if (prefix_node == NULL)
    {
        return NULL;
    }

    // List to store matching paths
    ListNode *head = NULL;

    // Recursive function to add paths
    adding_paths_under_prefix_with_ssid(prefix_node, &head, target_ss_id);

    return head;
}

void adding_paths_under_prefix_with_ssid(TrieNode *root,
                                         ListNode **head, int target_ss_id)
{
    if (root == NULL)
        return;

    // If this is a complete path and matches the SS ID
    if (root->is_end_of_path && root->storage_server_id == target_ss_id)
    {
        append_to_list(head, root->key);
    }
    for (int i = 0; i < 256; i++)
    {
        if (root->children[i] != NULL)
        {
            adding_paths_under_prefix_with_ssid(root->children[i],head,target_ss_id);
        }
    }
}

// Helper function to find the node corresponding to a prefix
TrieNode *find_node_for_prefix(TrieNode *root, const char *prefix)
{
    TrieNode *current = root;

    // Traverse the trie for each character in the prefix
    for (int i = 0; prefix[i] != '\0'; i++)
    {
        unsigned char c = (unsigned char)prefix[i];

        // If this character doesn't exist in the trie, prefix not found
        if (current->children[c] == NULL)
        {
            return NULL;
        }

        // Move to the next node
        current = current->children[c];
    }

    return current;
}