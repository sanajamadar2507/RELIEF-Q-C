#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../c_src/relief_q.h"
#include "../c_src/json_utils.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (!(cond)) { \
        printf("  [FAIL] %s (line %d): %s\n", __func__, __LINE__, msg); \
    } else { \
        g_tests_passed++; \
        printf("  [PASS] %s: %s\n", __func__, msg); \
    } \
} while(0)

/* Test 1: Resource Inventory Array */
void test_inventory(void) {
    printf("\n--- Test Suite 1: Resource Inventory (Array) ---\n");
    ResourceInventory inv;
    inventory_init(&inv);
    TEST_ASSERT(inv.count == 0, "Inventory initialized empty");

    ResourceItem* f = inventory_add_resource(&inv, "Food", 1000);
    TEST_ASSERT(f != NULL && f->total == 1000 && f->available == 1000 && f->distributed == 0, "Added Food 1000");

    ResourceItem* w = inventory_add_resource(&inv, "Water", 1200);
    TEST_ASSERT(w != NULL && inv.count == 2, "Added Water 1200, count is 2");

    /* Top up existing resource */
    ResourceItem* f2 = inventory_add_resource(&inv, "Food", 500);
    TEST_ASSERT(f2->total == 1500 && f2->available == 1500, "Top-up Food by 500 gives 1500 total");

    /* Consume available */
    int consumed = inventory_consume(&inv, "Water", 300);
    TEST_ASSERT(consumed == 300 && inventory_get_available(&inv, "Water") == 900, "Consumed 300 Water, 900 left");
    TEST_ASSERT(inv.items[inventory_find_index(&inv, "Water")].distributed == 300, "Distributed count updated to 300");

    /* Consume more than available */
    int over_consumed = inventory_consume(&inv, "Water", 1000);
    TEST_ASSERT(over_consumed == 900 && inventory_get_available(&inv, "Water") == 0, "Partial consume when requested > available");
    TEST_ASSERT(inv.items[inventory_find_index(&inv, "Water")].distributed == 1200, "All 1200 distributed");

    /* Restore on undo */
    inventory_restore(&inv, "Water", 400);
    TEST_ASSERT(inventory_get_available(&inv, "Water") == 400, "Restored 400 Water available");
    TEST_ASSERT(inv.items[inventory_find_index(&inv, "Water")].distributed == 800, "Distributed reduced to 800");
}

/* Test 2: FIFO Queue */
void test_queue(void) {
    printf("\n--- Test Suite 2: Queue (FIFO Linked List) ---\n");
    Queue q;
    queue_init(&q);
    TEST_ASSERT(queue_is_empty(&q), "Queue initialized empty");

    ReliefRequest r1 = {.population = 100, .priority = PRIORITY_HIGH};
    strcpy(r1.request_id, "REQ-1");
    ReliefRequest r2 = {.population = 200, .priority = PRIORITY_CRITICAL};
    strcpy(r2.request_id, "REQ-2");
    ReliefRequest r3 = {.population = 300, .priority = PRIORITY_LOW};
    strcpy(r3.request_id, "REQ-3");

    queue_enqueue(&q, &r1);
    queue_enqueue(&q, &r2);
    queue_enqueue(&q, &r3);
    TEST_ASSERT(q.count == 3, "Queue has 3 items");

    ReliefRequest peek_r;
    TEST_ASSERT(queue_peek(&q, &peek_r) && strcmp(peek_r.request_id, "REQ-1") == 0, "Peek returns REQ-1 at front");

    ReliefRequest out;
    TEST_ASSERT(queue_dequeue(&q, &out) && strcmp(out.request_id, "REQ-1") == 0, "1st dequeue is REQ-1 (FIFO)");
    TEST_ASSERT(queue_dequeue(&q, &out) && strcmp(out.request_id, "REQ-2") == 0, "2nd dequeue is REQ-2 (FIFO)");
    TEST_ASSERT(queue_dequeue(&q, &out) && strcmp(out.request_id, "REQ-3") == 0, "3rd dequeue is REQ-3 (FIFO)");
    TEST_ASSERT(queue_is_empty(&q), "Queue is empty after 3 dequeues");

    queue_free(&q);
}

/* Test 3: Priority Queue */
void test_priority_queue(void) {
    printf("\n--- Test Suite 3: Priority Queue (4 Level Queues) ---\n");
    PriorityQueue pq;
    pq_init(&pq);
    TEST_ASSERT(pq_is_empty(&pq), "Priority queue initialized empty");

    ReliefRequest r_low = {.priority = PRIORITY_LOW};
    strcpy(r_low.request_id, "R_LOW");
    ReliefRequest r_med = {.priority = PRIORITY_MEDIUM};
    strcpy(r_med.request_id, "R_MED");
    ReliefRequest r_high = {.priority = PRIORITY_HIGH};
    strcpy(r_high.request_id, "R_HIGH");
    ReliefRequest r_crit1 = {.priority = PRIORITY_CRITICAL};
    strcpy(r_crit1.request_id, "R_CRIT1");
    ReliefRequest r_crit2 = {.priority = PRIORITY_CRITICAL};
    strcpy(r_crit2.request_id, "R_CRIT2");

    /* Insert in arbitrary order */
    pq_insert(&pq, &r_low);
    pq_insert(&pq, &r_crit1);
    pq_insert(&pq, &r_med);
    pq_insert(&pq, &r_high);
    pq_insert(&pq, &r_crit2);

    TEST_ASSERT(pq_size(&pq) == 5, "PQ has 5 total items");
    TEST_ASSERT(pq_level_size(&pq, PRIORITY_CRITICAL) == 2, "Critical level has 2 items");

    ReliefRequest out;
    /* Dequeue 1: First Critical */
    TEST_ASSERT(pq_dequeue(&pq, &out) && strcmp(out.request_id, "R_CRIT1") == 0, "Dequeue 1 is R_CRIT1");
    /* Dequeue 2: Second Critical (FIFO tie-break) */
    TEST_ASSERT(pq_dequeue(&pq, &out) && strcmp(out.request_id, "R_CRIT2") == 0, "Dequeue 2 is R_CRIT2");
    /* Dequeue 3: High */
    TEST_ASSERT(pq_dequeue(&pq, &out) && strcmp(out.request_id, "R_HIGH") == 0, "Dequeue 3 is R_HIGH");
    /* Dequeue 4: Medium */
    TEST_ASSERT(pq_dequeue(&pq, &out) && strcmp(out.request_id, "R_MED") == 0, "Dequeue 4 is R_MED");
    /* Dequeue 5: Low */
    TEST_ASSERT(pq_dequeue(&pq, &out) && strcmp(out.request_id, "R_LOW") == 0, "Dequeue 5 is R_LOW");
    TEST_ASSERT(pq_is_empty(&pq), "PQ empty after all levels drained");

    pq_free(&pq);
}

/* Test 4: Linked List */
void test_linked_list(void) {
    printf("\n--- Test Suite 4: Linked List (Completed Requests) ---\n");
    LinkedList ll;
    ll_init(&ll);
    TEST_ASSERT(ll.size == 0 && ll.head == NULL, "Linked list initialized empty");

    ReliefRequest r1, r2, r3;
    memset(&r1, 0, sizeof(r1)); strcpy(r1.request_id, "C101");
    memset(&r2, 0, sizeof(r2)); strcpy(r2.request_id, "C102");
    memset(&r3, 0, sizeof(r3)); strcpy(r3.request_id, "C103");

    ll_insert_end(&ll, &r1);
    ll_insert_end(&ll, &r2);
    ll_insert_end(&ll, &r3);
    TEST_ASSERT(ll.size == 3, "Linked list size is 3");

    ReliefRequest* found = ll_search(&ll, "C102");
    TEST_ASSERT(found != NULL && strcmp(found->request_id, "C102") == 0, "Found C102 in linked list");
    TEST_ASSERT(ll_search(&ll, "C999") == NULL, "Searching non-existent ID returns NULL");

    /* Delete middle */
    bool del_ok = ll_delete(&ll, "C102");
    TEST_ASSERT(del_ok && ll.size == 2 && ll_search(&ll, "C102") == NULL, "Deleted C102 from linked list");

    /* Delete head */
    del_ok = ll_delete(&ll, "C101");
    TEST_ASSERT(del_ok && ll.size == 1 && strcmp(ll.head->data.request_id, "C103") == 0, "Deleted head C101");

    ll_free(&ll);
    TEST_ASSERT(ll.size == 0 && ll.head == NULL, "Linked list freed successfully");
}

/* Test 5: Stack (LIFO) */
void test_stack(void) {
    printf("\n--- Test Suite 5: Stack (Allocation History & Undo) ---\n");
    Stack s;
    stack_init(&s);
    TEST_ASSERT(stack_is_empty(&s), "Stack initialized empty");

    AllocationRecord a1, a2, a3;
    strcpy(a1.alloc_id, "A101"); a1.quantity = 100;
    strcpy(a2.alloc_id, "A102"); a2.quantity = 200;
    strcpy(a3.alloc_id, "A103"); a3.quantity = 300;

    stack_push(&s, &a1);
    stack_push(&s, &a2);
    stack_push(&s, &a3);
    TEST_ASSERT(s.top == 2, "Stack top is 2 after 3 pushes");

    AllocationRecord top_rec;
    TEST_ASSERT(stack_peek(&s, &top_rec) && strcmp(top_rec.alloc_id, "A103") == 0, "Peek returns top A103");

    AllocationRecord out;
    TEST_ASSERT(stack_pop(&s, &out) && strcmp(out.alloc_id, "A103") == 0, "1st pop is A103 (LIFO)");
    TEST_ASSERT(stack_pop(&s, &out) && strcmp(out.alloc_id, "A102") == 0, "2nd pop is A102 (LIFO)");
    TEST_ASSERT(stack_pop(&s, &out) && strcmp(out.alloc_id, "A101") == 0, "3rd pop is A101 (LIFO)");
    TEST_ASSERT(stack_is_empty(&s), "Stack empty after 3 pops");
}

/* Test 6: Searching and Sorting Algorithms */
void test_search_and_sort(void) {
    printf("\n--- Test Suite 6: Linear Search, Binary Search & Bubble Sort ---\n");
    ReliefRequest arr[5];
    memset(arr, 0, sizeof(arr));

    strcpy(arr[0].request_id, "R105"); arr[0].population = 300; arr[0].quantity = 600; arr[0].priority = PRIORITY_CRITICAL; strcpy(arr[0].location, "Nashik");
    strcpy(arr[1].request_id, "R102"); arr[1].population = 120; arr[1].quantity = 80;  arr[1].priority = PRIORITY_HIGH;     strcpy(arr[1].location, "Kolhapur");
    strcpy(arr[2].request_id, "R101"); arr[2].population = 500; arr[2].quantity = 300; arr[2].priority = PRIORITY_CRITICAL; strcpy(arr[2].location, "Pune");
    strcpy(arr[3].request_id, "R103"); arr[3].population = 200; arr[3].quantity = 150; arr[3].priority = PRIORITY_LOW;      strcpy(arr[3].location, "Satara");
    strcpy(arr[4].request_id, "R104"); arr[4].population = 750; arr[4].quantity = 400; arr[4].priority = PRIORITY_CRITICAL; strcpy(arr[4].location, "Sangli");

    /* Linear Search */
    SearchResult sr = linear_search_requests(arr, 5, "location", "Pune");
    TEST_ASSERT(sr.count == 1 && strcmp(sr.matches[0].request_id, "R101") == 0, "Linear search found Pune at R101");

    sr = linear_search_requests(arr, 5, "priority", "Critical");
    TEST_ASSERT(sr.count == 3, "Linear search found 3 Critical requests");

    /* Bubble Sort by requestID (ascending) */
    bubble_sort_requests(arr, 5, "requestID", true);
    TEST_ASSERT(strcmp(arr[0].request_id, "R101") == 0 &&
                strcmp(arr[1].request_id, "R102") == 0 &&
                strcmp(arr[2].request_id, "R103") == 0 &&
                strcmp(arr[3].request_id, "R104") == 0 &&
                strcmp(arr[4].request_id, "R105") == 0,
                "Bubble sort by requestID sorted in ascending alphabetical order");

    /* Binary Search on sorted array */
    BinarySearchResult bsr = binary_search_by_id(arr, 5, "R104");
    TEST_ASSERT(bsr.found && strcmp(bsr.match.request_id, "R104") == 0, "Binary search found R104");
    TEST_ASSERT(bsr.step_count > 0 && bsr.step_count <= 3, "Binary search completed in <= 3 comparisons (O(log n))");

    bsr = binary_search_by_id(arr, 5, "R999");
    TEST_ASSERT(!bsr.found, "Binary search correctly reports R999 not found");

    /* Bubble Sort by priority */
    bubble_sort_requests(arr, 5, "priority", true);
    TEST_ASSERT(arr[0].priority == PRIORITY_CRITICAL &&
                arr[1].priority == PRIORITY_CRITICAL &&
                arr[2].priority == PRIORITY_CRITICAL &&
                arr[3].priority == PRIORITY_HIGH &&
                arr[4].priority == PRIORITY_LOW,
                "Bubble sort by priority placed Critical -> High -> Low");
}

/* Test 7: Full Simulator Workflow & Sample Data */
void test_simulator_workflow(void) {
    printf("\n--- Test Suite 7: Full Simulation Workflow & Sample Data ---\n");
    ReliefSystem sys;
    system_init(&sys);
    system_load_sample_data(&sys);

    /* Verify 7 starting resources */
    TEST_ASSERT(sys.inventory.count == 7, "Sample inventory loaded 7 resources");
    TEST_ASSERT(sys.inventory.items[inventory_find_index(&sys.inventory, "Food")].total == 1000, "Total Food is 1000");
    TEST_ASSERT(inventory_get_available(&sys.inventory, "Food") == 600, "Available Food is 600 (400 allocated in pre-run)");
    TEST_ASSERT(inventory_get_available(&sys.inventory, "Water") < 1200, "Water allocated in initial pre-run");

    /* Verify 10 sample requests */
    TEST_ASSERT(sys.all_requests_count == 10, "10 sample requests loaded");

    /* Pre-run performed 2 allocations: R101 (Water 300) and R104 (Food 400) */
    TEST_ASSERT(sys.history_stack.top == 1, "Stack contains 2 allocation records");
    TEST_ASSERT(sys.completed_list.size == 2, "Completed list contains 2 requests");

    /* Allocate next request from Priority Queue: should be R105 or R109 */
    AllocationResult res = system_allocate_next(&sys);
    TEST_ASSERT(res.type == OUTCOME_OK || res.type == OUTCOME_PARTIAL, "Allocated next highest priority request");
    TEST_ASSERT(sys.history_stack.top == 2, "Stack grew to 3 records");

    /* Test Undo */
    AllocationRecord undone;
    bool undo_ok = system_undo_last(&sys, &undone);
    TEST_ASSERT(undo_ok && sys.history_stack.top == 1, "Undo restored last allocation, stack count decreased to 2");

    system_reset(&sys);
}

int main(void) {
    printf("=====================================================\n");
    printf("  RELIEF-Q — Comprehensive C Verification Test Suite \n");
    printf("=====================================================\n");

    test_inventory();
    test_queue();
    test_priority_queue();
    test_linked_list();
    test_stack();
    test_search_and_sort();
    test_simulator_workflow();

    printf("\n=====================================================\n");
    printf("  TEST RESULTS: %d / %d PASSED\n", g_tests_passed, g_tests_run);
    printf("=====================================================\n");

    if (g_tests_passed == g_tests_run) {
        printf("ALL C TESTS PASSED WITH 100%% SUCCESS!\n");
        return 0;
    } else {
        printf("SOME C TESTS FAILED!\n");
        return 1;
    }
}
