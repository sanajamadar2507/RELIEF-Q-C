/* =========================================================================
   RELIEF-Q — app.js
   -------------------------------------------------------------------------
   Wires the data structures (dataStructures.js) to the HTML page. Also
   contains the higher-level workflow (submit -> queue -> priority queue
   -> allocate -> stack -> linked list) and all DOM rendering.
   ========================================================================= */

const app = {
  resourceArray:   new ResourceArray(),
  waitingQueue:    new Queue(),
  priorityQueue:   new PriorityQueue(),
  completedList:   new LinkedList(),
  historyStack:    new Stack(),
  allRequests:     [],          // master array of every ReliefRequest (the "Array" of records)
  waitingForResources: [],      // requests that could not be allocated at all
  eventLog:        [],
  allocCounter:    100,

  /* ---------------------------------------------------------------------
     WORKFLOW STEP 1: Submit a new request -> goes into the waiting Queue
     --------------------------------------------------------------------- */
  submitRequest(req, opts = {}) {
    this.allRequests.push(req);
    this.waitingQueue.enqueue(req);
    this.logEvent(`${req.requestID} submitted and added to Queue`, opts.silent);
  },

  /* ---------------------------------------------------------------------
     WORKFLOW STEP 2: Move the front of the Queue into the Priority Queue
     --------------------------------------------------------------------- */
  classifyNext(silent = false) {
    const req = this.waitingQueue.dequeue();
    if (!req) return null;
    this.priorityQueue.insert(req);
    this.logEvent(`${req.requestID} classified as ${req.priority.toUpperCase()} and moved to Priority Queue`, silent);
    return req;
  },

  classifyAll(silent = false) {
    while (!this.waitingQueue.isEmpty()) this.classifyNext(silent);
  },

  /* ---------------------------------------------------------------------
     WORKFLOW STEP 3: Allocate resources to a specific request
     Shared by "Allocate Next" and "Retry" (for waiting requests)
     --------------------------------------------------------------------- */
  allocateRequest(req) {
    const available = this.resourceArray.getAvailable(req.resource);
    const stillNeeded = req.quantity - req.allocated;

    if (available <= 0) {
      req.status = "Waiting for Resources";
      if (!this.waitingForResources.includes(req)) this.waitingForResources.push(req);
      this.logEvent(`${req.requestID} could not be allocated — 0 units of ${req.resource} available. Marked "Waiting for Resources".`);
      return { type: "wait", req };
    }

    const given = this.resourceArray.consume(req.resource, stillNeeded);
    req.allocated += given;

    if (req.allocated >= req.quantity) {
      req.status = "Completed";
    } else {
      req.status = "Partially Allocated";
    }

    // remove from waiting list if it was there
    const wIdx = this.waitingForResources.indexOf(req);
    if (wIdx !== -1) this.waitingForResources.splice(wIdx, 1);

    // STORE completed/distributed request in the Linked List
    if (!this.completedList.search(req.requestID)) {
      this.completedList.insert(req);
    }

    // PUSH the allocation operation onto the Stack
    this.allocCounter++;
    const record = {
      allocID: "A" + this.allocCounter,
      requestID: req.requestID,
      resource: req.resource,
      quantity: given,
      location: req.location,
      status: req.status,
      time: new Date()
    };
    this.historyStack.push(record);

    this.logEvent(`${given} unit(s) of ${req.resource} allocated to ${req.requestID} (${req.location}) — ${req.status}`);
    this.logEvent(`Allocation ${record.allocID} pushed onto Stack`);
    if (req.status === "Completed") this.logEvent(`${req.requestID} added to Linked List (Completed)`);

    return { type: req.status === "Completed" ? "ok" : "partial", req, given, record };
  },

  // Take the highest-priority request out of the Priority Queue and allocate it
  allocateNext() {
    const req = this.priorityQueue.dequeue();
    if (!req) return { type: "empty" };
    return this.allocateRequest(req);
  },

  /* ---------------------------------------------------------------------
     UNDO: pop the last allocation from the Stack (LIFO) and reverse it
     --------------------------------------------------------------------- */
  undoLast() {
    const record = this.historyStack.pop();
    if (!record) return null;

    const req = this.allRequests.find(r => r.requestID === record.requestID);
    if (req) {
      this.resourceArray.restore(record.resource, record.quantity);
      req.allocated -= record.quantity;
      if (req.allocated <= 0) {
        req.allocated = 0;
        req.status = "Pending";
        this.completedList.delete(req.requestID);
        this.priorityQueue.insert(req); // put it back for re-allocation
      } else {
        req.status = "Partially Allocated";
      }
      this.logEvent(`Undo: allocation ${record.allocID} reversed — ${record.quantity} unit(s) of ${record.resource} returned to inventory, ${req.requestID} is now "${req.status}"`);
    }
    return record;
  },

  /* ---------------------------------------------------------------------
     EVENT LOG  (feeds "Live Relief Operations")
     --------------------------------------------------------------------- */
  logEvent(text, silent = false) {
    this.eventLog.push({ time: new Date(), text });
    if (this.eventLog.length > 60) this.eventLog.shift();
    if (!silent) renderAll();
  }
};


/* ===========================================================================
   VALIDATION
   =========================================================================== */
function validateRequestForm() {
  const id         = document.getElementById("f-id").value.trim();
  const area       = document.getElementById("f-area").value.trim();
  const location   = document.getElementById("f-location").value.trim();
  const disaster   = document.getElementById("f-disaster").value;
  const population = document.getElementById("f-population").value;
  const resource   = document.getElementById("f-resource").value;
  const quantity   = document.getElementById("f-quantity").value;
  const priority   = document.getElementById("f-priority").value;

  if (!id) return "Request ID cannot be empty.";
  if (app.allRequests.some(r => r.requestID.toLowerCase() === id.toLowerCase())) return "Request ID already exists.";
  if (!area) return "Disaster Area cannot be empty.";
  if (!location) return "Location cannot be empty.";
  if (!disaster) return "Please select a Disaster Type.";
  if (!population || Number(population) <= 0) return "Population must be greater than 0.";
  if (!resource) return "Please select a Resource.";
  if (!quantity || Number(quantity) <= 0) return "Quantity must be greater than 0.";
  if (!priority) return "Please select a Priority.";

  return null; // no errors
}


/* ===========================================================================
   RENDERING
   =========================================================================== */
function badge(text, cls) {
  return `<span class="badge badge-${cls}">${text}</span>`;
}
function statusBadge(status) {
  return `<span class="badge badge-status-${status.replace(/\s+/g, "")}">${status}</span>`;
}
function fmt(n) { return Number(n).toLocaleString(); }
function timeStr(d) { return d.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" }); }

function renderDashboard() {
  const total     = app.allRequests.length;
  const pending   = app.allRequests.filter(r => r.status === "Pending").length;
  const critical  = app.allRequests.filter(r => r.priority === "Critical").length;
  const completed = app.allRequests.filter(r => r.status === "Completed").length;
  const resAvail  = app.resourceArray.getAll().reduce((s, r) => s + r.available, 0);
  const resDist   = app.resourceArray.getAll().reduce((s, r) => s + r.distributed, 0);

  document.getElementById("stat-total").textContent = fmt(total);
  document.getElementById("stat-pending").textContent = fmt(pending);
  document.getElementById("stat-critical").textContent = fmt(critical);
  document.getElementById("stat-completed").textContent = fmt(completed);
  document.getElementById("stat-res-available").textContent = fmt(resAvail);
  document.getElementById("stat-res-distributed").textContent = fmt(resDist);

  // Priority distribution bars
  const levels = ["Critical", "High", "Medium", "Low"];
  const colors = { Critical: "var(--critical)", High: "var(--high)", Medium: "var(--medium)", Low: "var(--low)" };
  const pBars = levels.map(lv => {
    const count = app.allRequests.filter(r => r.priority === lv).length;
    const pct = total ? Math.round((count / total) * 100) : 0;
    return `<div class="bar-row">
      <div class="bar-row-top"><span>${lv}</span><span>${count} request(s)</span></div>
      <div class="bar-track"><div class="bar-fill" style="width:${pct}%;background:${colors[lv]}"></div></div>
    </div>`;
  }).join("");
  document.getElementById("priorityBars").innerHTML = pBars;

  // Inventory bars
  const invBars = app.resourceArray.getAll().map(r => {
    const pct = r.total ? Math.round((r.available / r.total) * 100) : 0;
    return `<div class="bar-row">
      <div class="bar-row-top"><span>${r.name}</span><span>${fmt(r.available)} / ${fmt(r.total)} available</span></div>
      <div class="bar-track"><div class="bar-fill" style="width:${pct}%;background:var(--low)"></div></div>
    </div>`;
  }).join("");
  document.getElementById("dashInventory").innerHTML = invBars || `<p class="muted">No resources yet.</p>`;

  // Live feed (most recent last -> flex-direction column-reverse in CSS shows newest on top)
  const feedItems = app.eventLog.slice(-25).map(e =>
    `<li><span class="lf-time">${timeStr(e.time)}</span><span>${e.text}</span></li>`
  ).join("");
  document.getElementById("liveFeed").innerHTML = feedItems || `<li class="muted">No activity yet.</li>`;
}

function renderRequestsPage() {
  document.getElementById("queueCountPill").textContent = app.waitingQueue.count;

  const qRows = app.waitingQueue.toArray().map((r, i) => `
    <tr>
      <td>${i + 1}</td>
      <td class="mono">${r.requestID}</td>
      <td>${r.area}</td>
      <td>${badge(r.priority, r.priority)}</td>
      <td>${r.resource}</td>
      <td>${fmt(r.quantity)}</td>
    </tr>`).join("");
  document.querySelector("#queueTable tbody").innerHTML = qRows || `<tr><td colspan="6" class="muted">Queue is empty — every request has been classified.</td></tr>`;

  const allRows = app.allRequests.map(r => `
    <tr>
      <td class="mono">${r.requestID}</td>
      <td>${r.area}</td>
      <td>${r.location}</td>
      <td>${r.disasterType}</td>
      <td>${fmt(r.population)}</td>
      <td>${r.resource}</td>
      <td>${fmt(r.quantity)}</td>
      <td>${fmt(r.allocated)}</td>
      <td>${badge(r.priority, r.priority)}</td>
      <td>${statusBadge(r.status)}</td>
    </tr>`).join("");
  document.querySelector("#allRequestsTable tbody").innerHTML = allRows || `<tr><td colspan="10" class="muted">No requests yet.</td></tr>`;
}

function renderInventory() {
  const rows = app.resourceArray.getAll().map(r => {
    const pct = r.total ? Math.round((r.available / r.total) * 100) : 0;
    return `<tr>
      <td>${r.name}</td>
      <td>${fmt(r.total)}</td>
      <td>${fmt(r.available)}</td>
      <td>${fmt(r.distributed)}</td>
      <td style="min-width:140px">
        <div class="bar-track"><div class="bar-fill" style="width:${pct}%;background:var(--medium)"></div></div>
      </td>
    </tr>`;
  }).join("");
  document.querySelector("#inventoryTable tbody").innerHTML = rows || `<tr><td colspan="5" class="muted">No resources yet.</td></tr>`;
}

function renderPQueue() {
  const grouped = app.priorityQueue.getGrouped();
  const levels = ["Critical", "High", "Medium", "Low"];
  const html = levels.map(lv => {
    const items = grouped[lv];
    const cards = items.map(r => `
      <div class="pq-card">
        <b>${r.requestID}</b> — ${r.area}<br>
        ${r.resource} · qty ${fmt(r.quantity)}
      </div>`).join("");
    return `<div class="pq-col">
      <div class="pq-col-head ${lv}">${lv} (${items.length})</div>
      <div class="pq-col-body">${cards || '<div class="pq-empty">— empty —</div>'}</div>
    </div>`;
  }).join("");
  document.getElementById("pqLevels").innerHTML = html;
}

function renderAllocationPage() {
  const rows = app.waitingForResources.map(r => `
    <tr>
      <td class="mono">${r.requestID}</td>
      <td>${r.area}</td>
      <td>${r.resource}</td>
      <td>${fmt(r.quantity - r.allocated)}</td>
      <td>${badge(r.priority, r.priority)}</td>
      <td><button class="btn btn-secondary retry-btn" data-id="${r.requestID}">Retry</button></td>
    </tr>`).join("");
  document.querySelector("#waitingTable tbody").innerHTML = rows || `<tr><td colspan="6" class="muted">No requests waiting on resources.</td></tr>`;

  document.querySelectorAll(".retry-btn").forEach(btn => {
    btn.addEventListener("click", () => {
      const id = btn.dataset.id;
      const req = app.allRequests.find(r => r.requestID === id);
      if (req) {
        const result = app.allocateRequest(req);
        showAllocateResult(result);
        renderAll();
      }
    });
  });
}

function showAllocateResult(result) {
  const box = document.getElementById("allocateResult");
  if (result.type === "empty") {
    box.innerHTML = `<div class="result-box">Priority Queue is empty — classify more requests from the Queue first.</div>`;
    return;
  }
  const r = result.req;
  const remaining = r.quantity - r.allocated;
  if (result.type === "wait") {
    box.innerHTML = `<div class="result-box wait">
      <b>${r.requestID}</b> (${r.area}) needs ${fmt(r.quantity)} ${r.resource}, but 0 units are available.<br>
      Status: ${statusBadge(r.status)}
    </div>`;
  } else if (result.type === "partial") {
    box.innerHTML = `<div class="result-box partial">
      <b>${r.requestID}</b> (${r.area}) — Partial Allocation<br>
      Required: ${fmt(r.quantity)} &nbsp;|&nbsp; Allocated: ${fmt(r.allocated)} &nbsp;|&nbsp; Remaining: ${fmt(remaining)}<br>
      Status: ${statusBadge(r.status)}
    </div>`;
  } else {
    box.innerHTML = `<div class="result-box ok">
      <b>${r.requestID}</b> (${r.area}) fully allocated: ${fmt(result.given)} ${r.resource}.<br>
      Status: ${statusBadge(r.status)}
    </div>`;
  }
}

function renderHistory() {
  const rows = app.historyStack.toArray().map(rec => `
    <tr>
      <td class="mono">${rec.allocID}</td>
      <td class="mono">${rec.requestID}</td>
      <td>${rec.resource}</td>
      <td>${fmt(rec.quantity)}</td>
      <td>${rec.location}</td>
      <td>${statusBadge(rec.status)}</td>
      <td class="mono">${timeStr(rec.time)}</td>
    </tr>`).join("");
  document.querySelector("#historyTable tbody").innerHTML = rows || `<tr><td colspan="7" class="muted">No allocations yet.</td></tr>`;
}

function renderReports() {
  const total     = app.allRequests.length;
  const completed = app.allRequests.filter(r => r.status === "Completed").length;
  const pending   = app.allRequests.filter(r => r.status === "Pending").length;
  const critical  = app.allRequests.filter(r => r.priority === "Critical").length;
  const resDist   = app.resourceArray.getAll().reduce((s, r) => s + r.distributed, 0);

  // most requested resource
  const resCount = {};
  app.allRequests.forEach(r => { resCount[r.resource] = (resCount[r.resource] || 0) + 1; });
  let mostRequested = "—", max = 0;
  for (const key in resCount) { if (resCount[key] > max) { max = resCount[key]; mostRequested = key; } }

  document.getElementById("reportSummary").innerHTML = `
    <li><span>Total resources distributed</span><b>${fmt(resDist)}</b></li>
    <li><span>Most requested resource</span><b>${mostRequested}</b></li>
    <li><span>Critical requests</span><b>${fmt(critical)}</b></li>
    <li><span>Completed requests</span><b>${fmt(completed)}</b></li>
    <li><span>Pending requests</span><b>${fmt(pending)}</b></li>
    <li><span>Total requests logged</span><b>${fmt(total)}</b></li>
  `;

  const levels = ["Critical", "High", "Medium", "Low"];
  const colors = { Critical: "var(--critical)", High: "var(--high)", Medium: "var(--medium)", Low: "var(--low)" };
  document.getElementById("reportPriority").innerHTML = levels.map(lv => {
    const count = app.allRequests.filter(r => r.priority === lv).length;
    const pct = total ? Math.round((count / total) * 100) : 0;
    return `<div class="bar-row">
      <div class="bar-row-top"><span>${lv}</span><span>${count}</span></div>
      <div class="bar-track"><div class="bar-fill" style="width:${pct}%;background:${colors[lv]}"></div></div>
    </div>`;
  }).join("");

  const disasterTypes = ["Flood", "Earthquake", "Cyclone", "Landslide", "Drought", "Fire"];
  document.getElementById("reportDisaster").innerHTML = disasterTypes.map(dt => {
    const count = app.allRequests.filter(r => r.disasterType === dt).length;
    const pct = total ? Math.round((count / total) * 100) : 0;
    return `<div class="bar-row">
      <div class="bar-row-top"><span>${dt}</span><span>${count}</span></div>
      <div class="bar-track"><div class="bar-fill" style="width:${pct}%;background:var(--navy-soft)"></div></div>
    </div>`;
  }).join("");

  const completedRows = app.completedList.toArray().map(r => `
    <tr>
      <td class="mono">${r.requestID}</td>
      <td>${r.area}</td>
      <td>${r.resource}</td>
      <td>${fmt(r.allocated)}</td>
      <td>${statusBadge(r.status)}</td>
    </tr>`).join("");
  document.querySelector("#completedTable tbody").innerHTML = completedRows || `<tr><td colspan="5" class="muted">No completed requests yet.</td></tr>`;
}

function renderAll() {
  renderDashboard();
  renderRequestsPage();
  renderInventory();
  renderPQueue();
  renderAllocationPage();
  renderHistory();
  renderReports();
}


/* ===========================================================================
   NAVIGATION
   =========================================================================== */
const pageTitles = {
  dashboard:  ["Dashboard", "Live overview of the current relief situation"],
  requests:   ["Relief Requests", "Submit new requests and track the waiting Queue"],
  inventory:  ["Resource Inventory", "Array-based store of all available relief resources"],
  pqueue:     ["Priority Queue", "Critical requests are always served first"],
  allocation: ["Allocate Relief Resources", "Match the highest-priority request against available stock"],
  history:    ["Allocation History", "Stack (LIFO) of every allocation performed"],
  search:     ["Search Requests", "Linear Search and Binary Search demonstrations"],
  sort:       ["Sort Requests", "Bubble Sort demonstration"],
  reports:    ["Reports", "Aggregate statistics across the whole simulation"],
  about:      ["FDS Map & Docs", "Where every data structure is used in this project"]
};

function goToPage(name) {
  document.querySelectorAll(".page").forEach(p => p.classList.remove("active"));
  document.getElementById("page-" + name).classList.add("active");
  document.querySelectorAll(".nav-item").forEach(b => b.classList.toggle("active", b.dataset.page === name));
  document.getElementById("pageTitle").textContent = pageTitles[name][0];
  document.getElementById("pageSubtitle").textContent = pageTitles[name][1];
}

document.querySelectorAll(".nav-item").forEach(btn => {
  btn.addEventListener("click", () => goToPage(btn.dataset.page));
});


/* ===========================================================================
   EVENT WIRING
   =========================================================================== */

// Submit relief request
document.getElementById("requestForm").addEventListener("submit", e => {
  e.preventDefault();
  const err = validateRequestForm();
  const errBox = document.getElementById("formError");
  if (err) { errBox.textContent = err; return; }
  errBox.textContent = "";

  const req = new ReliefRequest(
    document.getElementById("f-id").value.trim(),
    document.getElementById("f-area").value.trim(),
    document.getElementById("f-location").value.trim(),
    document.getElementById("f-disaster").value,
    document.getElementById("f-population").value,
    document.getElementById("f-resource").value,
    document.getElementById("f-quantity").value,
    document.getElementById("f-priority").value
  );
  app.submitRequest(req);
  e.target.reset();
  renderAll();
});

// Classify next queued request into the Priority Queue
document.getElementById("classifyNextBtn").addEventListener("click", () => {
  const req = app.classifyNext();
  if (!req) alert("The waiting Queue is empty.");
  renderAll();
});

// Add resource
document.getElementById("resourceForm").addEventListener("submit", e => {
  e.preventDefault();
  const name = document.getElementById("r-name").value.trim();
  const qty = Number(document.getElementById("r-qty").value);
  if (!name || qty <= 0) return;
  app.resourceArray.addResource(name, qty);
  app.logEvent(`${fmt(qty)} unit(s) of ${name} added to inventory`);
  e.target.reset();
  renderAll();
});

// Allocate next (both buttons trigger the same workflow step)
function handleAllocateNext() {
  const result = app.allocateNext();
  showAllocateResult(result);
  renderAll();
  goToPage("allocation");
}
document.getElementById("allocateBtn").addEventListener("click", handleAllocateNext);
document.getElementById("pqAllocateBtn").addEventListener("click", handleAllocateNext);

// Undo last allocation
document.getElementById("undoBtn").addEventListener("click", () => {
  const record = app.undoLast();
  if (!record) alert("No allocations to undo.");
  renderAll();
});

// Search
document.getElementById("searchBtn").addEventListener("click", () => {
  const field = document.getElementById("searchField").value;
  const keyword = document.getElementById("searchInput").value.trim();
  if (!keyword) { document.getElementById("searchMeta").textContent = "Type something to search for."; return; }
  const results = linearSearchRequests(app.allRequests, field, keyword);
  document.getElementById("searchMeta").textContent = `Linear Search scanned ${app.allRequests.length} record(s) and found ${results.length} match(es).`;
  renderSearchResults(results);
});

document.getElementById("binarySearchBtn").addEventListener("click", () => {
  const keyword = document.getElementById("searchInput").value.trim();
  if (!keyword) { document.getElementById("searchMeta").textContent = "Type a Request ID to use Binary Search."; return; }
  const sorted = bubbleSortRequests(app.allRequests, "requestID", true);
  const { result, steps } = binarySearchByID(sorted, keyword);
  document.getElementById("searchMeta").textContent = result
    ? `Binary Search found ${keyword} in ${steps.length} comparison(s) (array pre-sorted by ID).`
    : `Binary Search checked ${steps.length} comparison(s) — ${keyword} was not found.`;
  renderSearchResults(result ? [result] : []);
});

function renderSearchResults(results) {
  const rows = results.map(r => `
    <tr>
      <td class="mono">${r.requestID}</td>
      <td>${r.area}</td>
      <td>${r.location}</td>
      <td>${r.disasterType}</td>
      <td>${r.resource}</td>
      <td>${badge(r.priority, r.priority)}</td>
      <td>${statusBadge(r.status)}</td>
    </tr>`).join("");
  document.querySelector("#searchTable tbody").innerHTML = rows || `<tr><td colspan="7" class="muted">No matches.</td></tr>`;
}

// Sort
document.getElementById("sortBtn").addEventListener("click", () => {
  const field = document.getElementById("sortField").value;
  const asc = document.getElementById("sortDir").value === "asc";

  const beforeRows = app.allRequests.map(r => `
    <tr><td class="mono">${r.requestID}</td><td>${badge(r.priority, r.priority)}</td><td>${fmt(r.population)}</td><td>${fmt(r.quantity)}</td></tr>`).join("");
  document.querySelector("#beforeSortTable tbody").innerHTML = beforeRows;

  const sorted = bubbleSortRequests(app.allRequests, field, asc);
  const afterRows = sorted.map(r => `
    <tr><td class="mono">${r.requestID}</td><td>${badge(r.priority, r.priority)}</td><td>${fmt(r.population)}</td><td>${fmt(r.quantity)}</td></tr>`).join("");
  document.querySelector("#afterSortTable tbody").innerHTML = afterRows;
});


/* ===========================================================================
   CLOCK
   =========================================================================== */
function tickClock() {
  document.getElementById("clock").textContent = new Date().toLocaleString([], {
    weekday: "short", hour: "2-digit", minute: "2-digit", second: "2-digit"
  });
}
setInterval(tickClock, 1000);
tickClock();


/* ===========================================================================
   BOOTSTRAP
   =========================================================================== */
(function init() {
  loadSampleResources(app.resourceArray);
  loadSampleRequests(app);        // all sample requests enter the waiting Queue
  app.classifyAll(true);          // demo: move them all into the Priority Queue silently

  // Pre-run a couple of allocations so History / Completed pages aren't empty on load
  app.allocateNext();
  app.allocateNext();

  app.logEvent("RELIEF-Q initialized with sample data", true);
  renderAll();
  document.querySelector("#beforeSortTable tbody").innerHTML =
    `<tr><td colspan="4" class="muted">Click "Sort" to compare before/after.</td></tr>`;
})();
