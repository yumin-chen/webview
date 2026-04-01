#ifndef LIST_H
#define LIST_H

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

static inline void init_list_head(struct list_head *head) {
    head->next = head;
    head->prev = head;
}

static inline void list_add_tail(struct list_head *new_node, struct list_head *head) {
    struct list_head *prev = head->prev;
    new_node->next = head;
    new_node->prev = prev;
    prev->next = new_node;
    head->prev = new_node;
}

static inline void list_del(struct list_head *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#define list_entry(ptr, type, member) \
    ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

#endif
