/* =========================================================================
   RELIEF-Q  —  dataStructures.js
   -------------------------------------------------------------------------
   This file contains MANUAL implementations of every Fundamental Data
   Structure used in the project. Nothing here relies on JavaScript's
   built-in Map, Set, or any library. Every structure is built node-by-node
   or index-by-index so it can be explained line-by-line in a viva.

   Structures implemented in this file:
     1. ReliefRequest        -> Structure / Class
     2. ResourceArray        -> Array
     3. LLNode + LinkedList  -> Singly Linked List
     4. Queue                -> Linked-list based Queue (FIFO)
     5. PriorityQueue        -> 4 internal Queues, one per priority level
     6. Stack                -> Array based Stack (LIFO)
     7. LinearSearch / BinarySearch -> Searching algorithms
     8. bubbleSortRequests   -> Sorting algorithm
   ========================================================================= */


/* -------------------------------------------------------------------------
   1. STRUCTURE  ->  ReliefRequest
   -------------------------------------------------------------------------
   In C++ this would be a `struct` or `class`. In JavaScript we use a
   `class` to group all the related fields of ONE relief request together,
   instead of keeping them in separate loose variables/arrays.
   ------------------------------------------------------------------------- */
class ReliefRequest {
  constructor(requestID, area, location, disasterType, population, resource, quantity, priority) {
    this.requestID     = requestID;
    this.area          = area;
    this.location      = location;
    this.disasterType  = disasterType;
    this.population    = Number(population);
    this.resource      = resource;
    this.quantity      = Number(quantity);       // quantity required
    this.allocated     = 0;                       // quantity actually given
    this.priority      = priority;                // "Critical" | "High" | "Medium" | "Low"
    this.status        = "Pending";                // Pending | Waiting for Resources | Partially Allocated | Completed
    this.requestTime   = new Date();
    this.arrivalOrder  = ReliefRequest.counter++;  // used to break priority ties (FIFO)
  }
}
ReliefRequest.counter = 0; // global increasing counter -> arrival order


/* -------------------------------------------------------------------------
   2. ARRAY  ->  ResourceArray
   -------------------------------------------------------------------------
   Resource inventory (Food, Water, Medicines, ...) is stored as a plain
   JavaScript array of small objects. This demonstrates direct array
   traversal for add / update / search / display operations.
   ------------------------------------------------------------------------- */
class ResourceArray {
  constructor() {
    this.items = []; // each item: { name, total, available, distributed }
  }

  addResource(name, quantity) {
    // Linear search through the array to see if the resource already exists
    for (let i = 0; i < this.items.length; i++) {
      if (this.items[i].name.toLowerCase() === name.toLowerCase()) {
        this.items[i].total += quantity;
        this.items[i].available += quantity;
        return this.items[i];
      }
    }
    const newItem = { name, total: quantity, available: quantity, distributed: 0 };
    this.items.push(newItem);
    return newItem;
  }

  // Linear search by name -> returns the array index, or -1
  findIndex(name) {
    for (let i = 0; i < this.items.length; i++) {
      if (this.items[i].name.toLowerCase() === name.toLowerCase()) return i;
    }
    return -1;
  }

  getAvailable(name) {
    const idx = this.findIndex(name);
    return idx === -1 ? 0 : this.items[idx].available;
  }

  // Reduce available quantity when resources are allocated
  consume(name, quantity) {
    const idx = this.findIndex(name);
    if (idx === -1) return 0;
    const given = Math.min(this.items[idx].available, quantity);
    this.items[idx].available -= given;
    this.items[idx].distributed += given;
    return given;
  }

  // Used by Undo -> gives resources back to the inventory
  restore(name, quantity) {
    const idx = this.findIndex(name);
    if (idx === -1) return;
    this.items[idx].available += quantity;
    this.items[idx].distributed -= quantity;
  }

  getAll() {
    return this.items;
  }
}


/* -------------------------------------------------------------------------
   3. LINKED LIST  ->  LLNode + LinkedList
   -------------------------------------------------------------------------
   Used to store COMPLETED / DISTRIBUTED relief requests.
   Implemented manually with a `next` pointer on each node, exactly like a
   singly linked list would be implemented in C++ with pointers.
   ------------------------------------------------------------------------- */
class LLNode {
  constructor(data) {
    this.data = data;   // holds a ReliefRequest object
    this.next = null;
  }
}

class LinkedList {
  constructor() {
    this.head = null;
    this.size = 0;
  }

  // Insert at the end -> O(n) traversal (simple & easy to explain)
  insert(request) {
    const node = new LLNode(request);
    if (!this.head) {
      this.head = node;
    } else {
      let temp = this.head;
      while (temp.next !== null) temp = temp.next;
      temp.next = node;
    }
    this.size++;
  }

  // Linear search through the linked list by requestID
  search(requestID) {
    let temp = this.head;
    while (temp !== null) {
      if (temp.data.requestID === requestID) return temp.data;
      temp = temp.next;
    }
    return null;
  }

  // Delete a node by requestID
  delete(requestID) {
    if (!this.head) return false;
    if (this.head.data.requestID === requestID) {
      this.head = this.head.next;
      this.size--;
      return true;
    }
    let prev = this.head;
    let temp = this.head.next;
    while (temp !== null) {
      if (temp.data.requestID === requestID) {
        prev.next = temp.next;
        this.size--;
        return true;
      }
      prev = temp;
      temp = temp.next;
    }
    return false;
  }

  // Convert linked list -> plain array, only for displaying in the UI
  toArray() {
    const result = [];
    let temp = this.head;
    while (temp !== null) {
      result.push(temp.data);
      temp = temp.next;
    }
    return result;
  }
}


/* -------------------------------------------------------------------------
   4. QUEUE  ->  Queue  (FIFO)
   -------------------------------------------------------------------------
   Holds newly-submitted requests that are waiting to be classified into
   the Priority Queue. Implemented with linked nodes and head/tail
   pointers so enqueue() and dequeue() are both O(1).
   ------------------------------------------------------------------------- */
class QNode {
  constructor(data) {
    this.data = data;
    this.next = null;
  }
}

class Queue {
  constructor() {
    this.front = null; // pointer to the front node (next to leave)
    this.rear  = null;  // pointer to the rear node (last to enter)
    this.count = 0;
  }

  enqueue(request) {
    const node = new QNode(request);
    if (this.rear === null) {
      this.front = this.rear = node;
    } else {
      this.rear.next = node;
      this.rear = node;
    }
    this.count++;
  }

  dequeue() {
    if (this.front === null) return null;
    const node = this.front;
    this.front = this.front.next;
    if (this.front === null) this.rear = null;
    this.count--;
    return node.data;
  }

  peek() {
    return this.front === null ? null : this.front.data;
  }

  isEmpty() {
    return this.count === 0;
  }

  toArray() {
    const result = [];
    let temp = this.front;
    while (temp !== null) {
      result.push(temp.data);
      temp = temp.next;
    }
    return result;
  }
}


/* -------------------------------------------------------------------------
   5. PRIORITY QUEUE  ->  PriorityQueue
   -------------------------------------------------------------------------
   Implemented manually using FOUR ordinary Queues internally — one queue
   per priority level (Critical, High, Medium, Low).

   Why this design?
     - It is easy for a first-year student to explain: "each priority
       level has its own line, and we always serve the highest line
       first."
     - Within the SAME priority level, requests are still served in the
       order they arrived (FIFO), because each level is a real Queue.
     - dequeue() therefore always removes the highest-priority, oldest
       request first — exactly the behaviour of a priority queue.
   ------------------------------------------------------------------------- */
class PriorityQueue {
  constructor() {
    this.levels = {
      Critical: new Queue(),
      High:     new Queue(),
      Medium:   new Queue(),
      Low:      new Queue()
    };
    this.order = ["Critical", "High", "Medium", "Low"];
  }

  insert(request) {
    this.levels[request.priority].enqueue(request);
  }

  // Removes and returns the highest priority, earliest-arrived request
  dequeue() {
    for (const level of this.order) {
      if (!this.levels[level].isEmpty()) {
        return this.levels[level].dequeue();
      }
    }
    return null;
  }

  peek() {
    for (const level of this.order) {
      if (!this.levels[level].isEmpty()) {
        return this.levels[level].peek();
      }
    }
    return null;
  }

  isEmpty() {
    return this.order.every(level => this.levels[level].isEmpty());
  }

  // For displaying the whole priority queue grouped by level in the UI
  getGrouped() {
    const grouped = {};
    for (const level of this.order) {
      grouped[level] = this.levels[level].toArray();
    }
    return grouped;
  }

  size() {
    return this.order.reduce((sum, level) => sum + this.levels[level].count, 0);
  }
}


/* -------------------------------------------------------------------------
   6. STACK  ->  Stack  (LIFO)
   -------------------------------------------------------------------------
   Used for the Allocation History / Undo feature. Implemented on top of a
   plain JavaScript array, but only using push()/pop() from one end, which
   is exactly how a stack is defined.
   ------------------------------------------------------------------------- */
class Stack {
  constructor() {
    this.items = [];
  }

  push(item) {
    this.items.push(item);
  }

  pop() {
    if (this.isEmpty()) return null;
    return this.items.pop();
  }

  peek() {
    if (this.isEmpty()) return null;
    return this.items[this.items.length - 1];
  }

  isEmpty() {
    return this.items.length === 0;
  }

  // Returns history with the MOST RECENT allocation first (top of stack first)
  toArray() {
    return [...this.items].reverse();
  }
}


/* -------------------------------------------------------------------------
   7. SEARCHING ALGORITHMS
   ------------------------------------------------------------------------- */

// Linear Search: checks every element one by one -> O(n)
function linearSearchRequests(requestsArray, field, keyword) {
  const results = [];
  const key = keyword.toLowerCase();
  for (let i = 0; i < requestsArray.length; i++) {
    const value = String(requestsArray[i][field] ?? "").toLowerCase();
    if (value.includes(key)) {
      results.push(requestsArray[i]);
    }
  }
  return results;
}

// Binary Search: works only on a SORTED array, by requestID -> O(log n)
// Demonstrated separately since it requires the array to be pre-sorted.
function binarySearchByID(sortedArray, requestID) {
  let low = 0;
  let high = sortedArray.length - 1;
  const steps = [];

  while (low <= high) {
    const mid = Math.floor((low + high) / 2);
    steps.push(mid);
    if (sortedArray[mid].requestID === requestID) {
      return { result: sortedArray[mid], steps };
    } else if (sortedArray[mid].requestID < requestID) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }
  return { result: null, steps };
}


/* -------------------------------------------------------------------------
   8. SORTING ALGORITHM  ->  Bubble Sort
   -------------------------------------------------------------------------
   Repeatedly compares adjacent elements and swaps them if they are in the
   wrong order. O(n^2) time complexity — simple and easy to explain.
   ------------------------------------------------------------------------- */
const PRIORITY_RANK = { Critical: 1, High: 2, Medium: 3, Low: 4 };

function bubbleSortRequests(requestsArray, field, ascending = true) {
  const arr = [...requestsArray]; // copy so the original order is preserved elsewhere
  const n = arr.length;

  for (let i = 0; i < n - 1; i++) {
    for (let j = 0; j < n - i - 1; j++) {
      let a, b;

      if (field === "priority") {
        a = PRIORITY_RANK[arr[j].priority];
        b = PRIORITY_RANK[arr[j + 1].priority];
      } else if (field === "requestID") {
        a = arr[j].requestID;
        b = arr[j + 1].requestID;
      } else {
        a = arr[j][field];
        b = arr[j + 1][field];
      }

      const shouldSwap = ascending ? a > b : a < b;
      if (shouldSwap) {
        const temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
  return arr;
}
