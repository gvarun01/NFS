#include "header.h"

// FNV-1a hash function - more efficient than previous multiplication hash
static uint32_t hash_function(const char *path)
{
    uint32_t hash = 2166136261u;
    while (*path)
    {
        hash ^= (uint8_t)*path++;
        hash *= 16777619u;
    }
    return hash;
}

// Safe string duplication
static char *safe_strdup(const char *str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str);
    if (len >= MAX_PATH_LENGTH)
        return NULL; // Path too long

    char *dup = (char *)malloc(len + 1);
    if (!dup)
        return NULL;

    strcpy(dup, str); // Safe because we checked length
    return dup;
}

// Initialize a new cache node with safety checks
static CacheNode *create_cache_node(const char *path, int ss_id)
{
    if (!path)
        return NULL;

    CacheNode *node = (CacheNode *)calloc(1, sizeof(CacheNode));
    if (!node)
        return NULL;

    node->path = safe_strdup(path);
    if (!node->path)
    {
        free(node);
        return NULL;
    }

    node->ss_id = ss_id;
    return node;
}

// Initialize the LRU Cache with fixed size
LRUCache *init_lru_cache(int requested_capacity)
{
    // Validate capacity
    if (requested_capacity < MIN_CACHE_SIZE || requested_capacity > MAX_CACHE_SIZE)
    {
        errno = EINVAL;
        return NULL;
    }

    LRUCache *cache = calloc(1, sizeof(LRUCache));
    if (!cache)
        return NULL;

    // Use placement new technique to initialize const capacity
    *(int *)&cache->capacity = requested_capacity;

    // Initialize hash table with a prime number size for better distribution
    cache->hash_size = requested_capacity * 2; // Keep load factor under 0.5
    // Round up to next prime number (implementation omitted for brevity)

    cache->hash_table = calloc(cache->hash_size, sizeof(HashEntry *));
    if (!cache->hash_table)
    {
        free(cache);
        return NULL;
    }

    // Initialize dummy nodes with invalid SS_ID
    cache->head = create_cache_node("HEAD", -1);
    cache->tail = create_cache_node("TAIL", -1);
    if (!cache->head || !cache->tail)
    {
        free(cache->hash_table);
        free(cache->head);
        free(cache->tail);
        free(cache);
        return NULL;
    }

    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;

    if (pthread_mutex_init(&cache->lock, NULL) != 0)
    {
        free(cache->head->path);
        free(cache->tail->path);
        free(cache->head);
        free(cache->tail);
        free(cache->hash_table);
        free(cache);
        return NULL;
    }

    return cache;
}

// Add node to front (most recently used position)
static void add_node(LRUCache *cache, CacheNode *node)
{
    node->prev = cache->head;
    node->next = cache->head->next;

    cache->head->next->prev = node;
    cache->head->next = node;
}

// Remove node from list
static void remove_node(CacheNode *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

// Move node to front (most recently used position)
static void move_to_head(LRUCache *cache, CacheNode *node)
{
    remove_node(node);
    add_node(cache, node);
}

// Get storage server ID for a path with error checking
cache_error_t get_from_cache(LRUCache *cache, const char *path, int *ss_id)
{
    if (!cache || !path || !ss_id)
    {
        return CACHE_ERROR_INVALID_ARG;
    }

    pthread_mutex_lock(&cache->lock);

    uint32_t hash = hash_function(path);
    int index = hash % cache->hash_size;
    HashEntry *entry = cache->hash_table[index];
    int collisions = 0;

    while (entry)
    {
        if (strcmp(entry->path, path) == 0)
        {
            CacheNode *node = entry->node;
            move_to_head(cache, node);
            *ss_id = node->ss_id;

            // Update collision statistics
            cache->collision_count += collisions;

            pthread_mutex_unlock(&cache->lock);
            return CACHE_SUCCESS;
        }
        entry = entry->next;
        collisions++;
    }

    pthread_mutex_unlock(&cache->lock);
    return CACHE_ERROR_NOT_FOUND;
}

// Remove least recently used entry
static CacheNode *evict_lru(LRUCache *cache)
{
    CacheNode *lru = cache->tail->prev;
    if (lru == cache->head)
        return NULL; // Cache is empty

    // Remove from linked list
    remove_node(lru);

    // Remove from hash table
    uint32_t hash = hash_function(lru->path);
    int index = hash % cache->hash_size;
    HashEntry **pp = &cache->hash_table[index];

    while (*pp)
    {
        HashEntry *entry = *pp;
        if (strcmp(entry->path, lru->path) == 0)
        {
            *pp = entry->next;
            free(entry->path);
            free(entry);
            break;
        }
        pp = &entry->next;
    }

    return lru;
}

// Put new entry in cache with error handling
cache_error_t put_in_cache(LRUCache *cache, const char *path, int ss_id)
{
    if (!cache || !path)
    {
        return CACHE_ERROR_INVALID_ARG;
    }

    if (strlen(path) >= MAX_PATH_LENGTH)
    {
        return CACHE_ERROR_INVALID_ARG;
    }

    pthread_mutex_lock(&cache->lock);

    // Check if path already exists
    uint32_t hash = hash_function(path);
    int index = hash % cache->hash_size;
    HashEntry *entry = cache->hash_table[index];

    while (entry)
    {
        if (strcmp(entry->path, path) == 0)
        {
            // Update existing entry
            entry->node->ss_id = ss_id;
            move_to_head(cache, entry->node);
            pthread_mutex_unlock(&cache->lock);
            return CACHE_SUCCESS;
        }
        entry = entry->next;
    }

    // Create new node
    CacheNode *new_node = create_cache_node(path, ss_id);
    if (!new_node)
    {
        pthread_mutex_unlock(&cache->lock);
        return CACHE_ERROR_MEMORY;
    }

    // If cache is full, evict LRU entry
    if (cache->size >= cache->capacity)
    {
        CacheNode *evicted = evict_lru(cache);
        if (evicted)
        {
            free(evicted->path);
            free(evicted);
            cache->size--;
        }
    }

    // Add new entry
    add_node(cache, new_node);

    // Add to hash table
    HashEntry *new_entry = malloc(sizeof(HashEntry));
    if (!new_entry)
    {
        remove_node(new_node);
        free(new_node->path);
        free(new_node);
        pthread_mutex_unlock(&cache->lock);
        return CACHE_ERROR_MEMORY;
    }

    new_entry->path = safe_strdup(path);
    if (!new_entry->path)
    {
        remove_node(new_node);
        free(new_node->path);
        free(new_node);
        free(new_entry);
        pthread_mutex_unlock(&cache->lock);
        return CACHE_ERROR_MEMORY;
    }

    new_entry->node = new_node;
    new_entry->next = cache->hash_table[index];
    cache->hash_table[index] = new_entry;

    cache->size++;

    pthread_mutex_unlock(&cache->lock);
    return CACHE_SUCCESS;
}

// Delete a specific entry from the cache
cache_error_t delete_from_cache(LRUCache *cache, const char *path)
{
    if (!cache || !path)
    {
        return CACHE_ERROR_INVALID_ARG;
    }

    pthread_mutex_lock(&cache->lock);

    uint32_t hash = hash_function(path);
    int index = hash % cache->hash_size;
    HashEntry **pp = &cache->hash_table[index];
    HashEntry *entry = *pp;

    while (entry)
    {
        if (strcmp(entry->path, path) == 0)
        {
            // Remove the entry from the hash table
            *pp = entry->next;

            // Remove the node from the doubly linked list
            remove_node(entry->node);

            // Free the node and hash entry
            free(entry->node->path);
            free(entry->node);
            free(entry->path);
            free(entry);

            cache->size--;

            pthread_mutex_unlock(&cache->lock);
            return CACHE_SUCCESS; // Successfully deleted
        }
        pp = &entry->next;
        entry = *pp;
    }

    pthread_mutex_unlock(&cache->lock);
    return CACHE_ERROR_NOT_FOUND; // Entry not found
}

// Get cache statistics
void get_cache_stats(LRUCache *cache, int *size, int *capacity, float *collision_rate)
{
    if (!cache)
        return;

    pthread_mutex_lock(&cache->lock);
    if (size)
        *size = cache->size;
    if (capacity)
        *capacity = cache->capacity;
    if (collision_rate)
    {
        *collision_rate = cache->size > 0 ? (float)cache->collision_count / cache->size : 0.0f;
    }
    pthread_mutex_unlock(&cache->lock);
}

// Clean up cache resources
void destroy_cache(LRUCache *cache)
{
    if (!cache)
        return;

    pthread_mutex_lock(&cache->lock);

    // Free all nodes and hash entries
    for (int i = 0; i < cache->hash_size; i++)
    {
        HashEntry *entry = cache->hash_table[i];
        while (entry)
        {
            HashEntry *next = entry->next;
            free(entry->path);
            if (entry->node)
            {
                free(entry->node->path);
                free(entry->node);
            }
            free(entry);
            entry = next;
        }
    }

    // Free dummy nodes
    free(cache->head->path);
    free(cache->tail->path);
    free(cache->head);
    free(cache->tail);
    free(cache->hash_table);

    pthread_mutex_unlock(&cache->lock);
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}

// int main() {
//     // Initialize cache with fixed size
//     LRUCache* cache = init_lru_cache(1024);
//     if (!cache) {
//         fprintf(stderr, "Failed to initialize cache\n");
//         return 1;
//     }

//     // Add an entry
//     cache_error_t err = put_in_cache(cache, "/path/to/file.txt", 5);
//     if (err != CACHE_SUCCESS) {
//         fprintf(stderr, "Failed to add entry to cache: %d\n", err);
//     }

//     // Retrieve an entry
//     int ss_id;
//     err = get_from_cache(cache, "/path/to/file.txt", &ss_id);
//     if (err == CACHE_SUCCESS) {
//         printf("Found server ID: %d\n", ss_id);
//     } else {
//         printf("Path not found in cache\n");
//     }

//     // Delete an entry
//      Example usage of delete_from_cache
        // cache_error_t err = delete_from_cache(cache, "/path/to/file.txt");
        // if (err == CACHE_SUCCESS)
        // {
        //     printf("Entry deleted successfully\n");
        // }
        // else if (err == CACHE_ERROR_NOT_FOUND)
        // {
        //     printf("Entry not found in cache\n");
        // }
        // else
        // {
        //     printf("Failed to delete entry: %d\n", err);
        // }

//     // Get cache statistics
//     int size, capacity;
//     float collision_rate;
//     get_cache_stats(cache, &size, &capacity, &collision_rate);
//     printf("Cache stats: size=%d, capacity=%d, collision_rate=%.2f\n",
//            size, capacity, collision_rate);

//     // Clean up
//     destroy_cache(cache);
//     return 0;
// }