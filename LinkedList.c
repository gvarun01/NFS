#include "common.h"

ListNode* create_list_node(void *data) {
    ListNode *node = (ListNode *)malloc(sizeof(ListNode));
    node->data = data;
    node->next = NULL;
    return node;
}

void append_to_list(ListNode **head, void *data) {
    
    ListNode *new_node = create_list_node(data);
    if (*head == NULL) {
        *head = new_node;
    } else {
        ListNode *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

void delete_list_node(ListNode **head, void *data, int (*cmp)(const void *, const void *)) {
    if (*head == NULL) return;

    ListNode *current = *head;
    ListNode *prev = NULL;

    while (current != NULL && cmp(current->data, data) != 0) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) return;

    if (prev == NULL) {
        *head = current->next;
    } else {
        prev->next = current->next;
    }

    free(current);
}

void free_list(ListNode *head) {
    ListNode *current = head;
    while (current != NULL) {
        ListNode *next = current->next;
        free(current);
        current = next;
    }
}