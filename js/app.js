/* =========================================================================
   RELIEF-Q — app.js
   -------------------------------------------------------------------------
   Connects the disaster-response command-center UI directly to the compiled
   C WebAssembly engine. The C engine remains the sole, authoritative source
   of simulation state, data structures, and algorithms.
   ========================================================================= */

// Formatting helpers
function fmt(n) {
  return Number(n || 0).toLocaleString();
}

function badge(priority) {
  return `<span class="badge badge-${priority}">${priority}</span>`;
}

function statusBadge(status) {
  const clean = String(status || '').replace(/\s+/g, '');
  return `<span class="badge badge-status-${clean}">${status}</span>`;
}

// Global page navigation
window.pageTitles = {
  dashboard:  ["Dashboard", "Live overview of current disaster relief allocation and inventory"],
  requests:   ["Relief Requests", "Submit new requests and track the waiting FIFO Queue"],
  inventory:  ["Resource Inventory", "Direct array-based warehouse storage of all supplies"],
  pqueue:     ["Priority Queue", "Critical emergency lane served first; ties broken by arrival"],
  allocation: ["Resource Allocation Operations", "Match highest-priority demands against available warehouse stock"],
  history:    ["Allocation History (Stack)", "LIFO stack tracking all allocations with full undo capability"],
  search:     ["Search Requests", "Demonstrating Linear Search and Binary Search in C"],
  sort:       ["Bubble Sort Demonstration", "Quadratic comparison sorting by priority, population, quantity, or ID"],
  reports:    ["Analytics & Reports", "Aggregated disaster metrics and Singly Linked List records"],
  about:      ["FODS Architecture & Viva Guide", "Where every fundamental data structure is implemented in C"]
};

window.goToPage = function(name) {
  if (!window.pageTitles[name]) return;
  document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
  const target = document.getElementById('page-' + name);
  if (target) target.classList.add('active');

  document.querySelectorAll('.nav-item').forEach(b => {
    b.classList.toggle('active', b.dataset.page === name);
  });

  document.getElementById('pageTitle').textContent = window.pageTitles[name][0];
  document.getElementById('pageSubtitle').textContent = window.pageTitles[name][1];

  // Close mobile sidebar if open
  const sidebar = document.getElementById('sidebar');
  if (sidebar) sidebar.classList.remove('mobile-open');

  // Trigger relevant renders if needed
  if (name === 'sort') renderSortPreview();
};

/* ========================================================
   RENDER FUNCTIONS (All read exclusively from C Engine)
   ======================================================== */

function renderDashboard() {
  const stats = WasmEngine.getDashboardStats();
  const allReqs = WasmEngine.getAllRequests();
  const inv = WasmEngine.getInventory();
  const liveFeed = WasmEngine.getLiveFeed();

  // Stat values
  document.getElementById('stat-total').textContent = fmt(stats.totalRequests);
  document.getElementById('stat-pending').textContent = fmt(stats.pending);
  document.getElementById('stat-critical').textContent = fmt(stats.critical);
  document.getElementById('stat-completed').textContent = fmt(stats.completed);
  document.getElementById('stat-res-available').textContent = fmt(stats.resAvailable);
  document.getElementById('stat-res-distributed').textContent = fmt(stats.resDistributed);

  // Priority distribution bars
  const levels = ["Critical", "High", "Medium", "Low"];
  const colors = {
    Critical: "var(--critical)",
    High: "var(--high)",
    Medium: "var(--medium)",
    Low: "var(--low)"
  };
  const total = stats.totalRequests || 1;
  const pBars = levels.map(lv => {
    const count = allReqs.filter(r => r.priority === lv).length;
    const pct = Math.round((count / total) * 100);
    return `
      <div class="bar-row">
        <div class="bar-row-top">
          <span>${lv}</span>
          <span>${count} request(s) (${pct}%)</span>
        </div>
        <div class="bar-track">
          <div class="bar-fill" style="width:${pct}%;background:${colors[lv]}"></div>
        </div>
      </div>`;
  }).join('');
  document.getElementById('priorityBars').innerHTML = pBars;

  // Inventory snapshot bars
  const invBars = inv.map(r => {
    const pct = r.total ? Math.round((r.available / r.total) * 100) : 0;
    return `
      <div class="bar-row">
        <div class="bar-row-top">
          <span>${r.name}</span>
          <span>${fmt(r.available)} / ${fmt(r.total)} units available</span>
        </div>
        <div class="bar-track">
          <div class="bar-fill" style="width:${pct}%;background:var(--teal)"></div>
        </div>
      </div>`;
  }).join('');
  document.getElementById('dashInventory').innerHTML = invBars || '<p class="muted">No resources in inventory.</p>';

  // Live feed
  const feedItems = liveFeed.slice(-25).map(e => `
    <li>
      <span class="lf-time">${e.time}</span>
      <span>${e.text}</span>
    </li>
  `).join('');
  document.getElementById('liveFeed').innerHTML = feedItems || '<li class="muted">No activity yet.</li>';
}

function renderRequestsPage() {
  const qData = WasmEngine.getWaitingQueue();
  const allReqs = WasmEngine.getAllRequests();

  // Queue pill & badge
  document.getElementById('queueCountPill').textContent = qData.count;
  const navBadge = document.getElementById('navQueueBadge');
  if (navBadge) navBadge.textContent = qData.count;

  // Waiting queue table (FIFO)
  const qRows = (qData.items || []).map((r, i) => `
    <tr>
      <td class="mono">${i + 1}</td>
      <td class="mono"><strong>${r.requestID}</strong></td>
      <td>${r.area}</td>
      <td>${badge(r.priority)}</td>
      <td>${r.resource}</td>
      <td class="mono">${fmt(r.quantity)}</td>
    </tr>
  `).join('');
  document.querySelector('#queueTable tbody').innerHTML = qRows || `
    <tr><td colspan="6" class="muted" style="text-align:center;padding:16px;">Queue is empty — all submitted requests have been classified into the Priority Queue.</td></tr>
  `;

  // Master all requests table
  const query = (document.getElementById('masterTableSearch')?.value || '').toLowerCase();
  const filtered = query
    ? allReqs.filter(r => 
        r.requestID.toLowerCase().includes(query) ||
        r.area.toLowerCase().includes(query) ||
        r.location.toLowerCase().includes(query) ||
        r.resource.toLowerCase().includes(query) ||
        r.priority.toLowerCase().includes(query) ||
        r.status.toLowerCase().includes(query))
    : allReqs;

  const allRows = filtered.map(r => `
    <tr>
      <td class="mono"><strong>${r.requestID}</strong></td>
      <td>${r.area}</td>
      <td>${r.location}</td>
      <td>${r.disasterType}</td>
      <td class="mono">${fmt(r.population)}</td>
      <td>${r.resource}</td>
      <td class="mono">${fmt(r.quantity)}</td>
      <td class="mono">${fmt(r.allocated)}</td>
      <td>${badge(r.priority)}</td>
      <td>${statusBadge(r.status)}</td>
    </tr>
  `).join('');
  document.querySelector('#allRequestsTable tbody').innerHTML = allRows || `
    <tr><td colspan="10" class="muted" style="text-align:center;padding:16px;">No requests found.</td></tr>
  `;
}

function renderInventory() {
  const inv = WasmEngine.getInventory();
  const rows = inv.map(r => {
    const pct = r.total ? Math.round((r.available / r.total) * 100) : 0;
    return `
      <tr>
        <td><strong>${r.name}</strong></td>
        <td class="mono">${fmt(r.total)}</td>
        <td class="mono" style="color:var(--teal-dark);font-weight:700;">${fmt(r.available)}</td>
        <td class="mono">${fmt(r.distributed)}</td>
        <td style="min-width:160px">
          <div class="bar-track">
            <div class="bar-fill" style="width:${pct}%;background:${pct > 25 ? 'var(--teal)' : 'var(--critical)'}"></div>
          </div>
          <span style="font-size:11px;color:var(--muted);">${pct}% in stock</span>
        </td>
      </tr>
    `;
  }).join('');
  document.querySelector('#inventoryTable tbody').innerHTML = rows || `
    <tr><td colspan="5" class="muted" style="text-align:center;padding:16px;">No resources currently recorded.</td></tr>
  `;
}

function renderPQueue() {
  const pqData = WasmEngine.getPriorityQueue();
  const navBadge = document.getElementById('navPqBadge');
  if (navBadge) navBadge.textContent = pqData.total;

  const levels = ["Critical", "High", "Medium", "Low"];
  const html = levels.map(lv => {
    const items = (pqData.levels && pqData.levels[lv]) ? pqData.levels[lv] : [];
    const cards = items.map(r => `
      <div class="pq-card">
        <b>${r.requestID}</b> — ${r.area} (${r.location})<br>
        <span style="color:var(--muted);font-size:11.5px;">${r.resource} · Qty: <strong>${fmt(r.quantity)}</strong></span>
      </div>
    `).join('');

    return `
      <div class="pq-col">
        <div class="pq-col-head ${lv}">
          <span>${lv}</span>
          <span class="pill" style="background:rgba(255,255,255,0.25);color:#fff;">${items.length}</span>
        </div>
        <div class="pq-col-body">
          ${cards || '<div class="pq-empty">— Lane Empty —</div>'}
        </div>
      </div>
    `;
  }).join('');
  document.getElementById('pqLevels').innerHTML = html;
}

function renderAllocationPage() {
  const waiting = WasmEngine.getWaitingResources();
  const rows = waiting.map(r => `
    <tr>
      <td class="mono"><strong>${r.requestID}</strong></td>
      <td>${r.area} (${r.location})</td>
      <td>${r.resource}</td>
      <td class="mono" style="color:var(--critical);font-weight:700;">${fmt(r.quantity - r.allocated)}</td>
      <td>${badge(r.priority)}</td>
      <td>
        <button class="btn btn-sm btn-primary retry-btn" data-id="${r.requestID}">
          Retry Allocation
        </button>
      </td>
    </tr>
  `).join('');

  document.querySelector('#waitingTable tbody').innerHTML = rows || `
    <tr><td colspan="6" class="muted" style="text-align:center;padding:16px;">No requests currently waiting for resources. All processed requests received stock!</td></tr>
  `;

  // Attach retry button listeners
  document.querySelectorAll('.retry-btn').forEach(btn => {
    btn.onclick = () => {
      const id = btn.dataset.id;
      const res = WasmEngine.allocateById(id);
      showAllocateResult(res);
      renderAll();
    };
  });
}

function showAllocateResult(res) {
  const box = document.getElementById('allocateResult');
  if (!res || res.type === 'empty') {
    box.innerHTML = `
      <div class="result-box">
        <strong>Priority Queue is Empty:</strong> All requests have been served or none are classified yet. Submit more requests or click "Classify Next" from the Relief Requests page.
      </div>
    `;
    return;
  }

  const r = res.req;
  const remaining = r.quantity - r.allocated;

  if (res.type === 'wait') {
    box.innerHTML = `
      <div class="result-box wait">
        <strong>⚠️ Resource Shortage:</strong> Request <b>${r.requestID}</b> for <b>${r.area}</b> needs <b>${fmt(r.quantity)} ${r.resource}</b>, but 0 units are available in the warehouse.<br>
        Request status is now: ${statusBadge(r.status)}. Added to "Requests Waiting for Resources". Replenish stock via the Inventory page and click Retry.
      </div>
    `;
  } else if (res.type === 'partial') {
    box.innerHTML = `
      <div class="result-box partial">
        <strong>⚡ Partial Allocation:</strong> Request <b>${r.requestID}</b> for <b>${r.area}</b> partially fulfilled.<br>
        Stock allocated: <b>${fmt(res.given)} ${r.resource}</b> &nbsp;|&nbsp; Remaining deficit: <b>${fmt(remaining)}</b> &nbsp;|&nbsp; Status: ${statusBadge(r.status)}<br>
        Recorded in Allocation History as <code>${res.record?.allocID || 'A-'}</code>.
      </div>
    `;
  } else {
    box.innerHTML = `
      <div class="result-box ok">
        <strong>✅ Full Allocation Completed:</strong> Request <b>${r.requestID}</b> for <b>${r.area}</b> (${r.location}) fully satisfied with <b>${fmt(res.given)} ${r.resource}</b>!<br>
        Status: ${statusBadge(r.status)} &nbsp;|&nbsp; Stored in Completed Linked List &nbsp;|&nbsp; Allocation ID: <code>${res.record?.allocID || 'A-'}</code> pushed to Stack.
      </div>
    `;
  }
}

function renderHistory() {
  const hist = WasmEngine.getHistory();
  const rows = hist.map(rec => `
    <tr>
      <td class="mono"><strong>${rec.allocID}</strong></td>
      <td class="mono">${rec.requestID}</td>
      <td>${rec.resource}</td>
      <td class="mono">${fmt(rec.quantity)}</td>
      <td>${rec.location}</td>
      <td>${statusBadge(rec.status)}</td>
      <td class="mono" style="color:var(--muted);">${rec.time}</td>
    </tr>
  `).join('');

  document.querySelector('#historyTable tbody').innerHTML = rows || `
    <tr><td colspan="7" class="muted" style="text-align:center;padding:16px;">No allocations performed yet. History Stack is empty.</td></tr>
  `;
}

function renderReports() {
  const rep = WasmEngine.getReports();
  const completed = WasmEngine.getCompletedList();

  document.getElementById('reportSummary').innerHTML = `
    <li><span>Total warehouse resources distributed</span><b>${fmt(rep.totalDistributed)} units</b></li>
    <li><span>Most heavily demanded resource</span><b>${rep.mostRequested}</b></li>
    <li><span>Critical urgency requests</span><b>${fmt(rep.criticalCount)}</b></li>
    <li><span>Completed requests (Fully satisfied)</span><b>${fmt(rep.completedCount)}</b></li>
    <li><span>Pending requests</span><b>${fmt(rep.pendingCount)}</b></li>
    <li><span>Total relief requests logged</span><b>${fmt(rep.totalLogged)}</b></li>
  `;

  // Priority breakdown
  const levels = ["Critical", "High", "Medium", "Low"];
  const colors = { Critical: "var(--critical)", High: "var(--high)", Medium: "var(--medium)", Low: "var(--low)" };
  const total = rep.totalLogged || 1;
  document.getElementById('reportPriority').innerHTML = levels.map(lv => {
    const count = rep.priorities ? (rep.priorities[lv] || 0) : 0;
    const pct = Math.round((count / total) * 100);
    return `
      <div class="bar-row">
        <div class="bar-row-top"><span>${lv}</span><span>${count} (${pct}%)</span></div>
        <div class="bar-track"><div class="bar-fill" style="width:${pct}%;background:${colors[lv]}"></div></div>
      </div>
    `;
  }).join('');

  // Disaster breakdown
  const disasterTypes = ["Flood", "Earthquake", "Cyclone", "Landslide", "Drought", "Fire"];
  document.getElementById('reportDisaster').innerHTML = disasterTypes.map(dt => {
    const count = rep.disasters ? (rep.disasters[dt] || 0) : 0;
    const pct = Math.round((count / total) * 100);
    return `
      <div class="bar-row">
        <div class="bar-row-top"><span>${dt}</span><span>${count} (${pct}%)</span></div>
        <div class="bar-track"><div class="bar-fill" style="width:${pct}%;background:var(--navy-surface)"></div></div>
      </div>
    `;
  }).join('');

  // Completed requests linked list table
  const completedRows = completed.map(r => `
    <tr>
      <td class="mono"><strong>${r.requestID}</strong></td>
      <td>${r.area} (${r.location})</td>
      <td>${r.resource}</td>
      <td class="mono">${fmt(r.allocated)}</td>
      <td>${statusBadge(r.status)}</td>
    </tr>
  `).join('');
  document.querySelector('#completedTable tbody').innerHTML = completedRows || `
    <tr><td colspan="5" class="muted" style="text-align:center;padding:16px;">No completed requests in the Linked List yet.</td></tr>
  `;
}

function renderSortPreview() {
  const allReqs = WasmEngine.getAllRequests();
  const rows = allReqs.map(r => `
    <tr>
      <td class="mono">${r.requestID}</td>
      <td>${badge(r.priority)}</td>
      <td class="mono">${fmt(r.population)}</td>
      <td class="mono">${fmt(r.quantity)}</td>
    </tr>
  `).join('');
  document.querySelector('#beforeSortTable tbody').innerHTML = rows || `
    <tr><td colspan="4" class="muted">No requests recorded yet.</td></tr>
  `;
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

/* ========================================================
   EVENT HANDLERS & USER INTERACTIONS
   ======================================================== */

function setupEventHandlers() {
  // Navigation item clicks
  document.querySelectorAll('.nav-item').forEach(btn => {
    btn.onclick = () => window.goToPage(btn.dataset.page);
  });

  // Mobile menu toggle
  const mobileToggle = document.getElementById('mobileToggle');
  const sidebar = document.getElementById('sidebar');
  if (mobileToggle && sidebar) {
    mobileToggle.onclick = () => sidebar.classList.toggle('mobile-open');
  }

  // Master table live filter
  const filterInput = document.getElementById('masterTableSearch');
  if (filterInput) {
    filterInput.oninput = () => renderRequestsPage();
  }

  // Submit Request Form
  const reqForm = document.getElementById('requestForm');
  if (reqForm) {
    reqForm.onsubmit = (e) => {
      e.preventDefault();
      const id = document.getElementById('f-id').value.trim();
      const area = document.getElementById('f-area').value.trim();
      const loc = document.getElementById('f-location').value.trim();
      const dis = document.getElementById('f-disaster').value;
      const pop = document.getElementById('f-population').value;
      const res = document.getElementById('f-resource').value;
      const qty = document.getElementById('f-quantity').value;
      const pri = document.getElementById('f-priority').value;

      const errBox = document.getElementById('formError');
      errBox.textContent = '';

      const outcome = WasmEngine.submitRequest(id, area, loc, dis, pop, res, qty, pri);
      if (outcome.status === 'error') {
        errBox.textContent = outcome.message || 'Error submitting request';
        return;
      }

      reqForm.reset();
      renderAll();
      alert(`Request ${id} successfully submitted and enqueued in FIFO Queue.`);
    };
  }

  // Classify Next / All buttons
  document.getElementById('classifyNextBtn').onclick = () => {
    const outcome = WasmEngine.classifyNext();
    if (outcome.status === 'empty') {
      alert('The waiting FIFO Queue is empty. All requests have been classified.');
    }
    renderAll();
  };

  document.getElementById('classifyAllBtn').onclick = () => {
    WasmEngine.classifyAll();
    renderAll();
  };

  const dashClassify = document.getElementById('dashClassifyBtn');
  if (dashClassify) {
    dashClassify.onclick = () => {
      WasmEngine.classifyAll();
      renderAll();
    };
  }

  // Add Resource Form
  const resForm = document.getElementById('resourceForm');
  if (resForm) {
    resForm.onsubmit = (e) => {
      e.preventDefault();
      const name = document.getElementById('r-name').value.trim();
      const qty = document.getElementById('r-qty').value;
      if (!name || Number(qty) <= 0) return;

      const outcome = WasmEngine.addResource(name, qty);
      if (outcome.status === 'error') {
        alert(outcome.message);
        return;
      }
      resForm.reset();
      renderAll();
      alert(`Added ${fmt(qty)} unit(s) of ${name} to warehouse inventory.`);
    };
  }

  // Allocate Next buttons
  const handleAllocate = () => {
    const outcome = WasmEngine.allocateNext();
    showAllocateResult(outcome);
    renderAll();
    window.goToPage('allocation');
  };

  document.getElementById('allocateBtn').onclick = handleAllocate;
  document.getElementById('pqAllocateBtn').onclick = handleAllocate;
  const dashAlloc = document.getElementById('dashAllocateBtn');
  if (dashAlloc) dashAlloc.onclick = handleAllocate;

  // Undo Last Allocation button
  document.getElementById('undoBtn').onclick = () => {
    const outcome = WasmEngine.undoLast();
    if (outcome.status === 'empty') {
      alert('No allocations remaining on the Stack to undo.');
      return;
    }
    renderAll();
    alert(`Undo successful! Allocation ${outcome.record?.allocID} was popped from the Stack and stock was restored to warehouse inventory.`);
  };

  // Linear Search
  document.getElementById('searchBtn').onclick = () => {
    const field = document.getElementById('searchField').value;
    const query = document.getElementById('searchInput').value.trim();
    const meta = document.getElementById('searchMeta');

    if (!query) {
      meta.textContent = 'Please enter a search query.';
      return;
    }

    const res = WasmEngine.searchLinear(field, query);
    meta.textContent = `Linear Search scanned all ${res.scanned} record(s) sequentially and found ${res.count} match(es).`;

    const rows = (res.results || []).map(r => `
      <tr>
        <td class="mono"><strong>${r.requestID}</strong></td>
        <td>${r.area}</td>
        <td>${r.location}</td>
        <td>${r.disasterType}</td>
        <td>${r.resource}</td>
        <td>${badge(r.priority)}</td>
        <td>${statusBadge(r.status)}</td>
      </tr>
    `).join('');
    document.querySelector('#searchTable tbody').innerHTML = rows || `
      <tr><td colspan="7" class="muted" style="text-align:center;padding:16px;">No matching records found.</td></tr>
    `;
  };

  // Binary Search by ID
  document.getElementById('binarySearchBtn').onclick = () => {
    const query = document.getElementById('searchInput').value.trim();
    const meta = document.getElementById('searchMeta');

    if (!query) {
      meta.textContent = 'Please enter a Request ID (e.g. R104) for Binary Search.';
      return;
    }

    const res = WasmEngine.searchBinary(query);
    if (res.found && res.match) {
      meta.textContent = `Binary Search located ${query} in ${res.stepCount} comparison(s) across ${res.totalElements} pre-sorted elements (O(log n)). Check indices visited: [${res.steps.join(' → ')}].`;
      const r = res.match;
      document.querySelector('#searchTable tbody').innerHTML = `
        <tr>
          <td class="mono"><strong>${r.requestID}</strong></td>
          <td>${r.area}</td>
          <td>${r.location}</td>
          <td>${r.disasterType}</td>
          <td>${r.resource}</td>
          <td>${badge(r.priority)}</td>
          <td>${statusBadge(r.status)}</td>
        </tr>
      `;
    } else {
      meta.textContent = `Binary Search evaluated ${res.stepCount} comparison(s) — Request ID "${query}" was not found.`;
      document.querySelector('#searchTable tbody').innerHTML = `
        <tr><td colspan="7" class="muted" style="text-align:center;padding:16px;">Request ID not found.</td></tr>
      `;
    }
  };

  // Bubble Sort
  document.getElementById('sortBtn').onclick = () => {
    const field = document.getElementById('sortField').value;
    const asc = document.getElementById('sortDir').value === 'asc';

    renderSortPreview(); // populate before table

    const res = WasmEngine.sortBubble(field, asc);
    const sortedRows = (res.sorted || []).map(r => `
      <tr>
        <td class="mono">${r.requestID}</td>
        <td>${badge(r.priority)}</td>
        <td class="mono">${fmt(r.population)}</td>
        <td class="mono">${fmt(r.quantity)}</td>
      </tr>
    `).join('');
    document.querySelector('#afterSortTable tbody').innerHTML = sortedRows || `
      <tr><td colspan="4" class="muted">No records sorted.</td></tr>
    `;
  };

  // Quick reload and reset buttons
  document.getElementById('quickReloadBtn').onclick = () => {
    if (confirm('Reload the original 10 sample requests and warehouse inventory?')) {
      WasmEngine.reloadSampleData();
      renderAll();
      alert('Sample data reloaded successfully!');
    }
  };

  document.getElementById('quickResetBtn').onclick = () => {
    if (confirm('Reset the entire simulator to an empty clean state?')) {
      WasmEngine.resetSystem();
      renderAll();
      alert('Simulator reset to empty state.');
    }
  };

  // Real-time Clock
  function tickClock() {
    const el = document.getElementById('clock');
    if (el) {
      el.textContent = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    }
  }
  setInterval(tickClock, 1000);
  tickClock();
}

/* ========================================================
   INITIALIZATION BOOTSTRAP
   ======================================================== */
window.addEventListener('DOMContentLoaded', async () => {
  setupEventHandlers();
  
  // Listen for WebAssembly ready event
  window.addEventListener('wasm-ready', () => {
    renderAll();
    renderSortPreview();
    console.log('🚀 RELIEF-Q Command Center UI Rendered from WebAssembly C Engine');
  });

  // Trigger Wasm engine initialization
  await WasmEngine.init();
});
