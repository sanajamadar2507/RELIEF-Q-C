#include "relief_q.h"

ReliefSystem g_system;

/* Safe copy helper avoiding truncation warnings */
static void safe_copy(char* dest, const char* src, size_t max_len) {
    if (!dest || max_len == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    size_t i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Helper to get formatted current time */
static void get_current_time_str(char* dest, size_t max_len) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    if (tm_info) {
        strftime(dest, max_len, "%H:%M:%S", tm_info);
    } else {
        safe_copy(dest, "12:00:00", max_len);
    }
}

void system_init(ReliefSystem* sys) {
    if (!sys) return;
    inventory_init(&sys->inventory);
    queue_init(&sys->waiting_queue);
    pq_init(&sys->priority_queue);
    ll_init(&sys->completed_list);
    stack_init(&sys->history_stack);
    event_log_init(&sys->event_log);

    sys->all_requests_count = 0;
    sys->waiting_for_resources_count = 0;
    sys->alloc_counter = 100;
    sys->request_counter = 0;
    memset(&sys->last_alloc_result, 0, sizeof(sys->last_alloc_result));
    sys->last_alloc_result.type = OUTCOME_EMPTY;
}

void system_reset(ReliefSystem* sys) {
    if (!sys) return;
    queue_free(&sys->waiting_queue);
    pq_free(&sys->priority_queue);
    ll_free(&sys->completed_list);
    system_init(sys);
}

void system_load_sample_data(ReliefSystem* sys) {
    if (!sys) return;
    system_reset(sys);

    /* 1. Load initial 7 resources */
    inventory_add_resource(&sys->inventory, "Food", 1000);
    inventory_add_resource(&sys->inventory, "Water", 1200);
    inventory_add_resource(&sys->inventory, "Medicines", 500);
    inventory_add_resource(&sys->inventory, "Blankets", 800);
    inventory_add_resource(&sys->inventory, "Tents", 300);
    inventory_add_resource(&sys->inventory, "Clothes", 600);
    inventory_add_resource(&sys->inventory, "Emergency Kits", 250);

    /* 2. Load 10 sample requests */
    struct {
        const char* id;
        const char* area;
        const char* location;
        const char* disaster;
        int population;
        const char* resource;
        int quantity;
        PriorityLevel priority;
    } samples[] = {
        {"R101", "Village A", "Pune",     "Flood",      500, "Water",          300, PRIORITY_CRITICAL},
        {"R102", "Village B", "Kolhapur", "Fire",       120, "Medicines",       80, PRIORITY_HIGH},
        {"R103", "Village C", "Satara",   "Landslide",  200, "Tents",          150, PRIORITY_LOW},
        {"R104", "Village D", "Sangli",   "Flood",      750, "Food",           400, PRIORITY_CRITICAL},
        {"R105", "Village E", "Nashik",   "Earthquake", 300, "Medicines",      600, PRIORITY_CRITICAL},
        {"R106", "Village F", "Pune",     "Cyclone",    180, "Blankets",       200, PRIORITY_MEDIUM},
        {"R107", "Village G", "Satara",   "Drought",    400, "Water",          500, PRIORITY_MEDIUM},
        {"R108", "Village H", "Sangli",   "Fire",        90, "Emergency Kits",  60, PRIORITY_HIGH},
        {"R109", "Village I", "Kolhapur", "Flood",      620, "Water",          350, PRIORITY_CRITICAL},
        {"R110", "Village J", "Nashik",   "Landslide",  150, "Clothes",        120, PRIORITY_LOW}
    };

    int sample_count = sizeof(samples) / sizeof(samples[0]);
    for (int i = 0; i < sample_count; i++) {
        ReliefRequest req;
        memset(&req, 0, sizeof(req));
        safe_copy(req.request_id, samples[i].id, sizeof(req.request_id));
        safe_copy(req.area, samples[i].area, sizeof(req.area));
        safe_copy(req.location, samples[i].location, sizeof(req.location));
        safe_copy(req.disaster_type, samples[i].disaster, sizeof(req.disaster_type));
        req.population = samples[i].population;
        safe_copy(req.resource, samples[i].resource, sizeof(req.resource));
        req.quantity = samples[i].quantity;
        req.allocated = 0;
        req.priority = samples[i].priority;
        req.status = STATUS_PENDING;
        get_current_time_str(req.request_time, sizeof(req.request_time));
        system_submit_request(sys, &req, true);
    }

    /* Move all sample requests into the Priority Queue initially */
    system_classify_all(sys, true);

    /* Pre-run 2 allocations so History & Completed records show initial activity */
    system_allocate_next(sys);
    system_allocate_next(sys);

    event_log_add(&sys->event_log, "RELIEF-Q initialized with sample data");
}

ReliefRequest* system_find_request(ReliefSystem* sys, const char* request_id) {
    if (!sys || !request_id) return NULL;
    for (int i = 0; i < sys->all_requests_count; i++) {
        if (strcmp(sys->all_requests[i].request_id, request_id) == 0) {
            return &sys->all_requests[i];
        }
    }
    return NULL;
}

bool system_submit_request(ReliefSystem* sys, const ReliefRequest* req, bool silent) {
    if (!sys || !req || sys->all_requests_count >= MAX_REQUESTS) return false;

    /* Verify unique request ID */
    if (system_find_request(sys, req->request_id) != NULL) {
        return false;
    }

    int idx = sys->all_requests_count++;
    sys->all_requests[idx] = *req;
    sys->all_requests[idx].arrival_order = sys->request_counter++;
    sys->all_requests[idx].allocated = 0;
    sys->all_requests[idx].status = STATUS_PENDING;
    if (strlen(sys->all_requests[idx].request_time) == 0) {
        get_current_time_str(sys->all_requests[idx].request_time, sizeof(sys->all_requests[idx].request_time));
    }

    queue_enqueue(&sys->waiting_queue, &sys->all_requests[idx]);

    if (!silent) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%s submitted and added to Queue", req->request_id);
        event_log_add(&sys->event_log, msg);
    }
    return true;
}

bool system_classify_next(ReliefSystem* sys, bool silent, ReliefRequest* out_req) {
    if (!sys) return false;
    ReliefRequest dequeued;
    if (!queue_dequeue(&sys->waiting_queue, &dequeued)) {
        return false;
    }

    /* Synchronize with master array record */
    ReliefRequest* master_req = system_find_request(sys, dequeued.request_id);
    if (master_req) {
        pq_insert(&sys->priority_queue, master_req);
        if (out_req) *out_req = *master_req;
    } else {
        pq_insert(&sys->priority_queue, &dequeued);
        if (out_req) *out_req = dequeued;
    }

    if (!silent) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%s classified as %s and moved to Priority Queue",
                 dequeued.request_id, priority_to_string(dequeued.priority));
        event_log_add(&sys->event_log, msg);
    }
    return true;
}

int system_classify_all(ReliefSystem* sys, bool silent) {
    if (!sys) return 0;
    int count = 0;
    ReliefRequest temp;
    while (system_classify_next(sys, silent, &temp)) {
        count++;
    }
    return count;
}

static void remove_from_waiting_resources(ReliefSystem* sys, const char* request_id) {
    for (int i = 0; i < sys->waiting_for_resources_count; i++) {
        if (strcmp(sys->waiting_for_resources_ids[i], request_id) == 0) {
            for (int j = i; j < sys->waiting_for_resources_count - 1; j++) {
                strcpy(sys->waiting_for_resources_ids[j], sys->waiting_for_resources_ids[j + 1]);
            }
            sys->waiting_for_resources_count--;
            break;
        }
    }
}

static void add_to_waiting_resources(ReliefSystem* sys, const char* request_id) {
    for (int i = 0; i < sys->waiting_for_resources_count; i++) {
        if (strcmp(sys->waiting_for_resources_ids[i], request_id) == 0) return;
    }
    if (sys->waiting_for_resources_count < MAX_REQUESTS) {
        safe_copy(sys->waiting_for_resources_ids[sys->waiting_for_resources_count++], request_id, MAX_ID_LEN);
    }
}

AllocationResult system_allocate_request(ReliefSystem* sys, ReliefRequest* req) {
    AllocationResult res;
    memset(&res, 0, sizeof(res));

    if (!sys || !req) {
        res.type = OUTCOME_EMPTY;
        return res;
    }

    int available = inventory_get_available(&sys->inventory, req->resource);
    int still_needed = req->quantity - req->allocated;

    if (available <= 0) {
        req->status = STATUS_WAITING_FOR_RESOURCES;
        add_to_waiting_resources(sys, req->request_id);

        char msg[256];
        snprintf(msg, sizeof(msg), "%s could not be allocated — 0 units of %s available. Marked \"Waiting for Resources\".",
                 req->request_id, req->resource);
        event_log_add(&sys->event_log, msg);

        res.type = OUTCOME_WAIT;
        res.req = *req;
        res.given = 0;
        sys->last_alloc_result = res;
        return res;
    }

    int given = inventory_consume(&sys->inventory, req->resource, still_needed);
    req->allocated += given;

    if (req->allocated >= req->quantity) {
        req->status = STATUS_COMPLETED;
    } else {
        req->status = STATUS_PARTIALLY_ALLOCATED;
    }

    remove_from_waiting_resources(sys, req->request_id);

    /* Store completed/distributed request in Linked List */
    if (ll_search(&sys->completed_list, req->request_id) == NULL) {
        ll_insert_end(&sys->completed_list, req);
    }

    /* Push allocation record onto Stack (LIFO) */
    sys->alloc_counter++;
    AllocationRecord record;
    memset(&record, 0, sizeof(record));
    snprintf(record.alloc_id, sizeof(record.alloc_id), "A%d", sys->alloc_counter);
    safe_copy(record.request_id, req->request_id, sizeof(record.request_id));
    safe_copy(record.resource, req->resource, sizeof(record.resource));
    record.quantity = given;
    safe_copy(record.location, req->location, sizeof(record.location));
    record.status = req->status;
    get_current_time_str(record.time_str, sizeof(record.time_str));

    stack_push(&sys->history_stack, &record);

    char msg[256];
    snprintf(msg, sizeof(msg), "%d unit(s) of %s allocated to %s (%s) — %s",
             given, req->resource, req->request_id, req->location, status_to_string(req->status));
    event_log_add(&sys->event_log, msg);

    snprintf(msg, sizeof(msg), "Allocation %s pushed onto Stack", record.alloc_id);
    event_log_add(&sys->event_log, msg);

    if (req->status == STATUS_COMPLETED) {
        snprintf(msg, sizeof(msg), "%s added to Linked List (Completed)", req->request_id);
        event_log_add(&sys->event_log, msg);
    }

    res.type = (req->status == STATUS_COMPLETED) ? OUTCOME_OK : OUTCOME_PARTIAL;
    res.req = *req;
    res.given = given;
    res.record = record;
    sys->last_alloc_result = res;
    return res;
}

AllocationResult system_allocate_next(ReliefSystem* sys) {
    AllocationResult res;
    memset(&res, 0, sizeof(res));

    if (!sys) {
        res.type = OUTCOME_EMPTY;
        return res;
    }

    ReliefRequest next_req;
    if (!pq_dequeue(&sys->priority_queue, &next_req)) {
        res.type = OUTCOME_EMPTY;
        sys->last_alloc_result = res;
        return res;
    }

    ReliefRequest* master_req = system_find_request(sys, next_req.request_id);
    if (!master_req) {
        master_req = &next_req;
    }

    return system_allocate_request(sys, master_req);
}

AllocationResult system_allocate_by_id(ReliefSystem* sys, const char* request_id) {
    AllocationResult res;
    memset(&res, 0, sizeof(res));

    if (!sys || !request_id) {
        res.type = OUTCOME_NOT_FOUND;
        return res;
    }

    ReliefRequest* req = system_find_request(sys, request_id);
    if (!req) {
        res.type = OUTCOME_NOT_FOUND;
        return res;
    }

    return system_allocate_request(sys, req);
}

bool system_undo_last(ReliefSystem* sys, AllocationRecord* out_record) {
    if (!sys) return false;

    AllocationRecord record;
    if (!stack_pop(&sys->history_stack, &record)) {
        return false;
    }

    ReliefRequest* req = system_find_request(sys, record.request_id);
    if (req) {
        inventory_restore(&sys->inventory, record.resource, record.quantity);
        req->allocated -= record.quantity;
        if (req->allocated < 0) req->allocated = 0;

        if (req->allocated <= 0) {
            req->allocated = 0;
            req->status = STATUS_PENDING;
            ll_delete(&sys->completed_list, req->request_id);
            pq_insert(&sys->priority_queue, req); /* Return to Priority Queue for re-allocation */
        } else {
            req->status = STATUS_PARTIALLY_ALLOCATED;
        }

        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Undo: allocation %s reversed — %d unit(s) of %s returned to inventory, %s is now \"%s\"",
                 record.alloc_id, record.quantity, record.resource, req->request_id, status_to_string(req->status));
        event_log_add(&sys->event_log, msg);
    }

    if (out_record) {
        *out_record = record;
    }
    return true;
}
