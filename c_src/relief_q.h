#ifndef RELIEF_Q_H
#define RELIEF_Q_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define RQ_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define RQ_EXPORT
#endif

#define MAX_ID_LEN 32
#define MAX_NAME_LEN 64
#define MAX_TEXT_LEN 128
#define MAX_RESOURCES 32
#define MAX_REQUESTS 256
#define MAX_HISTORY 512
#define MAX_LOG_EVENTS 120
#define MAX_JSON_BUFFER (512 * 1024) /* 512 KB buffer for rich JSON serializations */

/* Priority levels: 1 is highest (Critical), 4 is lowest (Low) */
typedef enum {
    PRIORITY_CRITICAL = 1,
    PRIORITY_HIGH     = 2,
    PRIORITY_MEDIUM   = 3,
    PRIORITY_LOW      = 4
} PriorityLevel;

/* Request lifecycle status */
typedef enum {
    STATUS_PENDING                = 0,
    STATUS_WAITING_FOR_RESOURCES  = 1,
    STATUS_PARTIALLY_ALLOCATED    = 2,
    STATUS_COMPLETED              = 3
} RequestStatus;

/* Result of an allocation step */
typedef enum {
    OUTCOME_EMPTY                 = 0,
    OUTCOME_OK                    = 1,
    OUTCOME_PARTIAL               = 2,
    OUTCOME_WAIT                  = 3,
    OUTCOME_NOT_FOUND             = 4
} AllocationOutcome;

/* -------------------------------------------------------------------------
   1. STRUCTURE: ReliefRequest
   Groups all fields of one relief request record together.
   ------------------------------------------------------------------------- */
typedef struct {
    char request_id[MAX_ID_LEN];
    char area[MAX_NAME_LEN];
    char location[MAX_NAME_LEN];
    char disaster_type[MAX_NAME_LEN];
    int population;
    char resource[MAX_NAME_LEN];
    int quantity;        /* Quantity required */
    int allocated;       /* Quantity distributed so far */
    PriorityLevel priority;
    RequestStatus status;
    char request_time[32];
    int arrival_order;   /* Global sequence counter used for FIFO tie-breaking */
} ReliefRequest;

/* -------------------------------------------------------------------------
   2. ARRAY: ResourceItem & ResourceInventory
   Contiguous array-based store of all available relief supplies.
   ------------------------------------------------------------------------- */
typedef struct {
    char name[MAX_NAME_LEN];
    int total;
    int available;
    int distributed;
} ResourceItem;

typedef struct {
    ResourceItem items[MAX_RESOURCES];
    int count;
} ResourceInventory;

/* -------------------------------------------------------------------------
   3. LINKED LIST: Singly Linked List for Completed Requests
   Dynamically holds distributed requests.
   ------------------------------------------------------------------------- */
typedef struct LLNode {
    ReliefRequest data;
    struct LLNode* next;
} LLNode;

typedef struct {
    LLNode* head;
    int size;
} LinkedList;

/* -------------------------------------------------------------------------
   4. QUEUE: Linked-list based FIFO Queue for Newly Submitted Requests
   ------------------------------------------------------------------------- */
typedef struct QNode {
    ReliefRequest data;
    struct QNode* next;
} QNode;

typedef struct {
    QNode* front;
    QNode* rear;
    int count;
} Queue;

/* -------------------------------------------------------------------------
   5. PRIORITY QUEUE: 4 Internal Queues (Critical, High, Medium, Low)
   Critical requests are served before High, then Medium, then Low.
   Within each priority level, exact FIFO arrival order is preserved.
   ------------------------------------------------------------------------- */
typedef struct {
    Queue levels[4]; /* Index 0: Critical, 1: High, 2: Medium, 3: Low */
} PriorityQueue;

/* -------------------------------------------------------------------------
   6. STACK: Array-based LIFO Stack for Allocation History & Undo
   ------------------------------------------------------------------------- */
typedef struct {
    char alloc_id[MAX_ID_LEN];
    char request_id[MAX_ID_LEN];
    char resource[MAX_NAME_LEN];
    int quantity;
    char location[MAX_NAME_LEN];
    RequestStatus status;
    char time_str[32];
} AllocationRecord;

typedef struct {
    AllocationRecord items[MAX_HISTORY];
    int top; /* -1 when empty */
} Stack;

/* -------------------------------------------------------------------------
   LOG EVENT (feeds Live Relief Operations feed)
   ------------------------------------------------------------------------- */
typedef struct {
    char time_str[32];
    char text[MAX_TEXT_LEN * 2];
} LogEvent;

typedef struct {
    LogEvent events[MAX_LOG_EVENTS];
    int count;
    int start;
} EventLog;

/* -------------------------------------------------------------------------
   ALLOCATION RESULT
   ------------------------------------------------------------------------- */
typedef struct {
    AllocationOutcome type;
    ReliefRequest req;
    int given;
    AllocationRecord record;
} AllocationResult;

/* -------------------------------------------------------------------------
   SEARCH & SORT RESULT CONTAINERS
   ------------------------------------------------------------------------- */
typedef struct {
    ReliefRequest matches[MAX_REQUESTS];
    int count;
    int scanned;
} SearchResult;

typedef struct {
    bool found;
    ReliefRequest match;
    int steps[MAX_REQUESTS];
    int step_count;
    int total_elements;
} BinarySearchResult;

/* -------------------------------------------------------------------------
   MASTER SIMULATOR STATE
   ------------------------------------------------------------------------- */
typedef struct {
    ResourceInventory inventory;
    Queue waiting_queue;
    PriorityQueue priority_queue;
    LinkedList completed_list;
    Stack history_stack;
    
    ReliefRequest all_requests[MAX_REQUESTS];
    int all_requests_count;

    char waiting_for_resources_ids[MAX_REQUESTS][MAX_ID_LEN];
    int waiting_for_resources_count;

    EventLog event_log;
    int alloc_counter;
    int request_counter;

    AllocationResult last_alloc_result;
} ReliefSystem;

/* Global state pointer */
extern ReliefSystem g_system;

/* -------------------------------------------------------------------------
   FUNCTION DECLARATIONS: DATA STRUCTURE OPERATIONS
   ------------------------------------------------------------------------- */

/* Helper string conversion */
const char* priority_to_string(PriorityLevel p);
PriorityLevel string_to_priority(const char* str);
const char* status_to_string(RequestStatus s);
RequestStatus string_to_status(const char* str);

/* Resource Inventory */
void inventory_init(ResourceInventory* inv);
ResourceItem* inventory_add_resource(ResourceInventory* inv, const char* name, int quantity);
int inventory_find_index(const ResourceInventory* inv, const char* name);
int inventory_get_available(const ResourceInventory* inv, const char* name);
int inventory_consume(ResourceInventory* inv, const char* name, int quantity);
void inventory_restore(ResourceInventory* inv, const char* name, int quantity);

/* Linked List */
void ll_init(LinkedList* list);
void ll_insert_end(LinkedList* list, const ReliefRequest* req);
ReliefRequest* ll_search(LinkedList* list, const char* request_id);
bool ll_delete(LinkedList* list, const char* request_id);
void ll_free(LinkedList* list);

/* Queue (FIFO) */
void queue_init(Queue* q);
void queue_enqueue(Queue* q, const ReliefRequest* req);
bool queue_dequeue(Queue* q, ReliefRequest* out_req);
bool queue_peek(const Queue* q, ReliefRequest* out_req);
bool queue_is_empty(const Queue* q);
void queue_free(Queue* q);

/* Priority Queue */
void pq_init(PriorityQueue* pq);
void pq_insert(PriorityQueue* pq, const ReliefRequest* req);
bool pq_dequeue(PriorityQueue* pq, ReliefRequest* out_req);
bool pq_peek(const PriorityQueue* pq, ReliefRequest* out_req);
bool pq_is_empty(const PriorityQueue* pq);
int pq_size(const PriorityQueue* pq);
int pq_level_size(const PriorityQueue* pq, PriorityLevel level);
void pq_free(PriorityQueue* pq);

/* Stack (LIFO) */
void stack_init(Stack* s);
bool stack_push(Stack* s, const AllocationRecord* record);
bool stack_pop(Stack* s, AllocationRecord* out_record);
bool stack_peek(const Stack* s, AllocationRecord* out_record);
bool stack_is_empty(const Stack* s);

/* Event Log */
void event_log_init(EventLog* log);
void event_log_add(EventLog* log, const char* text);

/* Algorithms */
SearchResult linear_search_requests(const ReliefRequest* requests, int count, const char* field, const char* keyword);
BinarySearchResult binary_search_by_id(const ReliefRequest* sorted_requests, int count, const char* request_id);
void bubble_sort_requests(ReliefRequest* requests, int count, const char* field, bool ascending);

/* Core Simulation Workflows */
void system_init(ReliefSystem* sys);
void system_reset(ReliefSystem* sys);
void system_load_sample_data(ReliefSystem* sys);
bool system_submit_request(ReliefSystem* sys, const ReliefRequest* req, bool silent);
bool system_classify_next(ReliefSystem* sys, bool silent, ReliefRequest* out_req);
int system_classify_all(ReliefSystem* sys, bool silent);
AllocationResult system_allocate_request(ReliefSystem* sys, ReliefRequest* req);
AllocationResult system_allocate_next(ReliefSystem* sys);
AllocationResult system_allocate_by_id(ReliefSystem* sys, const char* request_id);
bool system_undo_last(ReliefSystem* sys, AllocationRecord* out_record);
ReliefRequest* system_find_request(ReliefSystem* sys, const char* request_id);

#endif /* RELIEF_Q_H */
