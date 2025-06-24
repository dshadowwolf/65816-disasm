typedef struct listent_s {
    struct listent_s* parent;
    struct listent_s* child;
    void* data;
} listent_t;

listent_t* init_node(void* nodeval);
void append_node(listent_t* list, listent_t* node);
void delete_node(listent_t* list);
void delete_list(listent_t* list);
void append(listent_t* list, void* val);
