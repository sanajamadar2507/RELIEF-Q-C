#include "relief_q.h"

/* Case-insensitive string search helper */
static bool string_contains_ci(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    if (needle[0] == '\0') return true;

    size_t h_len = strlen(haystack);
    size_t n_len = strlen(needle);
    if (n_len > h_len) return false;

    for (size_t i = 0; i <= h_len - n_len; i++) {
        size_t j = 0;
        while (j < n_len && tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == n_len) return true;
    }
    return false;
}

static int string_compare_ci(const char* a, const char* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    while (*a && *b) {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) return diff;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* -------------------------------------------------------------------------
   STRING & ENUM CONVERSIONS
   ------------------------------------------------------------------------- */
const char* priority_to_string(PriorityLevel p) {
    switch (p) {
        case PRIORITY_CRITICAL: return "Critical";
        case PRIORITY_HIGH:     return "High";
        case PRIORITY_MEDIUM:   return "Medium";
        case PRIORITY_LOW:      return "Low";
        default:                return "Unknown";
    }
}

PriorityLevel string_to_priority(const char* str) {
    if (!str) return PRIORITY_LOW;
    if (string_compare_ci(str, "Critical") == 0) return PRIORITY_CRITICAL;
    if (string_compare_ci(str, "High") == 0)     return PRIORITY_HIGH;
    if (string_compare_ci(str, "Medium") == 0)   return PRIORITY_MEDIUM;
    if (string_compare_ci(str, "Low") == 0)      return PRIORITY_LOW;
    return PRIORITY_LOW;
}

const char* status_to_string(RequestStatus s) {
    switch (s) {
        case STATUS_PENDING:               return "Pending";
        case STATUS_WAITING_FOR_RESOURCES: return "Waiting for Resources";
        case STATUS_PARTIALLY_ALLOCATED:   return "Partially Allocated";
        case STATUS_COMPLETED:             return "Completed";
        default:                           return "Unknown";
    }
}

RequestStatus string_to_status(const char* str) {
    if (!str) return STATUS_PENDING;
    if (string_compare_ci(str, "Pending") == 0) return STATUS_PENDING;
    if (string_compare_ci(str, "Waiting for Resources") == 0) return STATUS_WAITING_FOR_RESOURCES;
    if (string_compare_ci(str, "Partially Allocated") == 0) return STATUS_PARTIALLY_ALLOCATED;
    if (string_compare_ci(str, "Completed") == 0) return STATUS_COMPLETED;
    return STATUS_PENDING;
}

/* -------------------------------------------------------------------------
   RESOURCE INVENTORY (ARRAY)
   ------------------------------------------------------------------------- */
void inventory_init(ResourceInventory* inv) {
    if (!inv) return;
    inv->count = 0;
    memset(inv->items, 0, sizeof(inv->items));
}

int inventory_find_index(const ResourceInventory* inv, const char* name) {
    if (!inv || !name) return -1;
    for (int i = 0; i < inv->count; i++) {
        if (string_compare_ci(inv->items[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

ResourceItem* inventory_add_resource(ResourceInventory* inv, const char* name, int quantity) {
    if (!inv || !name || quantity <= 0) return NULL;
    int idx = inventory_find_index(inv, name);
    if (idx != -1) {
        inv->items[idx].total += quantity;
        inv->items[idx].available += quantity;
        return &inv->items[idx];
    }
    if (inv->count >= MAX_RESOURCES) return NULL;

    int new_idx = inv->count++;
    strncpy(inv->items[new_idx].name, name, MAX_NAME_LEN - 1);
    inv->items[new_idx].name[MAX_NAME_LEN - 1] = '\0';
    inv->items[new_idx].total = quantity;
    inv->items[new_idx].available = quantity;
    inv->items[new_idx].distributed = 0;
    return &inv->items[new_idx];
}

int inventory_get_available(const ResourceInventory* inv, const char* name) {
    int idx = inventory_find_index(inv, name);
    return (idx == -1) ? 0 : inv->items[idx].available;
}

int inventory_consume(ResourceInventory* inv, const char* name, int quantity) {
    int idx = inventory_find_index(inv, name);
    if (idx == -1 || quantity <= 0) return 0;
    int avail = inv->items[idx].available;
    int given = (avail < quantity) ? avail : quantity;
    inv->items[idx].available -= given;
    inv->items[idx].distributed += given;
    return given;
}

void inventory_restore(ResourceInventory* inv, const char* name, int quantity) {
    int idx = inventory_find_index(inv, name);
    if (idx == -1 || quantity <= 0) return;
    inv->items[idx].available += quantity;
    inv->items[idx].distributed -= quantity;
    if (inv->items[idx].distributed < 0) inv->items[idx].distributed = 0;
}

/* -------------------------------------------------------------------------
   LINKED LIST (FOR COMPLETED / DISTRIBUTED REQUESTS)
   ------------------------------------------------------------------------- */
void ll_init(LinkedList* list) {
    if (!list) return;
    list->head = NULL;
    list->size = 0;
}

void ll_insert_end(LinkedList* list, const ReliefRequest* req) {
    if (!list || !req) return;
    LLNode* node = (LLNode*)malloc(sizeof(LLNode));
    if (!node) return;
    node->data = *req;
    node->next = NULL;

    if (!list->head) {
        list->head = node;
    } else {
        LLNode* curr = list->head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = node;
    }
    list->size++;
}

ReliefRequest* ll_search(LinkedList* list, const char* request_id) {
    if (!list || !request_id) return NULL;
    LLNode* curr = list->head;
    while (curr != NULL) {
        if (strcmp(curr->data.request_id, request_id) == 0) {
            return &curr->data;
        }
        curr = curr->next;
    }
    return NULL;
}

bool ll_delete(LinkedList* list, const char* request_id) {
    if (!list || !list->head || !request_id) return false;
    LLNode* curr = list->head;
    LLNode* prev = NULL;

    while (curr != NULL) {
        if (strcmp(curr->data.request_id, request_id) == 0) {
            if (prev == NULL) {
                list->head = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            list->size--;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}

void ll_free(LinkedList* list) {
    if (!list) return;
    LLNode* curr = list->head;
    while (curr != NULL) {
        LLNode* temp = curr;
        curr = curr->next;
        free(temp);
    }
    list->head = NULL;
    list->size = 0;
}

/* -------------------------------------------------------------------------
   QUEUE (FIFO)
   ------------------------------------------------------------------------- */
void queue_init(Queue* q) {
    if (!q) return;
    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
}

void queue_enqueue(Queue* q, const ReliefRequest* req) {
    if (!q || !req) return;
    QNode* node = (QNode*)malloc(sizeof(QNode));
    if (!node) return;
    node->data = *req;
    node->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    q->count++;
}

bool queue_dequeue(Queue* q, ReliefRequest* out_req) {
    if (!q || q->front == NULL) return false;
    QNode* node = q->front;
    if (out_req) {
        *out_req = node->data;
    }
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    q->count--;
    free(node);
    return true;
}

bool queue_peek(const Queue* q, ReliefRequest* out_req) {
    if (!q || q->front == NULL) return false;
    if (out_req) {
        *out_req = q->front->data;
    }
    return true;
}

bool queue_is_empty(const Queue* q) {
    return (q == NULL || q->count == 0 || q->front == NULL);
}

void queue_free(Queue* q) {
    if (!q) return;
    ReliefRequest temp;
    while (queue_dequeue(q, &temp)) {
        /* deallocate all nodes */
    }
}

/* -------------------------------------------------------------------------
   PRIORITY QUEUE (4 INTERNAL QUEUES: CRITICAL, HIGH, MEDIUM, LOW)
   ------------------------------------------------------------------------- */
void pq_init(PriorityQueue* pq) {
    if (!pq) return;
    for (int i = 0; i < 4; i++) {
        queue_init(&pq->levels[i]);
    }
}

void pq_insert(PriorityQueue* pq, const ReliefRequest* req) {
    if (!pq || !req) return;
    int idx = (int)req->priority - 1;
    if (idx < 0 || idx > 3) idx = 3; /* default to Low if unexpected */
    queue_enqueue(&pq->levels[idx], req);
}

bool pq_dequeue(PriorityQueue* pq, ReliefRequest* out_req) {
    if (!pq) return false;
    for (int i = 0; i < 4; i++) {
        if (!queue_is_empty(&pq->levels[i])) {
            return queue_dequeue(&pq->levels[i], out_req);
        }
    }
    return false;
}

bool pq_peek(const PriorityQueue* pq, ReliefRequest* out_req) {
    if (!pq) return false;
    for (int i = 0; i < 4; i++) {
        if (!queue_is_empty(&pq->levels[i])) {
            return queue_peek(&pq->levels[i], out_req);
        }
    }
    return false;
}

bool pq_is_empty(const PriorityQueue* pq) {
    if (!pq) return true;
    for (int i = 0; i < 4; i++) {
        if (!queue_is_empty(&pq->levels[i])) return false;
    }
    return true;
}

int pq_size(const PriorityQueue* pq) {
    if (!pq) return 0;
    int total = 0;
    for (int i = 0; i < 4; i++) {
        total += pq->levels[i].count;
    }
    return total;
}

int pq_level_size(const PriorityQueue* pq, PriorityLevel level) {
    if (!pq) return 0;
    int idx = (int)level - 1;
    if (idx < 0 || idx > 3) return 0;
    return pq->levels[idx].count;
}

void pq_free(PriorityQueue* pq) {
    if (!pq) return;
    for (int i = 0; i < 4; i++) {
        queue_free(&pq->levels[i]);
    }
}

/* -------------------------------------------------------------------------
   STACK (LIFO FOR ALLOCATION HISTORY & UNDO)
   ------------------------------------------------------------------------- */
void stack_init(Stack* s) {
    if (!s) return;
    s->top = -1;
}

bool stack_push(Stack* s, const AllocationRecord* record) {
    if (!s || !record || s->top >= MAX_HISTORY - 1) return false;
    s->items[++s->top] = *record;
    return true;
}

bool stack_pop(Stack* s, AllocationRecord* out_record) {
    if (!s || s->top < 0) return false;
    if (out_record) {
        *out_record = s->items[s->top];
    }
    s->top--;
    return true;
}

bool stack_peek(const Stack* s, AllocationRecord* out_record) {
    if (!s || s->top < 0) return false;
    if (out_record) {
        *out_record = s->items[s->top];
    }
    return true;
}

bool stack_is_empty(const Stack* s) {
    return (s == NULL || s->top < 0);
}

/* -------------------------------------------------------------------------
   EVENT LOG (CIRCULAR BUFFER)
   ------------------------------------------------------------------------- */
void event_log_init(EventLog* log) {
    if (!log) return;
    log->count = 0;
    log->start = 0;
}

void event_log_add(EventLog* log, const char* text) {
    if (!log || !text) return;
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    int insert_idx;
    if (log->count < MAX_LOG_EVENTS) {
        insert_idx = log->count++;
    } else {
        insert_idx = log->start;
        log->start = (log->start + 1) % MAX_LOG_EVENTS;
    }

    if (tm_info) {
        strftime(log->events[insert_idx].time_str, sizeof(log->events[insert_idx].time_str), "%H:%M:%S", tm_info);
    } else {
        strcpy(log->events[insert_idx].time_str, "--:--:--");
    }
    strncpy(log->events[insert_idx].text, text, sizeof(log->events[insert_idx].text) - 1);
    log->events[insert_idx].text[sizeof(log->events[insert_idx].text) - 1] = '\0';
}

/* -------------------------------------------------------------------------
   SEARCHING ALGORITHMS
   ------------------------------------------------------------------------- */

/* Linear Search: checks every element one by one -> O(n) */
SearchResult linear_search_requests(const ReliefRequest* requests, int count, const char* field, const char* keyword) {
    SearchResult res;
    res.count = 0;
    res.scanned = count;
    if (!requests || count <= 0 || !keyword) return res;

    for (int i = 0; i < count; i++) {
        bool match = false;
        if (!field || strcmp(field, "all") == 0) {
            match = string_contains_ci(requests[i].request_id, keyword) ||
                    string_contains_ci(requests[i].area, keyword) ||
                    string_contains_ci(requests[i].location, keyword) ||
                    string_contains_ci(requests[i].disaster_type, keyword) ||
                    string_contains_ci(requests[i].resource, keyword) ||
                    string_contains_ci(priority_to_string(requests[i].priority), keyword) ||
                    string_contains_ci(status_to_string(requests[i].status), keyword);
        } else if (strcmp(field, "requestID") == 0) {
            match = string_contains_ci(requests[i].request_id, keyword);
        } else if (strcmp(field, "location") == 0) {
            match = string_contains_ci(requests[i].location, keyword);
        } else if (strcmp(field, "area") == 0) {
            match = string_contains_ci(requests[i].area, keyword);
        } else if (strcmp(field, "disasterType") == 0) {
            match = string_contains_ci(requests[i].disaster_type, keyword);
        } else if (strcmp(field, "resource") == 0) {
            match = string_contains_ci(requests[i].resource, keyword);
        } else if (strcmp(field, "priority") == 0) {
            match = string_contains_ci(priority_to_string(requests[i].priority), keyword);
        }

        if (match) {
            if (res.count < MAX_REQUESTS) {
                res.matches[res.count++] = requests[i];
            }
        }
    }
    return res;
}

/* Binary Search: O(log n) on sorted array by request_id */
BinarySearchResult binary_search_by_id(const ReliefRequest* sorted_requests, int count, const char* request_id) {
    BinarySearchResult res;
    res.found = false;
    res.step_count = 0;
    res.total_elements = count;
    memset(&res.match, 0, sizeof(res.match));

    if (!sorted_requests || count <= 0 || !request_id) return res;

    int low = 0;
    int high = count - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (res.step_count < MAX_REQUESTS) {
            res.steps[res.step_count++] = mid;
        }

        int cmp = strcmp(sorted_requests[mid].request_id, request_id);
        if (cmp == 0) {
            res.found = true;
            res.match = sorted_requests[mid];
            return res;
        } else if (cmp < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return res;
}

/* -------------------------------------------------------------------------
   SORTING ALGORITHM: BUBBLE SORT -> O(n^2)
   Repeatedly compares adjacent elements and swaps them if out of order.
   ------------------------------------------------------------------------- */
void bubble_sort_requests(ReliefRequest* requests, int count, const char* field, bool ascending) {
    if (!requests || count <= 1) return;

    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            bool should_swap = false;

            if (field == NULL || strcmp(field, "priority") == 0) {
                /* Priority order: Critical (1) is highest priority, Low (4) is lowest */
                int a = (int)requests[j].priority;
                int b = (int)requests[j + 1].priority;
                should_swap = ascending ? (a > b) : (a < b);
            } else if (strcmp(field, "population") == 0) {
                int a = requests[j].population;
                int b = requests[j + 1].population;
                should_swap = ascending ? (a > b) : (a < b);
            } else if (strcmp(field, "quantity") == 0) {
                int a = requests[j].quantity;
                int b = requests[j + 1].quantity;
                should_swap = ascending ? (a > b) : (a < b);
            } else if (strcmp(field, "requestID") == 0) {
                int cmp = strcmp(requests[j].request_id, requests[j + 1].request_id);
                should_swap = ascending ? (cmp > 0) : (cmp < 0);
            } else if (strcmp(field, "area") == 0) {
                int cmp = strcmp(requests[j].area, requests[j + 1].area);
                should_swap = ascending ? (cmp > 0) : (cmp < 0);
            } else if (strcmp(field, "location") == 0) {
                int cmp = strcmp(requests[j].location, requests[j + 1].location);
                should_swap = ascending ? (cmp > 0) : (cmp < 0);
            }

            if (should_swap) {
                ReliefRequest temp = requests[j];
                requests[j] = requests[j + 1];
                requests[j + 1] = temp;
            }
        }
    }
}
