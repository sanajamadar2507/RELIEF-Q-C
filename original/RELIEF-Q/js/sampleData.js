/* =========================================================================
   RELIEF-Q  —  sampleData.js
   -------------------------------------------------------------------------
   Loads sample resources and sample relief requests when the app starts,
   so the examiner can see the system working immediately without having
   to type in data manually.
   ========================================================================= */

function loadSampleResources(resourceArray) {
  resourceArray.addResource("Food", 1000);
  resourceArray.addResource("Water", 1200);
  resourceArray.addResource("Medicines", 500);
  resourceArray.addResource("Blankets", 800);
  resourceArray.addResource("Tents", 300);
  resourceArray.addResource("Clothes", 600);
  resourceArray.addResource("Emergency Kits", 250);
}

function loadSampleRequests(app) {
  const sample = [
    ["R101", "Village A", "Pune",     "Flood",      500,  "Water",          300, "Critical"],
    ["R102", "Village B", "Kolhapur", "Fire",       120,  "Medicines",      80,  "High"],
    ["R103", "Village C", "Satara",   "Landslide",  200,  "Tents",          150, "Low"],
    ["R104", "Village D", "Sangli",   "Flood",      750,  "Food",           400, "Critical"],
    ["R105", "Village E", "Nashik",   "Earthquake", 300,  "Medicines",      600, "Critical"],
    ["R106", "Village F", "Pune",     "Cyclone",    180,  "Blankets",       200, "Medium"],
    ["R107", "Village G", "Satara",   "Drought",    400,  "Water",          500, "Medium"],
    ["R108", "Village H", "Sangli",   "Fire",       90,   "Emergency Kits", 60,  "High"],
    ["R109", "Village I", "Kolhapur", "Flood",      620,  "Water",          350, "Critical"],
    ["R110", "Village J", "Nashik",   "Landslide",  150,  "Clothes",        120, "Low"],
  ];

  for (const row of sample) {
    const [requestID, area, location, disasterType, population, resource, quantity, priority] = row;
    const req = new ReliefRequest(requestID, area, location, disasterType, population, resource, quantity, priority);
    app.submitRequest(req, { silent: true });
  }
}
