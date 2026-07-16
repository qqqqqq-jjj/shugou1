#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PATH 1024
#define MAX_STR  256

/* 节点类型：目录或文件 */
typedef enum { NODE_DIR, NODE_FILE } NodeType;

/* 树节点 - 表示文件系统中的一个目录或文件 */
typedef struct TreeNode {
    char *name;                 /* 文件名或目录名 */
    NodeType type;              /* 类型 */
    long size;                  /* 文件大小（字节） */
    time_t mtime;               /* 修改时间 */
    struct TreeNode *parent;    /* 父节点 */
    struct Vector *children;    /* 子节点列表（线性表） */
} TreeNode;

/* -------------------- 动态数组（线性表） -------------------- */
/* 用于存储子节点指针、搜索结果等 */
typedef struct Vector {
    void **data;        /* 存储元素的数组 */
    int size;           /* 当前元素个数 */
    int capacity;       /* 分配容量 */
} Vector;

/* 创建空向量 */
Vector* vec_create() {
    Vector *v = (Vector*)malloc(sizeof(Vector));
    v->data = (void**)malloc(sizeof(void*) * 4);
    v->size = 0;
    v->capacity = 4;
    return v;
}

/* 向向量末尾添加元素 */
void vec_push(Vector *v, void *elem) {
    if (v->size >= v->capacity) {
        v->capacity *= 2;
        v->data = (void**)realloc(v->data, sizeof(void*) * v->capacity);
    }
    v->data[v->size++] = elem;
}

/* 获取指定索引的元素 */
void* vec_get(Vector *v, int idx) {
    if (idx < 0 || idx >= v->size) return NULL;
    return v->data[idx];
}

/* 释放向量及其内部数据（不释放元素本身） */
void vec_free(Vector *v) {
    if (v) {
        free(v->data);
        free(v);
    }
}

/* -------------------- 栈（基于动态数组） -------------------- */
/* 用于非递归深度优先遍历 */
typedef struct Stack {
    Vector *vec;
} Stack;

Stack* stack_create() {
    Stack *s = (Stack*)malloc(sizeof(Stack));
    s->vec = vec_create();
    return s;
}

void stack_push(Stack *s, void *elem) {
    vec_push(s->vec, elem);
}

void* stack_pop(Stack *s) {
    if (s->vec->size == 0) return NULL;
    return s->vec->data[--s->vec->size];
}

int stack_empty(Stack *s) {
    return s->vec->size == 0;
}

void stack_free(Stack *s) {
    if (s) {
        vec_free(s->vec);
        free(s);
    }
}

/* -------------------- 队列（链式） -------------------- */
/* 用于广度优先遍历 */
typedef struct QueueNode {
    void *data;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    QueueNode *front;
    QueueNode *rear;
    int size;
} Queue;

Queue* queue_create() {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

void queue_enqueue(Queue *q, void *elem) {
    QueueNode *node = (QueueNode*)malloc(sizeof(QueueNode));
    node->data = elem;
    node->next = NULL;
    if (q->rear) {
        q->rear->next = node;
    } else {
        q->front = node;
    }
    q->rear = node;
    q->size++;
}

void* queue_dequeue(Queue *q) {
    if (!q->front) return NULL;
    QueueNode *tmp = q->front;
    void *data = tmp->data;
    q->front = q->front->next;
    if (!q->front) q->rear = NULL;
    free(tmp);
    q->size--;
    return data;
}

int queue_empty(Queue *q) {
    return q->size == 0;
}

void queue_free(Queue *q) {
    while (!queue_empty(q)) queue_dequeue(q);
    free(q);
}

/* -------------------- 字符串工具（串） -------------------- */
/* 将字符串转为小写 */
void str_tolower(char *s) {
    while (*s) {
        if (*s >= 'A' && *s <= 'Z') *s += 'a' - 'A';
        s++;
    }
}

/* 复制字符串并转为小写（返回新串，需释放） */
char* str_tolower_copy(const char *s) {
    char *copy = strdup(s);
    if (copy) str_tolower(copy);
    return copy;
}

/* 判断文件名是否以指定后缀结尾（大小写不敏感） */
int has_suffix(const char *filename, const char *suffix) {
    if (!suffix || !*suffix) return 1;  /* 空后缀匹配所有 */
    int len1 = strlen(filename);
    int len2 = strlen(suffix);
    if (len1 < len2) return 0;
    const char *p1 = filename + len1 - len2;
    const char *p2 = suffix;
    while (*p2) {
        char c1 = *p1, c2 = *p2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 'a' - 'A';
        if (c2 >= 'A' && c2 <= 'Z') c2 += 'a' - 'A';
        if (c1 != c2) return 0;
        p1++; p2++;
    }
    return 1;
}

/* 分割字符串（按分隔符），返回字符串数组（需释放） */
char** split_string(const char *str, char delim, int *count) {
    if (!str || !*str) {
        *count = 0;
        return NULL;
    }
    /* 先统计分隔符个数 */
    int cnt = 1;
    const char *p = str;
    while (*p) if (*p++ == delim) cnt++;
    char **arr = (char**)malloc(sizeof(char*) * cnt);
    int idx = 0;
    char *copy = strdup(str);
    char *token = strtok(copy, &delim);
    while (token) {
        arr[idx++] = strdup(token);
        token = strtok(NULL, &delim);
    }
    *count = idx;
    free(copy);
    return arr;
}

/* 释放字符串数组 */
void free_str_array(char **arr, int count) {
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

/* -------------------- 树节点操作 -------------------- */
/* 创建树节点 */
TreeNode* tree_node_create(const char *name, NodeType type) {
    TreeNode *node = (TreeNode*)malloc(sizeof(TreeNode));
    node->name = strdup(name);
    node->type = type;
    node->size = 0;
    node->mtime = 0;
    node->parent = NULL;
    node->children = vec_create();
    return node;
}

/* 添加子节点 */
void tree_add_child(TreeNode *parent, TreeNode *child) {
    child->parent = parent;
    vec_push(parent->children, child);
}

/* 递归释放整棵树 */
void tree_free(TreeNode *node) {
    if (!node) return;
    for (int i = 0; i < node->children->size; i++) {
        tree_free((TreeNode*)vec_get(node->children, i));
    }
    vec_free(node->children);
    free(node->name);
    free(node);
}

/* 获取节点的完整路径（返回新串，需释放） */
char* tree_get_path(TreeNode *node) {
    if (!node) return NULL;
    /* 先计算长度 */
    int len = 0;
    TreeNode *cur = node;
    while (cur) {
        len += strlen(cur->name) + 1; /* 加 '/' 或 '\0' */
        cur = cur->parent;
    }
    char *path = (char*)malloc(len + 1);
    path[0] = '\0';
    /* 从根到叶子拼接，用递归或栈，这里用栈 */
    Stack *s = stack_create();
    cur = node;
    while (cur) {
        stack_push(s, cur);
        cur = cur->parent;
    }
    int first = 1;
    while (!stack_empty(s)) {
        cur = (TreeNode*)stack_pop(s);
        if (!first) strcat(path, "/");
        strcat(path, cur->name);
        first = 0;
    }
    stack_free(s);
    return path;
}

/* -------------------- 图（邻接表） -------------------- */
/* 这里将目录树转化为无向图，方便演示图算法 */
typedef struct GraphNode {
    TreeNode *tree_node;   /* 对应的树节点 */
    Vector *neighbors;     /* 相邻图节点的索引（int*） */
} GraphNode;

typedef struct Graph {
    GraphNode **nodes;     /* 节点数组 */
    int node_count;
    int capacity;
} Graph;

Graph* graph_create() {
    Graph *g = (Graph*)malloc(sizeof(Graph));
    g->nodes = (GraphNode**)malloc(sizeof(GraphNode*) * 16);
    g->node_count = 0;
    g->capacity = 16;
    return g;
}

void graph_add_node(Graph *g, TreeNode *tn) {
    if (g->node_count >= g->capacity) {
        g->capacity *= 2;
        g->nodes = (GraphNode**)realloc(g->nodes, sizeof(GraphNode*) * g->capacity);
    }
    GraphNode *gn = (GraphNode*)malloc(sizeof(GraphNode));
    gn->tree_node = tn;
    gn->neighbors = vec_create();
    g->nodes[g->node_count++] = gn;
}

/* 添加无向边（通过索引） */
void graph_add_edge(Graph *g, int i, int j) {
    if (i < 0 || i >= g->node_count || j < 0 || j >= g->node_count) return;
    vec_push(g->nodes[i]->neighbors, (void*)(long)j);
    vec_push(g->nodes[j]->neighbors, (void*)(long)i);
}

/* 从树构建图（每个树节点对应一个图节点，父子之间加边） */
Graph* build_graph_from_tree(TreeNode *root) {
    Graph *g = graph_create();
    /* 先遍历树，添加所有节点到图，并记录映射？这里简单用递归顺序 */
    /* 由于我们需要节点索引，先进行一次DFS添加节点 */
    Stack *s = stack_create();
    stack_push(s, root);
    while (!stack_empty(s)) {
        TreeNode *cur = (TreeNode*)stack_pop(s);
        graph_add_node(g, cur);
        for (int i = 0; i < cur->children->size; i++) {
            stack_push(s, (TreeNode*)vec_get(cur->children, i));
        }
    }
    stack_free(s);
    /* 现在节点已添加，但我们需要知道每个TreeNode对应的图索引，以便加边 */
    /* 简单方法：在GraphNode中存储tree_node，然后遍历所有节点，对每个节点找其子节点在图中的索引 */
    /* 这里用双重循环匹配，效率低但演示够用 */
    for (int i = 0; i < g->node_count; i++) {
        TreeNode *parent = g->nodes[i]->tree_node;
        for (int j = 0; j < parent->children->size; j++) {
            TreeNode *child = (TreeNode*)vec_get(parent->children, j);
            /* 找到child在图中的索引 */
            for (int k = 0; k < g->node_count; k++) {
                if (g->nodes[k]->tree_node == child) {
                    graph_add_edge(g, i, k);
                    break;
                }
            }
        }
    }
    return g;
}

/* 释放图 */
void graph_free(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->node_count; i++) {
        vec_free(g->nodes[i]->neighbors);
        free(g->nodes[i]);
    }
    free(g->nodes);
    free(g);
}

/* 打印图的邻接表（用于演示） */
void graph_print(Graph *g) {
    printf("\n===== 目录图（邻接表）=====\n");
    for (int i = 0; i < g->node_count; i++) {
        TreeNode *tn = g->nodes[i]->tree_node;
        printf("[%d] %s (type=%s) -> ", i, tn->name, tn->type==NODE_DIR?"DIR":"FILE");
        for (int j = 0; j < g->nodes[i]->neighbors->size; j++) {
            int neighbor_idx = (int)(long)vec_get(g->nodes[i]->neighbors, j);
            printf("%d ", neighbor_idx);
        }
        printf("\n");
    }
}

/* BFS求两节点间最短路径（无向图），返回路径节点索引数组（需释放） */
int* graph_bfs_path(Graph *g, int start, int end, int *out_len) {
    if (start < 0 || start >= g->node_count || end < 0 || end >= g->node_count) {
        *out_len = 0;
        return NULL;
    }
    int *visited = (int*)calloc(g->node_count, sizeof(int));
    int *parent = (int*)malloc(sizeof(int) * g->node_count);
    for (int i = 0; i < g->node_count; i++) parent[i] = -1;
    Queue *q = queue_create();
    visited[start] = 1;
    queue_enqueue(q, (void*)(long)start);
    while (!queue_empty(q)) {
        int cur = (int)(long)queue_dequeue(q);
        if (cur == end) break;
        Vector *nei = g->nodes[cur]->neighbors;
        for (int i = 0; i < nei->size; i++) {
            int nb = (int)(long)vec_get(nei, i);
            if (!visited[nb]) {
                visited[nb] = 1;
                parent[nb] = cur;
                queue_enqueue(q, (void*)(long)nb);
            }
        }
    }
    queue_free(q);
    if (!visited[end]) {
        free(visited); free(parent);
        *out_len = 0;
        return NULL;
    }
    /* 重建路径 */
    int len = 0;
    int cur = end;
    while (cur != -1) {
        len++;
        cur = parent[cur];
    }
    int *path = (int*)malloc(sizeof(int) * len);
    cur = end;
    for (int i = len - 1; i >= 0; i--) {
        path[i] = cur;
        cur = parent[cur];
    }
    *out_len = len;
    free(visited); free(parent);
    return path;
}

/* -------------------- 扫描目录构建树 -------------------- */
/* 递归扫描，构建树 */
TreeNode* scan_directory(const char *path, TreeNode *parent) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return NULL;
    }
    /* 获取路径的最后一个组件作为节点名 */
    const char *name = strrchr(path, '/');
    if (!name) name = path;
    else name++; /* 跳过 '/' */
    TreeNode *node = tree_node_create(name, NODE_DIR);
    /* 获取目录自身的修改时间 */
    struct stat st;
    if (stat(path, &st) == 0) {
        node->mtime = st.st_mtime;
        node->size = st.st_size;
    }
    node->parent = parent;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char full[MAX_PATH];
        snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
        struct stat st2;
        if (stat(full, &st2) == -1) continue;
        if (S_ISDIR(st2.st_mode)) {
            /* 递归子目录 */
            TreeNode *child = scan_directory(full, node);
            if (child) tree_add_child(node, child);
        } else {
            /* 文件节点 */
            TreeNode *file_node = tree_node_create(entry->d_name, NODE_FILE);
            file_node->size = st2.st_size;
            file_node->mtime = st2.st_mtime;
            file_node->parent = node;
            tree_add_child(node, file_node);
        }
    }
    closedir(dir);
    return node;
}

/* -------------------- 搜索功能 -------------------- */
/* 在树中搜索，使用栈进行非递归DFS，同时展示栈的使用 */
Vector* search_tree(TreeNode *root, const char *keyword, int case_sensitive,
                     const char **suffixes, int suffix_count) {
    Vector *results = vec_create();  /* 存储匹配的TreeNode* */
    if (!root) return results;

    Stack *stack = stack_create();
    stack_push(stack, root);

    char *key_lower = NULL;
    if (!case_sensitive) key_lower = str_tolower_copy(keyword);

    while (!stack_empty(stack)) {
        TreeNode *cur = (TreeNode*)stack_pop(stack);
        /* 检查当前节点是否为文件，且文件名匹配 */
        if (cur->type == NODE_FILE) {
            const char *fname = cur->name;
            int match = 0;
            if (case_sensitive) {
                match = (strstr(fname, keyword) != NULL);
            } else {
                char *fname_lower = str_tolower_copy(fname);
                match = (strstr(fname_lower, key_lower) != NULL);
                free(fname_lower);
            }
            if (match) {
                /* 后缀过滤 */
                if (suffix_count > 0) {
                    int has = 0;
                    for (int i = 0; i < suffix_count; i++) {
                        if (has_suffix(fname, suffixes[i])) { has = 1; break; }
                    }
                    if (!has) match = 0;
                }
                if (match) {
                    vec_push(results, cur);
                }
            }
        }
        /* 将子节点压栈 */
        for (int i = 0; i < cur->children->size; i++) {
            stack_push(stack, (TreeNode*)vec_get(cur->children, i));
        }
    }
    if (key_lower) free(key_lower);
    stack_free(stack);
    return results;
}

/* -------------------- 显示结果 -------------------- */
void print_result(Vector *results) {
    if (results->size == 0) {
        printf("没有找到匹配的文件。\n");
        return;
    }
    printf("\n找到 %d 个文件：\n", results->size);
    printf("------------------------------------------------------------\n");
    for (int i = 0; i < results->size; i++) {
        TreeNode *node = (TreeNode*)vec_get(results, i);
        char *path = tree_get_path(node);
        char time_str[64];
        struct tm *tm_info = localtime(&node->mtime);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        printf("%d. %s\n   大小: %ld 字节   修改时间: %s\n", i+1, path, node->size, time_str);
        free(path);
    }
    printf("------------------------------------------------------------\n");
}

/* -------------------- 交互菜单 -------------------- */
void show_menu() {
    printf("\n========== 简易搜索引擎 ==========\n");
    printf("1. 扫描目录并构建索引树\n");
    printf("2. 搜索文件（需要先构建索引）\n");
    printf("3. 查找两个文件的最短路径（图演示）\n");
    printf("4. 显示目录图（邻接表）\n");
    printf("5. 退出\n");
    printf("请选择: ");
}

int main() {
    TreeNode *root = NULL;
    Graph *graph = NULL;
    char root_path[MAX_PATH];
    char keyword[MAX_STR];
    char suffix_str[MAX_STR];
    int case_sens;
    int choice;

    while (1) {
        show_menu();
        scanf("%d", &choice);
        getchar(); /* 消耗换行 */

        switch (choice) {
            case 1: {
                printf("请输入要扫描的目录路径: ");
                fgets(root_path, sizeof(root_path), stdin);
                root_path[strcspn(root_path, "\n")] = '\0';
                if (root_path[0] == '\0') {
                    printf("路径不能为空。\n");
                    break;
                }
                /* 如果已有树，先释放 */
                if (root) { tree_free(root); root = NULL; }
                if (graph) { graph_free(graph); graph = NULL; }
                printf("正在扫描目录...\n");
                root = scan_directory(root_path, NULL);
                if (!root) {
                    printf("扫描失败。\n");
                    break;
                }
                printf("扫描完成。\n");
                /* 构建图 */
                graph = build_graph_from_tree(root);
                printf("图构建完成，节点数: %d\n", graph->node_count);
                break;
            }
            case 2: {
                if (!root) {
                    printf("请先扫描目录（选项1）。\n");
                    break;
                }
                printf("请输入搜索关键字: ");
                fgets(keyword, sizeof(keyword), stdin);
                keyword[strcspn(keyword, "\n")] = '\0';
                if (keyword[0] == '\0') {
                    printf("关键字不能为空。\n");
                    break;
                }
                printf("是否大小写不敏感？(1=是, 0=否): ");
                scanf("%d", &case_sens);
                getchar();
                printf("请输入后缀过滤（多个后缀用逗号分隔，如 .txt,.c；直接回车表示不过滤）: ");
                fgets(suffix_str, sizeof(suffix_str), stdin);
                suffix_str[strcspn(suffix_str, "\n")] = '\0';
                char **suffixes = NULL;
                int suffix_count = 0;
                if (suffix_str[0] != '\0') {
                    suffixes = split_string(suffix_str, ',', &suffix_count);
                }
                /* 搜索 */
                Vector *results = search_tree(root, keyword, case_sens,
                                              (const char**)suffixes, suffix_count);
                print_result(results);
                vec_free(results);
                if (suffixes) free_str_array(suffixes, suffix_count);
                break;
            }
            case 3: {
                if (!graph) {
                    printf("请先扫描目录构建图和索引（选项1）。\n");
                    break;
                }
                printf("请输入第一个文件名（不包含路径）: ");
                char name1[MAX_STR], name2[MAX_STR];
                fgets(name1, sizeof(name1), stdin);
                name1[strcspn(name1, "\n")] = '\0';
                printf("请输入第二个文件名: ");
                fgets(name2, sizeof(name2), stdin);
                name2[strcspn(name2, "\n")] = '\0';
                /* 在图中查找对应的节点索引 */
                int idx1 = -1, idx2 = -1;
                for (int i = 0; i < graph->node_count; i++) {
                    TreeNode *tn = graph->nodes[i]->tree_node;
                    if (strcmp(tn->name, name1) == 0) idx1 = i;
                    if (strcmp(tn->name, name2) == 0) idx2 = i;
                }
                if (idx1 == -1 || idx2 == -1) {
                    printf("未找到指定文件。\n");
                    break;
                }
                int path_len;
                int *path = graph_bfs_path(graph, idx1, idx2, &path_len);
                if (!path) {
                    printf("无法找到路径（可能不连通）。\n");
                } else {
                    printf("最短路径（节点索引）: ");
                    for (int i = 0; i < path_len; i++) {
                        printf("%d ", path[i]);
                    }
                    printf("\n路径详情:\n");
                    for (int i = 0; i < path_len; i++) {
                        TreeNode *tn = graph->nodes[path[i]]->tree_node;
                        char *p = tree_get_path(tn);
                        printf("  %s\n", p);
                        free(p);
                    }
                    free(path);
                }
                break;
            }
            case 4: {
                if (!graph) {
                    printf("请先扫描目录构建图和索引（选项1）。\n");
                    break;
                }
                graph_print(graph);
                break;
            }
            case 5:
                printf("退出程序。\n");
                if (root) tree_free(root);
                if (graph) graph_free(graph);
                return 0;
            default:
                printf("无效选项，请重新选择。\n");
        }
    }
    return 0;
}
