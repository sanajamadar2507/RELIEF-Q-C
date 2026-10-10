#include "relief_q.h"
#include "json_utils.h"

static char g_json_output[MAX_JSON_BUFFER];

/* Serialize a single ReliefRequest into JSON */
static void serialize_request(JsonBuffer* jb, const ReliefRequest* r) {
    json_buf_append(jb, "{\"requestID\":\"");
    json_buf_append_escaped(jb, r->request_id);
    json_buf_append(jb, "\",\"area\":\"");
    json_buf_append_escaped(jb, r->area);
    json_buf_append(jb, "\",\"location\":\"");
    json_buf_append_escaped(jb, r->location);
    json_buf_append(jb, "\",\"disasterType\":\"");
    json_buf_append_escaped(jb, r->disaster_type);
    json_buf_append(jb, "\",\"population\":");
    json_buf_append_int(jb, r->population);
    json_buf_append(jb, ",\"resource\":\"");
    json_buf_append_escaped(jb, r->resource);
    json_buf_append(jb, "\",\"quantity\":");
    json_buf_append_int(jb, r->quantity);
    json_buf_append(jb, ",\"allocated\":");
    json_buf_append_int(jb, r->allocated);
    json_buf_append(jb, ",\"priority\":\"");
    json_buf_append(jb, priority_to_string(r->priority));
    json_buf_append(jb, "\",\"status\":\"");
    json_buf_append(jb, status_to_string(r->status));
    json_buf_append(jb, "\",\"requestTime\":\"");
    json_buf_append_escaped(jb, r->request_time);
    json_buf_append(jb, "\",\"arrivalOrder\":");
    json_buf_append_int(jb, r->arrival_order);
    json_buf_append(jb, "}");
}

/* Serialize a single ResourceItem into JSON */
static void serialize_resource(JsonBuffer* jb, const ResourceItem* item) {
    json_buf_append(jb, "{\"name\":\"");
    json_buf_append_escaped(jb, item->name);
    json_buf_append(jb, "\",\"total\":");
    json_buf_append_int(jb, item->total);
    json_buf_append(jb, ",\"available\":");
    json_buf_append_int(jb, item->available);
    json_buf_append(jb, ",\"distributed\":");
    json_buf_append_int(jb, item->distributed);
    json_buf_append(jb, "}");
}

/* Serialize a single AllocationRecord into JSON */
static void serialize_allocation(JsonBuffer* jb, const AllocationRecord* rec) {
    json_buf_append(jb, "{\"allocID\":\"");
    json_buf_append_escaped(jb, rec->alloc_id);
    json_buf_append(jb, "\",\"requestID\":\"");
    json_buf_append_escaped(jb, rec->request_id);
    json_buf_append(jb, "\",\"resource\":\"");
    json_buf_append_escaped(jb, rec->resource);
    json_buf_append(jb, "\",\"quantity\":");
    json_buf_append_int(jb, rec->quantity);
    json_buf_append(jb, "\",\"location\":\"");
    json_buf_append_escaped(jb, rec->location);
    json_buf_append(jb, "\",\"status\":\"");
    json_buf_append(jb, status_to_string(rec->status));
    json_buf_append(jb, "\",\"time\":\"");
    json_buf_append_escaped(jb, rec->time_str);
    json_buf_append(jb, "\"}");
}

/* -------------------------------------------------------------------------
   EXPORTED C FUNCTIONS
   ------------------------------------------------------------------------- */

RQ_EXPORT const char* rq_init_system(void) {
    system_init(&g_system);
    system_load_sample_data(&g_system);
    return "{\"status\":\"ok\",\"message\":\"RELIEF-Q system initialized with sample data\"}";
}

RQ_EXPORT const char* rq_reset_system(void) {
    system_reset(&g_system);
    return "{\"status\":\"ok\",\"message\":\"RELIEF-Q system reset to clean state\"}";
}

RQ_EXPORT const char* rq_load_sample_data(void) {
    system_load_sample_data(&g_system);
    return "{\"status\":\"ok\",\"message\":\"Sample resources and requests reloaded\"}";
}

RQ_EXPORT const char* rq_submit_request(const char* id, const char* area, const char* location,
                                        const char* disaster, int population, const char* resource,
                                        int quantity, const char* priority) {
    if (!id || !area || !location || !disaster || !resource || !priority || population <= 0 || quantity <= 0) {
        return "{\"status\":\"error\",\"message\":\"Invalid input arguments\"}";
    }

    ReliefRequest req;
    memset(&req, 0, sizeof(req));
    strncpy(req.request_id, id, MAX_ID_LEN - 1);
    strncpy(req.area, area, MAX_NAME_LEN - 1);
    strncpy(req.location, location, MAX_NAME_LEN - 1);
    strncpy(req.disaster_type, disaster, MAX_NAME_LEN - 1);
    req.population = population;
    strncpy(req.resource, resource, MAX_NAME_LEN - 1);
    req.quantity = quantity;
    req.allocated = 0;
    req.priority = string_to_priority(priority);
    req.status = STATUS_PENDING;

    bool ok = system_submit_request(&g_system, &req, false);
    if (!ok) {
        return "{\"status\":\"error\",\"message\":\"Request ID already exists or storage full\"}";
    }
    return "{\"status\":\"ok\",\"message\":\"Request submitted to Queue successfully\"}";
}

RQ_EXPORT const char* rq_classify_next(void) {
    ReliefRequest classified;
    bool ok = system_classify_next(&g_system, false, &classified);
    if (!ok) {
        return "{\"status\":\"empty\",\"message\":\"Waiting queue is empty\"}";
    }

    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"status\":\"ok\",\"classified\":");
    serialize_request(&jb, &classified);
    json_buf_append(&jb, "}");
    return g_json_output;
}

RQ_EXPORT const char* rq_classify_all(void) {
    int count = system_classify_all(&g_system, false);
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"status\":\"ok\",\"classifiedCount\":");
    json_buf_append_int(&jb, count);
    json_buf_append(&jb, "}");
    return g_json_output;
}

RQ_EXPORT const char* rq_allocate_next(void) {
    AllocationResult res = system_allocate_next(&g_system);
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));

    if (res.type == OUTCOME_EMPTY) {
        json_buf_append(&jb, "{\"status\":\"empty\",\"type\":\"empty\",\"message\":\"Priority Queue is empty\"}");
    } else {
        json_buf_append(&jb, "{\"status\":\"ok\",\"type\":\"");
        if (res.type == OUTCOME_WAIT) {
            json_buf_append(&jb, "wait");
        } else if (res.type == OUTCOME_PARTIAL) {
            json_buf_append(&jb, "partial");
        } else {
            json_buf_append(&jb, "ok");
        }
        json_buf_append(&jb, "\",\"given\":");
        json_buf_append_int(&jb, res.given);
        json_buf_append(&jb, ",\"req\":");
        serialize_request(&jb, &res.req);
        if (res.type != OUTCOME_WAIT) {
            json_buf_append(&jb, ",\"record\":");
            serialize_allocation(&jb, &res.record);
        }
        json_buf_append(&jb, "}");
    }
    return g_json_output;
}

RQ_EXPORT const char* rq_allocate_request_by_id(const char* request_id) {
    AllocationResult res = system_allocate_by_id(&g_system, request_id);
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));

    if (res.type == OUTCOME_NOT_FOUND) {
        json_buf_append(&jb, "{\"status\":\"error\",\"message\":\"Request ID not found\"}");
    } else {
        json_buf_append(&jb, "{\"status\":\"ok\",\"type\":\"");
        if (res.type == OUTCOME_WAIT) {
            json_buf_append(&jb, "wait");
        } else if (res.type == OUTCOME_PARTIAL) {
            json_buf_append(&jb, "partial");
        } else {
            json_buf_append(&jb, "ok");
        }
        json_buf_append(&jb, "\",\"given\":");
        json_buf_append_int(&jb, res.given);
        json_buf_append(&jb, ",\"req\":");
        serialize_request(&jb, &res.req);
        if (res.type != OUTCOME_WAIT) {
            json_buf_append(&jb, ",\"record\":");
            serialize_allocation(&jb, &res.record);
        }
        json_buf_append(&jb, "}");
    }
    return g_json_output;
}

RQ_EXPORT const char* rq_undo_last(void) {
    AllocationRecord rec;
    bool ok = system_undo_last(&g_system, &rec);
    if (!ok) {
        return "{\"status\":\"empty\",\"message\":\"No allocations in stack to undo\"}";
    }

    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"status\":\"ok\",\"record\":");
    serialize_allocation(&jb, &rec);
    json_buf_append(&jb, "}");
    return g_json_output;
}

RQ_EXPORT const char* rq_add_resource(const char* name, int quantity) {
    if (!name || quantity <= 0) {
        return "{\"status\":\"error\",\"message\":\"Resource name cannot be empty and quantity must be positive\"}";
    }
    ResourceItem* item = inventory_add_resource(&g_system.inventory, name, quantity);
    if (!item) {
        return "{\"status\":\"error\",\"message\":\"Could not add resource; storage limit reached\"}";
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "%d unit(s) of %s added to inventory", quantity, name);
    event_log_add(&g_system.event_log, msg);

    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"status\":\"ok\",\"item\":");
    serialize_resource(&jb, item);
    json_buf_append(&jb, "}");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_dashboard_stats_json(void) {
    int total = g_system.all_requests_count;
    int pending = 0;
    int critical = 0;
    int completed = 0;

    for (int i = 0; i < total; i++) {
        if (g_system.all_requests[i].status == STATUS_PENDING) pending++;
        if (g_system.all_requests[i].priority == PRIORITY_CRITICAL) critical++;
        if (g_system.all_requests[i].status == STATUS_COMPLETED) completed++;
    }

    int res_avail = 0;
    int res_dist = 0;
    for (int i = 0; i < g_system.inventory.count; i++) {
        res_avail += g_system.inventory.items[i].available;
        res_dist += g_system.inventory.items[i].distributed;
    }

    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"totalRequests\":");
    json_buf_append_int(&jb, total);
    json_buf_append(&jb, ",\"pending\":");
    json_buf_append_int(&jb, pending);
    json_buf_append(&jb, ",\"critical\":");
    json_buf_append_int(&jb, critical);
    json_buf_append(&jb, ",\"completed\":");
    json_buf_append_int(&jb, completed);
    json_buf_append(&jb, ",\"resAvailable\":");
    json_buf_append_int(&jb, res_avail);
    json_buf_append(&jb, ",\"resDistributed\":");
    json_buf_append_int(&jb, res_dist);
    json_buf_append(&jb, "}");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_inventory_json(void) {
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "[");
    for (int i = 0; i < g_system.inventory.count; i++) {
        if (i > 0) json_buf_append(&jb, ",");
        serialize_resource(&jb, &g_system.inventory.items[i]);
    }
    json_buf_append(&jb, "]");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_waiting_queue_json(void) {
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"count\":");
    json_buf_append_int(&jb, g_system.waiting_queue.count);
    json_buf_append(&jb, ",\"items\":[");

    QNode* curr = g_system.waiting_queue.front;
    int idx = 0;
    while (curr != NULL) {
        if (idx > 0) json_buf_append(&jb, ",");
        serialize_request(&jb, &curr->data);
        curr = curr->next;
        idx++;
    }
    json_buf_append(&jb, "]}");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_priority_queue_json(void) {
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"total\":");
    json_buf_append_int(&jb, pq_size(&g_system.priority_queue));
    json_buf_append(&jb, ",\"levels\":{");

    const char* level_names[4] = {"Critical", "High", "Medium", "Low"};
    for (int l = 0; l < 4; l++) {
        if (l > 0) json_buf_append(&jb, ",");
        json_buf_append(&jb, "\"");
        json_buf_append(&jb, level_names[l]);
        json_buf_append(&jb, "\":[");

        QNode* curr = g_system.priority_queue.levels[l].front;
        int idx = 0;
        while (curr != NULL) {
            if (idx > 0) json_buf_append(&jb, ",");
            serialize_request(&jb, &curr->data);
            curr = curr->next;
            idx++;
        }
        json_buf_append(&jb, "]");
    }
    json_buf_append(&jb, "}}");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_all_requests_json(void) {
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "[");
    for (int i = 0; i < g_system.all_requests_count; i++) {
        if (i > 0) json_buf_append(&jb, ",");
        serialize_request(&jb, &g_system.all_requests[i]);
    }
    json_buf_append(&jb, "]");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_waiting_for_resources_json(void) {
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "[");
    int added = 0;
    for (int i = 0; i < g_system.waiting_for_resources_count; i++) {
        ReliefRequest* req = system_find_request(&g_system, g_system.waiting_for_resources_ids[i]);
        if (req) {
            if (added > 0) json_buf_append(&jb, ",");
            serialize_request(&jb, req);
            added++;
        }
    }
    json_buf_append(&jb, "]");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_history_json(void) {
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "[");
    /* Stack is LIFO: return from top down to 0 */
    int added = 0;
    for (int i = g_system.history_stack.top; i >= 0; i--) {
        if (added > 0) json_buf_append(&jb, ",");
        serialize_allocation(&jb, &g_system.history_stack.items[i]);
        added++;
    }
    json_buf_append(&jb, "]");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_completed_list_json(void) {
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "[");
    LLNode* curr = g_system.completed_list.head;
    int idx = 0;
    while (curr != NULL) {
        if (idx > 0) json_buf_append(&jb, ",");
        serialize_request(&jb, &curr->data);
        curr = curr->next;
        idx++;
    }
    json_buf_append(&jb, "]");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_live_feed_json(void) {
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "[");
    int count = g_system.event_log.count;
    int start = g_system.event_log.start;
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % MAX_LOG_EVENTS;
        if (i > 0) json_buf_append(&jb, ",");
        json_buf_append(&jb, "{\"time\":\"");
        json_buf_append_escaped(&jb, g_system.event_log.events[idx].time_str);
        json_buf_append(&jb, "\",\"text\":\"");
        json_buf_append_escaped(&jb, g_system.event_log.events[idx].text);
        json_buf_append(&jb, "\"}");
    }
    json_buf_append(&jb, "]");
    return g_json_output;
}

RQ_EXPORT const char* rq_search_requests_linear_json(const char* field, const char* keyword) {
    SearchResult res = linear_search_requests(g_system.all_requests, g_system.all_requests_count, field, keyword);
    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"scanned\":");
    json_buf_append_int(&jb, res.scanned);
    json_buf_append(&jb, ",\"count\":");
    json_buf_append_int(&jb, res.count);
    json_buf_append(&jb, ",\"results\":[");
    for (int i = 0; i < res.count; i++) {
        if (i > 0) json_buf_append(&jb, ",");
        serialize_request(&jb, &res.matches[i]);
    }
    json_buf_append(&jb, "]}");
    return g_json_output;
}

RQ_EXPORT const char* rq_search_requests_binary_json(const char* request_id) {
    /* Copy all requests and sort by requestID first */
    ReliefRequest sorted[MAX_REQUESTS];
    int count = g_system.all_requests_count;
    for (int i = 0; i < count; i++) {
        sorted[i] = g_system.all_requests[i];
    }
    bubble_sort_requests(sorted, count, "requestID", true);

    BinarySearchResult res = binary_search_by_id(sorted, count, request_id);

    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"found\":");
    json_buf_append_bool(&jb, res.found);
    json_buf_append(&jb, ",\"totalElements\":");
    json_buf_append_int(&jb, res.total_elements);
    json_buf_append(&jb, ",\"stepCount\":");
    json_buf_append_int(&jb, res.step_count);
    json_buf_append(&jb, ",\"steps\":[");
    for (int i = 0; i < res.step_count; i++) {
        if (i > 0) json_buf_append(&jb, ",");
        json_buf_append_int(&jb, res.steps[i]);
    }
    json_buf_append(&jb, "],\"match\":");
    if (res.found) {
        serialize_request(&jb, &res.match);
    } else {
        json_buf_append(&jb, "null");
    }
    json_buf_append(&jb, "}");
    return g_json_output;
}

RQ_EXPORT const char* rq_sort_requests_bubble_json(const char* field, int ascending) {
    ReliefRequest sorted[MAX_REQUESTS];
    int count = g_system.all_requests_count;
    for (int i = 0; i < count; i++) {
        sorted[i] = g_system.all_requests[i];
    }
    bubble_sort_requests(sorted, count, field, ascending != 0);

    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"field\":\"");
    json_buf_append_escaped(&jb, field ? field : "priority");
    json_buf_append(&jb, "\",\"ascending\":");
    json_buf_append_bool(&jb, ascending != 0);
    json_buf_append(&jb, ",\"sorted\":[");
    for (int i = 0; i < count; i++) {
        if (i > 0) json_buf_append(&jb, ",");
        serialize_request(&jb, &sorted[i]);
    }
    json_buf_append(&jb, "]}");
    return g_json_output;
}

RQ_EXPORT const char* rq_get_reports_json(void) {
    int total = g_system.all_requests_count;
    int completed = 0;
    int pending = 0;
    int critical = 0;

    for (int i = 0; i < total; i++) {
        if (g_system.all_requests[i].status == STATUS_COMPLETED) completed++;
        if (g_system.all_requests[i].status == STATUS_PENDING) pending++;
        if (g_system.all_requests[i].priority == PRIORITY_CRITICAL) critical++;
    }

    int res_dist = 0;
    for (int i = 0; i < g_system.inventory.count; i++) {
        res_dist += g_system.inventory.items[i].distributed;
    }

    /* Most requested resource */
    char most_requested[MAX_NAME_LEN] = "—";
    int max_req_count = 0;
    for (int i = 0; i < g_system.inventory.count; i++) {
        int r_count = 0;
        for (int j = 0; j < total; j++) {
            if (strcmp(g_system.all_requests[j].resource, g_system.inventory.items[i].name) == 0) {
                r_count++;
            }
        }
        if (r_count > max_req_count) {
            max_req_count = r_count;
            strncpy(most_requested, g_system.inventory.items[i].name, MAX_NAME_LEN - 1);
        }
    }

    /* Disaster type breakdown */
    const char* disaster_types[] = {"Flood", "Earthquake", "Cyclone", "Landslide", "Drought", "Fire"};
    int disaster_len = sizeof(disaster_types) / sizeof(disaster_types[0]);

    JsonBuffer jb;
    json_buf_init(&jb, g_json_output, sizeof(g_json_output));
    json_buf_append(&jb, "{\"totalDistributed\":");
    json_buf_append_int(&jb, res_dist);
    json_buf_append(&jb, ",\"mostRequested\":\"");
    json_buf_append_escaped(&jb, most_requested);
    json_buf_append(&jb, "\",\"criticalCount\":");
    json_buf_append_int(&jb, critical);
    json_buf_append(&jb, ",\"completedCount\":");
    json_buf_append_int(&jb, completed);
    json_buf_append(&jb, ",\"pendingCount\":");
    json_buf_append_int(&jb, pending);
    json_buf_append(&jb, ",\"totalLogged\":");
    json_buf_append_int(&jb, total);

    /* Priority breakdown */
    json_buf_append(&jb, ",\"priorities\":{");
    const char* p_names[4] = {"Critical", "High", "Medium", "Low"};
    for (int p = 0; p < 4; p++) {
        if (p > 0) json_buf_append(&jb, ",");
        int count = 0;
        for (int i = 0; i < total; i++) {
            if (g_system.all_requests[i].priority == (PriorityLevel)(p + 1)) count++;
        }
        json_buf_append(&jb, "\"");
        json_buf_append(&jb, p_names[p]);
        json_buf_append(&jb, "\":");
        json_buf_append_int(&jb, count);
    }
    json_buf_append(&jb, "}");

    /* Disaster breakdown */
    json_buf_append(&jb, ",\"disasters\":{");
    for (int d = 0; d < disaster_len; d++) {
        if (d > 0) json_buf_append(&jb, ",");
        int count = 0;
        for (int i = 0; i < total; i++) {
            if (strcmp(g_system.all_requests[i].disaster_type, disaster_types[d]) == 0) count++;
        }
        json_buf_append(&jb, "\"");
        json_buf_append(&jb, disaster_types[d]);
        json_buf_append(&jb, "\":");
        json_buf_append_int(&jb, count);
    }
    json_buf_append(&jb, "}}");

    return g_json_output;
}
