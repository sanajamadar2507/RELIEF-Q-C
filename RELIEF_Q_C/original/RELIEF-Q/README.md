# RELIEF-Q
### Priority-Based Disaster Relief Resource Allocation and Distribution Simulator Using Fundamental Data Structures

A first-year CSE mini-project built with **HTML, CSS, and JavaScript**, demonstrating Arrays, Structures, Linked Lists, Queues, Priority Queues, Stacks, Searching, and Sorting — all implemented manually, without relying on built-in `Map`, `Set`, or external libraries.

---

## 1. Introduction

Natural disasters (floods, earthquakes, fires, cyclones, droughts, landslides) create sudden demand for limited relief resources such as food, water, medicine, blankets, and tents. Relief teams must decide **who gets help first** when supplies cannot cover every request at once.

RELIEF-Q simulates this situation. Affected areas submit relief requests, the system classifies them by urgency, and resources are distributed in priority order — Critical areas first, then High, Medium, and finally Low — while keeping a full history of what was given to whom.

## 2. Problem Statement

During a disaster, relief coordinators receive many requests at the same time, from many different locations, each with different urgency and resource needs. Manually deciding the distribution order is slow and error-prone, and it becomes very difficult to track how much of each resource is left, or which requests have already been served. RELIEF-Q solves this by automating the process using well-defined data structures.

## 3. Objectives

- Simulate a realistic disaster-relief request-and-allocation workflow.
- Demonstrate the practical use of Arrays, Structures, Linked Lists, Queues, Priority Queues, Stacks, Searching, and Sorting.
- Allocate limited resources fairly, based on priority and arrival order.
- Support partial allocation when resources are insufficient.
- Provide a simple undo mechanism using a Stack.
- Present all of this through a clean, understandable dashboard.

## 4. Scope

The project is a **local, browser-based simulation** — it does not connect to any real emergency services, database, or cloud API. It is meant purely to demonstrate Fundamental Data Structures (FDS) concepts through a believable, relatable real-world scenario, at a level appropriate for a first-year CSE student.

## 5. Features

- Submit relief requests through a validated form.
- Automatic waiting Queue (FIFO) for newly submitted requests.
- Manual Priority Queue that always serves Critical → High → Medium → Low, and FIFO within the same level.
- Resource inventory management (add/top-up resources, track available vs. distributed).
- Resource allocation engine with **full**, **partial**, and **waiting-for-resources** outcomes.
- Allocation History implemented as a Stack, with an **Undo Last Allocation** button (LIFO).
- Completed/distributed requests stored in a Linked List.
- Linear Search (any field) and Binary Search (by Request ID, on a sorted copy).
- Bubble Sort by priority, population, quantity, or ID — with before/after comparison.
- Dashboard with live statistics, priority distribution, inventory bars, and a live operations feed.
- Reports page with aggregate summaries.
- 10 pre-loaded sample requests across 5 sample locations (Pune, Kolhapur, Sangli, Satara, Nashik).

## 6. Technologies Used

| Layer      | Technology                     |
|------------|---------------------------------|
| Structure  | HTML5                          |
| Styling    | CSS3 (custom, no framework)    |
| Logic      | Vanilla JavaScript (ES6 classes) |
| Storage    | In-memory (browser session only) |

No React/Angular/Vue, no Firebase/MongoDB, no external data-structure libraries. Everything runs locally by opening a single HTML file.

---

## 7. Data Structures Used — and Where

| # | Data Structure | Purpose in RELIEF-Q | File / Location |
|---|-----------------|----------------------|------------------|
| 1 | **Array** (`ResourceArray`) | Stores the resource inventory (Food, Water, Medicines…) | `dataStructures.js`, Inventory page |
| 2 | **Structure/Class** (`ReliefRequest`) | Groups all fields of one relief request together | `dataStructures.js`, every request row |
| 3 | **Linked List** (`LinkedList`) | Stores completed/distributed requests | `dataStructures.js`, Reports page |
| 4 | **Queue** (`Queue`) | Holds new requests waiting for classification | `dataStructures.js`, Relief Requests page |
| 5 | **Priority Queue** (`PriorityQueue`) | Orders requests for allocation by priority + arrival time | `dataStructures.js`, Priority Queue page |
| 6 | **Stack** (`Stack`) | Allocation history / undo | `dataStructures.js`, Allocation History page |
| 7 | **Linear & Binary Search** | Finding a request by field | `dataStructures.js`, Search page |
| 8 | **Bubble Sort** | Ordering requests by priority/population/quantity/ID | `dataStructures.js`, Sort page |

### How the Priority Queue is implemented

The Priority Queue is built from **four ordinary Queues**, one per priority level (`Critical`, `High`, `Medium`, `Low`). This design was chosen because it is easy to explain:

- Each priority level has its own "lane," implemented as a real FIFO Queue.
- `insert()` simply enqueues the request into the lane matching its priority.
- `dequeue()` always checks the lanes in order — Critical first, then High, then Medium, then Low — and removes the front request of the first non-empty lane.
- Because each lane is a Queue, two requests of the **same** priority always come out in the order they arrived (FIFO), satisfying the tie-breaking rule.

---

## 8. System Workflow

```
User submits a Relief Request
        │
        ▼
 Request enters the Queue   ──────────►  "Added to Queue"
        │
        ▼
Request is classified into
   the Priority Queue        ──────────►  "Moved to Priority Queue"
        │
        ▼
Highest-priority request
     is selected
        │
        ▼
 Resource availability
      is checked
        │
   ┌────┴─────┐
   ▼          ▼
Sufficient   Insufficient / Partial
   │              │
   ▼              ▼
Allocate       Allocate what is possible
resources      OR mark "Waiting for Resources"
   │              │
   ▼              │
Inventory updated ◄┘
   │
   ▼
Allocation pushed onto the Stack   ──────────►  "Allocation added to Stack"
   │
   ▼
Completed request added to
    the Linked List                ──────────►  "Added to Linked List"
   │
   ▼
Dashboard & Reports updated
```

---

## 9. Algorithms

For each algorithm: **Purpose · Input · Output · Steps · Data Structure Used · Time Complexity**

### 9.1 Add Relief Request
- **Purpose:** Record a new relief request in the system.
- **Input:** requestID, area, location, disasterType, population, resource, quantity, priority.
- **Output:** A new `ReliefRequest` object added to the master list and the Queue.
- **Steps:**
  1. Validate all input fields.
  2. Create a new `ReliefRequest` structure with the given values and status `"Pending"`.
  3. Push it into the master array of all requests.
  4. Enqueue it into the waiting Queue.
- **Data Structure:** Structure (record) + Array + Queue.
- **Time Complexity:** O(1).

### 9.2 Enqueue Request (into waiting Queue)
- **Purpose:** Place a new request at the back of the FIFO waiting line.
- **Input:** A `ReliefRequest` object.
- **Output:** Request added to the rear of the Queue.
- **Steps:** Create a queue node → if queue empty, front = rear = node; else old rear.next = node, rear = node.
- **Data Structure:** Linked-list based Queue.
- **Time Complexity:** O(1).

### 9.3 Insert into Priority Queue
- **Purpose:** Move a request from the waiting Queue into its correct priority lane.
- **Input:** A `ReliefRequest` object (with a `priority` field).
- **Output:** Request enqueued into the matching priority lane.
- **Steps:** Look at `request.priority` → enqueue the request into that lane's internal Queue.
- **Data Structure:** Priority Queue (4 internal Queues).
- **Time Complexity:** O(1).

### 9.4 Allocate Resources
- **Purpose:** Give resources to the highest-priority waiting request.
- **Input:** Current Priority Queue, current resource inventory (Array).
- **Output:** Updated request status, updated inventory, a new entry on the Stack.
- **Steps:**
  1. Dequeue the front request from the highest non-empty priority lane.
  2. Check available quantity of the required resource in the Array.
  3. If available ≥ required → allocate full amount, mark `"Completed"`.
  4. If 0 < available < required → allocate what's available, mark `"Partially Allocated"`.
  5. If available = 0 → mark `"Waiting for Resources"`.
  6. Reduce the resource's `available` count and increase its `distributed` count.
  7. If anything was allocated, push the operation onto the Stack and insert the request into the Linked List.
- **Data Structure:** Priority Queue, Array, Stack, Linked List.
- **Time Complexity:** O(1) for the priority dequeue and inventory update (Array lookup by name is O(n) in the worst case, since it's a linear scan over a very small resource list).

### 9.5 Partial Allocation
- **Purpose:** Handle the case where available stock is less than what is required.
- **Input:** Required quantity, available quantity.
- **Output:** `allocated = available`, `remaining = required - available`, status `"Partially Allocated"`.
- **Steps:** `given = min(available, required)` → subtract `given` from inventory → add `given` to `request.allocated`.
- **Data Structure:** Array (inventory) + Structure (request).
- **Time Complexity:** O(1).

### 9.6 Add Completed Request to Linked List
- **Purpose:** Keep a record of every request that received at least some resources.
- **Input:** A `ReliefRequest` object.
- **Output:** A new node appended to the Linked List.
- **Steps:** Create a node holding the request → traverse to the last node → attach the new node.
- **Data Structure:** Singly Linked List.
- **Time Complexity:** O(n) (traversal to the end) — acceptable at this project's scale.

### 9.7 Push Allocation into Stack
- **Purpose:** Record every allocation for history/undo.
- **Input:** Allocation ID, request ID, resource, quantity, location, status.
- **Output:** A new entry on top of the Stack.
- **Steps:** Append the allocation record to the top of the stack array.
- **Data Structure:** Stack (array-based).
- **Time Complexity:** O(1).

### 9.8 Search Request
- **Purpose:** Find one or more requests matching a keyword.
- **Input:** Field (ID/location/disaster type/resource), keyword.
- **Output:** List of matching requests.
- **Steps (Linear Search):** Scan every request from index 0 to n−1, compare the chosen field, collect matches.
- **Steps (Binary Search, by ID only):** Sort requests by ID → repeatedly compare the middle element to the target ID, narrowing the search range by half each time.
- **Data Structure:** Array.
- **Time Complexity:** Linear Search O(n). Binary Search O(log n) — but requires a sorted array first (O(n²) with Bubble Sort, or O(n log n) with a better sort).

### 9.9 Sort Requests
- **Purpose:** Reorder requests by a chosen field.
- **Input:** Field (priority/population/quantity/ID), direction.
- **Output:** A new sorted array (original order preserved elsewhere).
- **Steps (Bubble Sort):** Repeatedly compare each pair of adjacent elements; swap if out of order; repeat until no swaps are needed for a full pass.
- **Data Structure:** Array.
- **Time Complexity:** O(n²) worst/average case, O(n) best case (already sorted).

### 9.10 Undo Last Allocation
- **Purpose:** Reverse the most recent allocation.
- **Input:** Current Stack.
- **Output:** Inventory restored, request status reverted.
- **Steps:**
  1. Pop the top record from the Stack.
  2. Add the popped quantity back to the resource's `available` count.
  3. Subtract it from `request.allocated`.
  4. If `request.allocated` becomes 0 → status back to `"Pending"`, remove from Linked List, reinsert into Priority Queue.
  5. Otherwise → status stays `"Partially Allocated"` with the reduced amount.
- **Data Structure:** Stack, Array, Linked List, Priority Queue.
- **Time Complexity:** O(1) for the pop; O(n) if removal from the Linked List is required.

---

## 10. Time & Space Complexity Summary

| Operation                     | Data Structure   | Time Complexity |
|--------------------------------|------------------|------------------|
| Add Request                    | Queue            | O(1) |
| Remove Request (dequeue)       | Queue            | O(1) |
| Insert into Priority Queue     | Priority Queue   | O(1) |
| Priority Processing (dequeue)  | Priority Queue   | O(1) (checks at most 4 lanes) |
| Add Completed Request          | Linked List      | O(n) (append to tail) |
| Search Resource in Inventory   | Array            | O(n) (n = number of distinct resources, small) |
| Linear Search Request          | Array            | O(n) |
| Binary Search Request          | Array (sorted)   | O(log n) |
| Sorting (Bubble Sort)          | Array            | O(n²) |
| Push Allocation History        | Stack            | O(1) |
| Pop Allocation History (Undo)  | Stack            | O(1) |

**Space Complexity:** All structures grow linearly with the number of requests/resources, i.e. O(n) overall, where n is the number of relief requests or resource types.

---

## 11. Advantages

- Clearly demonstrates 8 core FDS concepts in one cohesive, realistic scenario.
- Every operation is manually implemented — nothing is hidden inside a library.
- Simple enough for a first-year student to explain line by line in a viva.
- Runs entirely offline in a browser — no installation, server, or database needed.

## 12. Limitations

- Data is stored only in memory; refreshing the page resets the simulation (by design, to keep the project simple — no database was used, as required).
- Bubble Sort is O(n²), which is fine for a classroom-scale dataset but not for real-world millions of records.
- The simulation does not model real-time logistics (transport, distances, delivery time).

## 13. Future Scope

- Add persistent storage (localStorage or a real database) so data survives a refresh.
- Add authentication for relief-camp administrators.
- Replace Bubble Sort with Merge Sort/Quick Sort for larger datasets.
- Add a real map view showing affected areas.
- Send real notifications/alerts to relief teams.

## 14. Conclusion

RELIEF-Q shows how fundamental, textbook data structures — arrays, structures, linked lists, queues, priority queues, and stacks — combine naturally to solve a realistic, high-stakes coordination problem: getting limited resources to the people who need them most, in the right order, with a full audit trail. It proves that these "basic" concepts are not just academic exercises but the backbone of real decision-making systems.

---

## 15. How to Run the Project

### Option A — Run at localhost (recommended)
Requires only Python 3, which is pre-installed on most Mac/Linux systems and free to install on Windows.

1. Unzip the `RELIEF-Q` folder.
2. **Windows:** double-click `start.bat`.
   **Mac/Linux:** double-click `start.sh`, or run `./start.sh` in a terminal (run `chmod +x start.sh` first if needed).
   **Any OS (manual):** open a terminal in the `RELIEF-Q` folder and run `python3 server.py` (or `python server.py` on Windows).
3. Your browser opens automatically at **http://localhost:8000**. If it doesn't, open that address manually.
4. Press `CTRL+C` in the terminal to stop the server when you're done.

### Option B — Open directly, no server
1. Open the folder and double-click **`index.html`** (or right-click → Open with → any modern browser: Chrome, Edge, Firefox).
2. No installation, server, or internet connection is required (an internet connection only improves the display font; the project works fully without one).

Either way:
- The dashboard loads automatically with 10 sample relief requests and sample resource inventory already set up.
- Use the left sidebar to move between Dashboard, Relief Requests, Resource Inventory, Priority Queue, Allocation, Allocation History, Search, Sort, and Reports.

---

## 16. Viva Questions & Answers

**Q1. What is the main goal of this project?**
A: To simulate how a limited pool of disaster-relief resources should be distributed to multiple affected areas, in order of urgency, using fundamental data structures.

**Q2. Why did you use a Priority Queue instead of a normal Queue for allocation?**
A: A normal Queue only serves requests in arrival order (FIFO), but in a disaster, a Critical request that arrives later must still be served before a Low-priority request that arrived earlier. A Priority Queue serves requests by urgency first, and by arrival time only as a tie-breaker.

**Q3. Why is a normal Queue used at all, then?**
A: The Queue models the very first "intake line" — every new request waits here before it is officially classified and placed into its correct priority lane. It demonstrates plain FIFO behaviour before priority logic is applied.

**Q4. Why is a Stack used for allocation history?**
A: Undo naturally follows Last-In-First-Out order — you always want to undo the *most recent* action first. A Stack's `push`/`pop` operations directly match this behaviour.

**Q5. Why use a Linked List for completed requests instead of another array?**
A: To demonstrate a different structure than the Array already used for inventory, and to show dynamic node-based insertion, traversal, and deletion — all core Linked List operations — using a real, meaningful dataset (completed relief records).

**Q6. What happens when resources are insufficient?**
A: The system allocates as much as is available. If some quantity remains available, the request is marked "Partially Allocated" and shows Required/Allocated/Remaining. If nothing at all is available, it is marked "Waiting for Resources" until more stock is added.

**Q7. What is the difference between a Queue and a Priority Queue?**
A: A Queue removes elements strictly in the order they were added (FIFO). A Priority Queue removes elements based on priority first; only requests of equal priority follow FIFO order among themselves.

**Q8. What is FIFO?**
A: First-In-First-Out — the first element added is the first one removed. Used by the waiting Queue and within each priority lane.

**Q9. What is LIFO?**
A: Last-In-First-Out — the most recently added element is the first one removed. Used by the Allocation History Stack for the Undo feature.

**Q10. What is the time complexity of searching in this project?**
A: Linear Search is O(n) because it may check every request. Binary Search is O(log n), but only works on a sorted array (sorted by Request ID here), so a sort must happen first if the data isn't already sorted.

**Q11. What is the time complexity of Bubble Sort?**
A: O(n²) in the worst and average case, because it uses two nested loops to repeatedly compare and swap adjacent elements. Its best case is O(n) if the array is already sorted (with an early-exit optimization).

**Q12. Why is priority important in disaster management?**
A: Because resources are always limited compared to need. Prioritizing ensures the most urgent, life-threatening situations (Critical) are addressed before less urgent ones, which can save more lives with the same amount of resources.

**Q13. What is partial allocation, and why is it useful?**
A: It's when a request receives less than what it asked for because full stock isn't available. It makes the simulation realistic, since real disaster relief operations rarely have unlimited supplies.

**Q14. How does the system decide which request gets resources first?**
A: It always looks at the Priority Queue's highest non-empty priority lane (Critical → High → Medium → Low) and, within that lane, picks the request that has been waiting the longest (FIFO).

**Q15. Why did you implement the data structures manually instead of using JavaScript's built-in features?**
A: The purpose of this project is to demonstrate understanding of how these data structures work internally — their nodes, pointers, and operations — not just to use a pre-built tool. Manual implementation is what makes the FDS concepts visible and explainable.

**Q16. What is a Structure/Class used for here?**
A: The `ReliefRequest` class groups all related information about one request (ID, area, location, disaster type, population, resource, quantity, priority, status) into a single, organized unit, instead of scattering the data across many separate variables or arrays.

**Q17. Could this priority queue be implemented differently?**
A: Yes — for example, with a single sorted array/list ordered by a priority "rank" number, or with a binary heap. This project intentionally uses the simplest version (4 queues, one per level) because it is easiest to explain and matches how real relief coordination naturally works (separate lines for separate urgency levels).

**Q18. What does "arrival order" mean and how is it tracked?**
A: It's the order in which requests were submitted. Each `ReliefRequest` stores an `arrivalOrder` counter value when it is created, and because each priority level is implemented as a real Queue, FIFO order is naturally preserved without needing to check that counter manually.

---

## 17. Project Architecture

```
RELIEF-Q/
│
├── index.html              # All pages/sections of the UI
├── css/
│   └── style.css           # Complete styling (design tokens, layout, components)
├── js/
│   ├── dataStructures.js   # ALL manual data structure implementations
│   ├── sampleData.js       # Sample resources + sample relief requests
│   └── app.js               # App state, workflow logic, DOM rendering, event wiring
├── assets/                  # (reserved for any future icons/images)
└── README.md                 # This file
```
