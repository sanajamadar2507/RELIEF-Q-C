/* =========================================================================
   RELIEF-Q — wasm_loader.js
   -------------------------------------------------------------------------
   Loads and initializes the compiled WebAssembly C Engine (relief_q.wasm),
   binds all exported C functions via cwrap, and exposes a high-level API
   returning parsed JSON objects from the authoritative C simulation core.
   ========================================================================= */

window.WasmEngine = {
  isReady: false,
  api: null,
  rawModule: null,

  async init() {
    const statusCard = document.getElementById('engineStatusCard');
    const statusLabel = document.getElementById('engineLabel');
    const statusDetails = document.getElementById('engineDetails');

    try {
      if (typeof createReliefQModule !== 'function') {
        throw new Error('createReliefQModule function not found. Ensure wasm/relief_q.js is loaded.');
      }

      if (statusLabel) statusLabel.textContent = 'Compiling WebAssembly…';

      // Initialize the Emscripten module
      const Module = await createReliefQModule({
        locateFile(path, scriptDirectory) {
          if (path.endsWith('.wasm')) {
            return 'wasm/' + path;
          }
          return (scriptDirectory || '') + path;
        }
      });

      this.rawModule = Module;

      // Wrap exported C functions
      this.api = {
        initSystem:          Module.cwrap('rq_init_system', 'string', []),
        resetSystem:         Module.cwrap('rq_reset_system', 'string', []),
        loadSampleData:      Module.cwrap('rq_load_sample_data', 'string', []),
        submitRequest:       Module.cwrap('rq_submit_request', 'string', ['string', 'string', 'string', 'string', 'number', 'string', 'number', 'string']),
        classifyNext:        Module.cwrap('rq_classify_next', 'string', []),
        classifyAll:         Module.cwrap('rq_classify_all', 'string', []),
        allocateNext:        Module.cwrap('rq_allocate_next', 'string', []),
        allocateById:        Module.cwrap('rq_allocate_request_by_id', 'string', ['string']),
        undoLast:            Module.cwrap('rq_undo_last', 'string', []),
        addResource:         Module.cwrap('rq_add_resource', 'string', ['string', 'number']),
        
        getDashboardStats:   Module.cwrap('rq_get_dashboard_stats_json', 'string', []),
        getInventory:        Module.cwrap('rq_get_inventory_json', 'string', []),
        getWaitingQueue:     Module.cwrap('rq_get_waiting_queue_json', 'string', []),
        getPriorityQueue:    Module.cwrap('rq_get_priority_queue_json', 'string', []),
        getAllRequests:      Module.cwrap('rq_get_all_requests_json', 'string', []),
        getWaitingResources: Module.cwrap('rq_get_waiting_for_resources_json', 'string', []),
        getHistory:          Module.cwrap('rq_get_history_json', 'string', []),
        getCompletedList:    Module.cwrap('rq_get_completed_list_json', 'string', []),
        getLiveFeed:         Module.cwrap('rq_get_live_feed_json', 'string', []),

        searchLinear:        Module.cwrap('rq_search_requests_linear_json', 'string', ['string', 'string']),
        searchBinary:        Module.cwrap('rq_search_requests_binary_json', 'string', ['string']),
        sortBubble:          Module.cwrap('rq_sort_requests_bubble_json', 'string', ['string', 'number']),
        getReports:          Module.cwrap('rq_get_reports_json', 'string', [])
      };

      // Call C init system (loads initial sample resources & requests, performs pre-run)
      const initRes = JSON.parse(this.api.initSystem());
      console.log('✅ RELIEF-Q C Engine Initialized:', initRes);

      this.isReady = true;

      // Update UI Status Card
      if (statusCard) statusCard.classList.add('active');
      if (statusLabel) statusLabel.textContent = 'C Engine: Wasm Active';
      if (statusDetails) statusDetails.textContent = 'Authoritative C Data Structures & Algorithms';

      // Dispatch ready event
      window.dispatchEvent(new CustomEvent('wasm-ready'));
      return true;

    } catch (err) {
      console.error('❌ Failed to initialize WebAssembly engine:', err);
      if (statusLabel) statusLabel.textContent = 'Engine Offline';
      if (statusDetails) statusDetails.textContent = err.message || 'WebAssembly loading error';
      return false;
    }
  },

  /* High-Level Helper Calls returning parsed JSON objects */
  getDashboardStats() {
    return this.isReady ? JSON.parse(this.api.getDashboardStats()) : {};
  },
  getInventory() {
    return this.isReady ? JSON.parse(this.api.getInventory()) : [];
  },
  getWaitingQueue() {
    return this.isReady ? JSON.parse(this.api.getWaitingQueue()) : { count: 0, items: [] };
  },
  getPriorityQueue() {
    return this.isReady ? JSON.parse(this.api.getPriorityQueue()) : { total: 0, levels: { Critical: [], High: [], Medium: [], Low: [] } };
  },
  getAllRequests() {
    return this.isReady ? JSON.parse(this.api.getAllRequests()) : [];
  },
  getWaitingResources() {
    return this.isReady ? JSON.parse(this.api.getWaitingResources()) : [];
  },
  getHistory() {
    return this.isReady ? JSON.parse(this.api.getHistory()) : [];
  },
  getCompletedList() {
    return this.isReady ? JSON.parse(this.api.getCompletedList()) : [];
  },
  getLiveFeed() {
    return this.isReady ? JSON.parse(this.api.getLiveFeed()) : [];
  },
  getReports() {
    return this.isReady ? JSON.parse(this.api.getReports()) : {};
  },

  submitRequest(id, area, loc, disaster, pop, res, qty, priority) {
    if (!this.isReady) return { status: 'error', message: 'Engine not ready' };
    return JSON.parse(this.api.submitRequest(id, area, loc, disaster, Number(pop), res, Number(qty), priority));
  },
  classifyNext() {
    if (!this.isReady) return { status: 'error' };
    return JSON.parse(this.api.classifyNext());
  },
  classifyAll() {
    if (!this.isReady) return { status: 'error' };
    return JSON.parse(this.api.classifyAll());
  },
  allocateNext() {
    if (!this.isReady) return { status: 'error' };
    return JSON.parse(this.api.allocateNext());
  },
  allocateById(id) {
    if (!this.isReady) return { status: 'error' };
    return JSON.parse(this.api.allocateById(id));
  },
  undoLast() {
    if (!this.isReady) return { status: 'empty' };
    return JSON.parse(this.api.undoLast());
  },
  addResource(name, qty) {
    if (!this.isReady) return { status: 'error' };
    return JSON.parse(this.api.addResource(name, Number(qty)));
  },
  searchLinear(field, query) {
    if (!this.isReady) return { scanned: 0, count: 0, results: [] };
    return JSON.parse(this.api.searchLinear(field, query));
  },
  searchBinary(id) {
    if (!this.isReady) return { found: false, match: null, stepCount: 0, steps: [] };
    return JSON.parse(this.api.searchBinary(id));
  },
  sortBubble(field, ascending) {
    if (!this.isReady) return { sorted: [] };
    return JSON.parse(this.api.sortBubble(field, ascending ? 1 : 0));
  },
  reloadSampleData() {
    if (!this.isReady) return;
    return JSON.parse(this.api.loadSampleData());
  },
  resetSystem() {
    if (!this.isReady) return;
    return JSON.parse(this.api.resetSystem());
  }
};
