// CYD Family Clock - web UI served from PROGMEM.
//
// Three blobs, edited here directly (they were data/index.html, app.js and
// style.css on LittleFS until 2.10.0). Keeping the UI in flash means an
// installer Update never needs a filesystem image for it; LittleFS now holds
// only the VLW fonts. Raw-string delimiters: never write )HTML" )JS" )CSS"
// inside a blob.
#pragma once

#include <Arduino.h>

// index.html
static const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CYD World Clock - Configuration</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <header>
    <h1>CYD World Clock</h1>
    <div class="sub">Configuration & Status</div>
  </header>

  <main>
    <!-- Live Clock Display - Canvas Mirror -->
    <section class="card">
      <h2>Display Mirror</h2>
      <div id="canvasContainer">
        <canvas id="displayCanvas" width="240" height="320"></canvas>
      </div>
      <div style="display: flex; justify-content: center; align-items: center; gap: 15px; margin-top: 12px;">
        <small style="color: #888;">Pixel-accurate mirror of TFT display (updates every 2 sec)</small>
        <button type="button" class="btn-secondary" id="snapshotBtn" style="padding: 6px 12px; font-size: 0.85rem;">Capture Screenshot</button>
      </div>
    </section>

    <!-- Timezone Configuration -->
    <section class="card">
      <h2>Timezone Configuration</h2>
      <form id="configForm">
        <!-- Home City -->
        <div class="form-group">
          <h3>Home City (Reference Timezone)</h3>
          <label for="homeSelect">Select City:</label>
          <select id="homeSelect" name="homeSelect" required>
            <option value="">-- Loading timezones --</option>
          </select>
          <div id="homeCustom" style="display: none; margin-top: 10px;">
            <label for="homeCustomLabel">Custom City Name:</label>
            <input type="text" id="homeCustomLabel" placeholder="e.g., New York" style="width: 100%; margin-bottom: 5px;">
            <label for="homeCustomTz">Timezone String (POSIX):</label>
            <input type="text" id="homeCustomTz" placeholder="e.g., EST5EDT,M3.2.0,M11.1.0" style="width: 100%; font-family: monospace;">
          </div>
          <input type="hidden" id="homeLabel" name="homeLabel">
          <input type="hidden" id="homeTz" name="homeTz">
          <small id="homeTzDisplay" style="color: #00d9ff; font-family: monospace;"></small>
        </div>

        <!-- Remote Cities -->
        <div class="form-group">
          <h3>Remote Cities (5)</h3>

          <div class="city-group">
            <label>City 1:</label>
            <select id="remote0Select" name="remote0Select" required>
              <option value="">-- Loading timezones --</option>
            </select>
            <div id="remote0Custom" style="display: none; margin-top: 5px;">
              <input type="text" id="remote0CustomLabel" placeholder="City name" style="width: 48%; margin-right: 4%;">
              <input type="text" id="remote0CustomTz" placeholder="TZ string" style="width: 48%; font-family: monospace;">
            </div>
            <input type="hidden" id="remote0Label" name="remote0Label">
            <input type="hidden" id="remote0Tz" name="remote0Tz">
            <small id="remote0TzDisplay" style="color: #00d9ff; font-family: monospace;"></small>
          </div>

          <div class="city-group">
            <label>City 2:</label>
            <select id="remote1Select" name="remote1Select" required>
              <option value="">-- Loading timezones --</option>
            </select>
            <div id="remote1Custom" style="display: none; margin-top: 5px;">
              <input type="text" id="remote1CustomLabel" placeholder="City name" style="width: 48%; margin-right: 4%;">
              <input type="text" id="remote1CustomTz" placeholder="TZ string" style="width: 48%; font-family: monospace;">
            </div>
            <input type="hidden" id="remote1Label" name="remote1Label">
            <input type="hidden" id="remote1Tz" name="remote1Tz">
            <small id="remote1TzDisplay" style="color: #00d9ff; font-family: monospace;"></small>
          </div>

          <div class="city-group">
            <label>City 3:</label>
            <select id="remote2Select" name="remote2Select" required>
              <option value="">-- Loading timezones --</option>
            </select>
            <div id="remote2Custom" style="display: none; margin-top: 5px;">
              <input type="text" id="remote2CustomLabel" placeholder="City name" style="width: 48%; margin-right: 4%;">
              <input type="text" id="remote2CustomTz" placeholder="TZ string" style="width: 48%; font-family: monospace;">
            </div>
            <input type="hidden" id="remote2Label" name="remote2Label">
            <input type="hidden" id="remote2Tz" name="remote2Tz">
            <small id="remote2TzDisplay" style="color: #00d9ff; font-family: monospace;"></small>
          </div>

          <div class="city-group">
            <label>City 4:</label>
            <select id="remote3Select" name="remote3Select" required>
              <option value="">-- Loading timezones --</option>
            </select>
            <div id="remote3Custom" style="display: none; margin-top: 5px;">
              <input type="text" id="remote3CustomLabel" placeholder="City name" style="width: 48%; margin-right: 4%;">
              <input type="text" id="remote3CustomTz" placeholder="TZ string" style="width: 48%; font-family: monospace;">
            </div>
            <input type="hidden" id="remote3Label" name="remote3Label">
            <input type="hidden" id="remote3Tz" name="remote3Tz">
            <small id="remote3TzDisplay" style="color: #00d9ff; font-family: monospace;"></small>
          </div>

          <div class="city-group">
            <label>City 5:</label>
            <select id="remote4Select" name="remote4Select" required>
              <option value="">-- Loading timezones --</option>
            </select>
            <div id="remote4Custom" style="display: none; margin-top: 5px;">
              <input type="text" id="remote4CustomLabel" placeholder="City name" style="width: 48%; margin-right: 4%;">
              <input type="text" id="remote4CustomTz" placeholder="TZ string" style="width: 48%; font-family: monospace;">
            </div>
            <input type="hidden" id="remote4Label" name="remote4Label">
            <input type="hidden" id="remote4Tz" name="remote4Tz">
            <small id="remote4TzDisplay" style="color: #00d9ff; font-family: monospace;"></small>
          </div>
        </div>

        <div class="form-actions">
          <button type="submit" class="btn-primary">Save Configuration</button>
          <button type="button" class="btn-secondary" id="reloadBtn">Reload from Device</button>
        </div>
      </form>
    </section>

    <!-- System Actions -->
    <section class="card">
      <h2>System Actions</h2>
      <div class="form-actions">
        <button type="button" class="btn-secondary" id="rebootBtn">Reboot Device</button>
        <button type="button" class="btn-warning" id="resetWifiBtn">Reset WiFi Credentials</button>
      </div>
      <small>Note: Reset WiFi will erase stored credentials and reboot the device into AP mode.</small>
    </section>

    <!-- System Status -->
    <section class="card">
      <h2>System Status</h2>
      <div class="status-grid">
        <div class="status-item">
          <span class="label">Firmware:</span>
          <span id="firmware">--</span>
        </div>
        <div class="status-item">
          <span class="label">Hostname:</span>
          <span id="hostname">--</span>
        </div>
        <div class="status-item">
          <span class="label">WiFi SSID:</span>
          <span id="wifi_ssid">--</span>
        </div>
        <div class="status-item">
          <span class="label">IP Address:</span>
          <span id="wifi_ip">--</span>
        </div>
        <div class="status-item">
          <span class="label">Signal:</span>
          <span id="wifi_rssi">--</span>
        </div>
        <div class="status-item">
          <span class="label">Uptime:</span>
          <span id="uptime">--</span>
        </div>
        <div class="status-item">
          <span class="label">Free Heap:</span>
          <span id="freeHeap">--</span>
        </div>
        <div class="status-item">
          <span class="label">LDR Value:</span>
          <span id="ldrValue">--</span>
        </div>
        <div class="status-item">
          <span class="label">Sensor:</span>
          <span id="sensorType">--</span>
        </div>
        <div class="status-item">
          <span class="label">Temperature:</span>
          <span id="temperature">--</span>
          <label style="margin-left: 10px; cursor: pointer;">
            <input type="checkbox" id="useFahrenheit" style="margin-right: 4px;">
            Use °F
          </label>
        </div>
        <div class="status-item">
          <span class="label">Humidity:</span>
          <span id="humidity">--</span>
        </div>
        <div class="status-item">
          <span class="label">Pressure:</span>
          <span id="pressure">--</span>
        </div>
        <div class="status-item">
          <span class="label">Display Mode:</span>
          <select id="displayMode" style="padding: 2px 5px;">
            <option value="portrait">Portrait (240x320)</option>
            <option value="landscape">Landscape (320x240)</option>
          </select>
          <label style="margin-left: 10px; cursor: pointer;">
            <input type="checkbox" id="flipDisplay" style="margin-right: 4px;">
            Flip 180°
          </label>
        </div>
        <div class="status-item">
          <span class="label">Screen Rotation:</span>
          <label style="cursor: pointer; margin-right: 10px;">
            <input type="checkbox" id="enableScreenRotation" style="margin-right: 4px;">
            Enable (Portrait+Sensor)
          </label>
          <label style="font-size: 0.9rem;">
            <span style="margin-right: 4px;">Interval:</span>
            <input type="number" id="screenFlipInterval" min="3" max="30" style="width: 50px; padding: 2px 4px;">
            <span style="margin-left: 4px;">sec</span>
          </label>
        </div>
        <div class="status-item">
          <span class="label">Debug Level:</span>
          <select id="debugLevel" style="padding: 2px 5px;">
            <option value="0">0 - Off</option>
            <option value="1">1 - Error</option>
            <option value="2">2 - Warn</option>
            <option value="3">3 - Info</option>
            <option value="4">4 - Verbose</option>
          </select>
        </div>
      </div>
      <small style="display: block; margin-top: 10px;">Touch the screen to view diagnostics. Status updates every 5 seconds.</small>
    </section>
  </main>

  <footer style="margin-top: 30px; padding: 20px; background: #0f1419; border-radius: 10px; text-align: center;">
    <div style="display: flex; justify-content: center; gap: 30px; margin-bottom: 15px; flex-wrap: wrap;">
      <a href="https://github.com/anthonyjclarke/CYD_Family_Clock" target="_blank" style="color: #00d9ff; text-decoration: none; display: flex; align-items: center; gap: 8px;">
        <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
          <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/>
        </svg>
        Project Repository
      </a>
      <a href="https://bsky.app/profile/anthonyjclarke.bsky.social" target="_blank" style="color: #00d9ff; text-decoration: none; display: flex; align-items: center; gap: 8px;">
        <svg width="24" height="24" viewBox="0 0 600 530" fill="currentColor">
          <path d="m135.72 44.03c66.496 49.921 138.02 151.14 164.28 205.46 26.262-54.316 97.782-155.54 164.28-205.46 47.98-36.021 125.72-63.892 125.72 24.795 0 17.712-10.155 148.79-16.111 170.07-20.703 73.984-96.144 92.854-163.25 81.433 117.3 19.964 147.14 86.092 82.697 152.22-122.39 125.59-175.91-31.511-189.63-71.766-2.514-7.3797-3.6904-10.832-3.7077-7.8964-0.0174-2.9357-1.1937 0.51669-3.7077 7.8964-13.714 40.255-67.233 197.36-189.63 71.766-64.444-66.128-34.605-132.26 82.697-152.22-67.108 11.421-142.55-7.4491-163.25-81.433-5.9562-21.282-16.111-152.36-16.111-170.07 0-88.687 77.742-60.816 125.72-24.795z"/>
        </svg>
        Bluesky
      </a>
    </div>
    <p style="margin: 0; color: #888; font-size: 0.9rem;">
      Software by <strong style="color: #00d9ff;">Anthony Clarke</strong> with help from <a href="https://claude.ai" target="_blank" style="color: #00d9ff; text-decoration: none;"><strong>Claude.ai</strong></a>
    </p>
  </footer>

  <script src="/app.js"></script>
</body>
</html>
)HTML";

// app.js
static const char APP_JS[] PROGMEM = R"JS(// CYD World Clock - Web UI JavaScript

// Global timezone list
let timezones = [];

// Load state on page load
document.addEventListener('DOMContentLoaded', async () => {
  // Set up event listeners first (before any async operations)
  document.getElementById('configForm').addEventListener('submit', handleSaveConfig);
  document.getElementById('reloadBtn').addEventListener('click', loadState);
  document.getElementById('rebootBtn').addEventListener('click', handleReboot);
  document.getElementById('resetWifiBtn').addEventListener('click', handleResetWiFi);
  document.getElementById('debugLevel').addEventListener('change', handleDebugLevelChange);
  document.getElementById('displayMode').addEventListener('change', handleDisplayModeChange);
  document.getElementById('flipDisplay').addEventListener('change', handleFlipDisplayChange);
  document.getElementById('useFahrenheit').addEventListener('change', handleUseFahrenheitChange);
  document.getElementById('enableScreenRotation').addEventListener('change', handleScreenRotationChange);
  document.getElementById('screenFlipInterval').addEventListener('change', handleFlipIntervalChange);
  document.getElementById('snapshotBtn').addEventListener('click', handleSnapshot);

  // Timezone dropdown change listeners
  document.getElementById('homeSelect').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote0Select').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote1Select').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote2Select').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote3Select').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote4Select').addEventListener('change', updateTimezoneFields);

  // Load timezones FIRST, then load state (must be in order)
  // This prevents race condition where loadState runs before dropdowns are populated
  await loadTimezones();
  await loadState();

  // Start polling AFTER initial load is complete
  startPolling();
});

// Polling function with timeout-based approach (non-blocking)
// Only updates status and mirror - does NOT touch form fields to preserve user edits
async function tick() {
  try {
    await updateStatus();
    await updateMirror();
  } catch (e) {
    console.warn('Polling error:', e);
  } finally {
    setTimeout(tick, 2000);  // Poll every 2 seconds for responsive display mirror
  }
}

function startPolling() {
  tick();
}

// Load timezone list from device
async function loadTimezones() {
  try {
    const response = await fetch('/api/timezones');
    if (!response.ok) throw new Error('Failed to fetch timezones');

    timezones = await response.json();
    console.log(`Loaded ${timezones.length} timezones`);

    // Populate all dropdowns
    populateTimezoneDropdown('homeSelect');
    populateTimezoneDropdown('remote0Select');
    populateTimezoneDropdown('remote1Select');
    populateTimezoneDropdown('remote2Select');
    populateTimezoneDropdown('remote3Select');
    populateTimezoneDropdown('remote4Select');
  } catch (error) {
    console.error('Error loading timezones:', error);
    showNotification('Error loading timezone list', 'error');
  }
}

// Populate a specific dropdown with timezone options grouped by region
function populateTimezoneDropdown(selectId) {
  const select = document.getElementById(selectId);
  select.innerHTML = '<option value="">-- Select a city --</option>';

  // Define region ranges based on timezones.h structure
  const regions = [
    { name: 'Australia & Oceania', start: 0, end: 11 },
    { name: 'North America', start: 12, end: 28 },
    { name: 'South America', start: 29, end: 34 },
    { name: 'Western Europe', start: 35, end: 45 },
    { name: 'Northern Europe', start: 46, end: 49 },
    { name: 'Central & Eastern Europe', start: 50, end: 57 },
    { name: 'Middle East', start: 58, end: 62 },
    { name: 'South Asia', start: 63, end: 69 },
    { name: 'Southeast Asia', start: 70, end: 76 },
    { name: 'East Asia', start: 77, end: 82 },
    { name: 'Central Asia', start: 83, end: 85 },
    { name: 'Caucasus', start: 86, end: 88 },
    { name: 'Africa', start: 89, end: 101 }
  ];

  regions.forEach(region => {
    const optgroup = document.createElement('optgroup');
    optgroup.label = region.name;

    for (let i = region.start; i <= region.end && i < timezones.length; i++) {
      const tz = timezones[i];
      const option = document.createElement('option');
      option.value = i;
      option.textContent = tz.name;
      option.dataset.tz = tz.tz;
      optgroup.appendChild(option);
    }

    select.appendChild(optgroup);
  });

  // Add "Custom Timezone" option at the end
  const customOption = document.createElement('option');
  customOption.value = 'custom';
  customOption.textContent = '-- Custom Timezone --';
  select.appendChild(customOption);
}

// Update hidden fields and display when dropdown changes
function updateTimezoneFields(event) {
  const selectId = event.target.id;
  const select = event.target;
  const selectedIndex = select.value;
  const prefix = selectId.replace('Select', '');

  if (selectedIndex === '') return;

  // Check if custom option selected
  if (selectedIndex === 'custom') {
    // Show custom input fields
    const customDiv = document.getElementById(prefix + 'Custom');
    customDiv.style.display = 'block';

    // Set up listeners for custom inputs
    const customLabel = document.getElementById(prefix + 'CustomLabel');
    const customTz = document.getElementById(prefix + 'CustomTz');

    const updateCustom = () => {
      document.getElementById(prefix + 'Label').value = customLabel.value || 'Custom';
      document.getElementById(prefix + 'Tz').value = customTz.value;
      document.getElementById(prefix + 'TzDisplay').textContent = customTz.value || '(enter timezone string)';
    };

    customLabel.addEventListener('input', updateCustom);
    customTz.addEventListener('input', updateCustom);

    // Initialize with current values if any
    updateCustom();
  } else {
    // Hide custom input fields
    const customDiv = document.getElementById(prefix + 'Custom');
    if (customDiv) customDiv.style.display = 'none';

    // Use predefined timezone
    const tz = timezones[selectedIndex];

    // Update hidden fields
    document.getElementById(prefix + 'Label').value = tz.name;
    document.getElementById(prefix + 'Tz').value = tz.tz;

    // Update display
    const display = document.getElementById(prefix + 'TzDisplay');
    display.textContent = tz.tz;
  }
}

// Set dropdown selection based on label and timezone string
function setTimezoneDropdown(prefix, label, tz) {
  // Try to find exact match by name and TZ
  let index = timezones.findIndex(t => t.name === label && t.tz === tz);

  // If no exact match, try matching just by city name (strip country)
  if (index === -1) {
    const cityOnly = label.split(',')[0].trim();
    index = timezones.findIndex(t => {
      const tzCity = t.name.split(',')[0].trim();
      return tzCity === cityOnly && t.tz === tz;
    });
  }

  // If still no match, try matching just by timezone string
  if (index === -1) {
    index = timezones.findIndex(t => t.tz === tz);
  }

  if (index !== -1) {
    // Found a match - use the timezone from the list
    const matchedTz = timezones[index];
    document.getElementById(prefix + 'Select').value = index;
    document.getElementById(prefix + 'Label').value = matchedTz.name;
    document.getElementById(prefix + 'Tz').value = matchedTz.tz;
    document.getElementById(prefix + 'TzDisplay').textContent = matchedTz.tz;
    // Hide custom fields
    const customDiv = document.getElementById(prefix + 'Custom');
    if (customDiv) customDiv.style.display = 'none';
  } else {
    // No match found - use custom mode
    document.getElementById(prefix + 'Select').value = 'custom';
    document.getElementById(prefix + 'Label').value = label;
    document.getElementById(prefix + 'Tz').value = tz;
    document.getElementById(prefix + 'TzDisplay').textContent = tz + ' (custom)';

    // Show and populate custom input fields
    const customDiv = document.getElementById(prefix + 'Custom');
    if (customDiv) {
      customDiv.style.display = 'block';
      document.getElementById(prefix + 'CustomLabel').value = label;
      document.getElementById(prefix + 'CustomTz').value = tz;
    }

    console.log(`Using custom timezone: ${label} - ${tz}`);
  }
}

// Update status display only (called during polling - doesn't touch form fields)
async function updateStatus() {
  try {
    const response = await fetch('/api/state');
    if (!response.ok) throw new Error('Failed to fetch state');

    const data = await response.json();

    // Update status display only - NOT form fields
    document.getElementById('firmware').textContent = data.firmware || '--';
    document.getElementById('hostname').textContent = data.hostname || '--';
    document.getElementById('wifi_ssid').textContent = data.wifi_ssid || '--';
    document.getElementById('wifi_ip').textContent = data.wifi_ip || '--';
    document.getElementById('wifi_rssi').textContent = (data.wifi_rssi || '--') + ' dBm';
    document.getElementById('uptime').textContent = formatUptime(data.uptime || 0);
    document.getElementById('freeHeap').textContent = formatBytes(data.freeHeap || 0);
    document.getElementById('ldrValue').textContent = data.ldrValue !== undefined ? data.ldrValue : '--';

    // Sensor data
    document.getElementById('sensorType').textContent = data.sensorAvailable ? data.sensorType : 'N/A';
    const tempUnit = data.useFahrenheit ? ' °F' : ' °C';
    document.getElementById('temperature').textContent = data.sensorAvailable && data.temperature !== undefined ? data.temperature + tempUnit : 'N/A';
    document.getElementById('humidity').textContent = data.humidity !== undefined ? data.humidity + ' %' : 'N/A';
    document.getElementById('pressure').textContent = data.pressure !== undefined ? data.pressure + ' hPa' : 'N/A';
    document.getElementById('useFahrenheit').checked = data.useFahrenheit || false;

    document.getElementById('displayMode').value = data.landscapeMode ? 'landscape' : 'portrait';
    document.getElementById('flipDisplay').checked = data.flipDisplay || false;
    document.getElementById('enableScreenRotation').checked = data.enableScreenRotation !== undefined ? data.enableScreenRotation : true;
    document.getElementById('screenFlipInterval').value = data.screenFlipInterval || 8;
    document.getElementById('debugLevel').value = data.debugLevel || 3;
  } catch (error) {
    console.error('Error updating status:', error);
  }
}

// Load full state including form fields (called on initial load and "Reload from Device")
async function loadState() {
  try {
    const response = await fetch('/api/state');
    if (!response.ok) throw new Error('Failed to fetch state');

    const data = await response.json();

    // Update status display
    document.getElementById('firmware').textContent = data.firmware || '--';
    document.getElementById('hostname').textContent = data.hostname || '--';
    document.getElementById('wifi_ssid').textContent = data.wifi_ssid || '--';
    document.getElementById('wifi_ip').textContent = data.wifi_ip || '--';
    document.getElementById('wifi_rssi').textContent = (data.wifi_rssi || '--') + ' dBm';
    document.getElementById('uptime').textContent = formatUptime(data.uptime || 0);
    document.getElementById('freeHeap').textContent = formatBytes(data.freeHeap || 0);
    document.getElementById('ldrValue').textContent = data.ldrValue !== undefined ? data.ldrValue : '--';

    // Sensor data
    document.getElementById('sensorType').textContent = data.sensorAvailable ? data.sensorType : 'N/A';
    const tempUnit = data.useFahrenheit ? ' °F' : ' °C';
    document.getElementById('temperature').textContent = data.sensorAvailable && data.temperature !== undefined ? data.temperature + tempUnit : 'N/A';
    document.getElementById('humidity').textContent = data.humidity !== undefined ? data.humidity + ' %' : 'N/A';
    document.getElementById('pressure').textContent = data.pressure !== undefined ? data.pressure + ' hPa' : 'N/A';
    document.getElementById('useFahrenheit').checked = data.useFahrenheit || false;

    document.getElementById('displayMode').value = data.landscapeMode ? 'landscape' : 'portrait';
    document.getElementById('flipDisplay').checked = data.flipDisplay || false;
    document.getElementById('enableScreenRotation').checked = data.enableScreenRotation !== undefined ? data.enableScreenRotation : true;
    document.getElementById('screenFlipInterval').value = data.screenFlipInterval || 8;
    document.getElementById('debugLevel').value = data.debugLevel || 3;

    // Update form fields (only on explicit load, not during polling)
    if (data.homeCity) {
      setTimezoneDropdown('home', data.homeCity.label, data.homeCity.tz);
    }

    if (data.remoteCities && data.remoteCities.length >= 5) {
      for (let i = 0; i < 5; i++) {
        setTimezoneDropdown(`remote${i}`, data.remoteCities[i].label, data.remoteCities[i].tz);
      }
    }

    console.log('State loaded successfully');
  } catch (error) {
    console.error('Error loading state:', error);
    showNotification('Error loading configuration from device', 'error');
  }
}

// Handle configuration save
async function handleSaveConfig(event) {
  event.preventDefault();

  const formData = new FormData(event.target);

  // Build config JSON
  const config = {
    homeCity: {
      label: formData.get('homeLabel'),
      tz: formData.get('homeTz')
    },
    remoteCities: [
      {
        label: formData.get('remote0Label'),
        tz: formData.get('remote0Tz')
      },
      {
        label: formData.get('remote1Label'),
        tz: formData.get('remote1Tz')
      },
      {
        label: formData.get('remote2Label'),
        tz: formData.get('remote2Tz')
      },
      {
        label: formData.get('remote3Label'),
        tz: formData.get('remote3Tz')
      },
      {
        label: formData.get('remote4Label'),
        tz: formData.get('remote4Tz')
      }
    ]
  };

  try {
    const response = await fetch('/api/config', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(config)
    });

    if (!response.ok) throw new Error('Failed to save configuration');

    const result = await response.text();
    showNotification('Configuration saved! Changes applied immediately.', 'success');
    console.log('Config saved:', result);
  } catch (error) {
    console.error('Error saving config:', error);
    showNotification('Error saving configuration', 'error');
  }
}

// Handle device reboot
async function handleReboot() {
  if (!confirm('Are you sure you want to reboot the device?')) {
    return;
  }

  try {
    const response = await fetch('/api/reboot', {
      method: 'POST'
    });

    if (!response.ok) throw new Error('Failed to reboot device');

    showNotification('Device is rebooting...', 'warning');
    console.log('Reboot initiated');

    // Disable UI while device reboots
    setTimeout(() => {
      document.body.innerHTML = '<div style="text-align:center; padding:50px;"><h1>Device Rebooting...</h1><p>Please wait 10-15 seconds, then refresh this page.</p></div>';
    }, 2000);
  } catch (error) {
    console.error('Error rebooting device:', error);
    showNotification('Error rebooting device', 'error');
  }
}

// Handle WiFi reset
async function handleResetWiFi() {
  if (!confirm('Are you sure you want to reset WiFi credentials? The device will reboot into AP mode.')) {
    return;
  }

  try {
    const response = await fetch('/api/reset-wifi', {
      method: 'POST'
    });

    if (!response.ok) throw new Error('Failed to reset WiFi');

    showNotification('WiFi credentials reset. Device is rebooting...', 'warning');
    console.log('WiFi reset initiated');

    // Disable UI while device reboots
    setTimeout(() => {
      document.body.innerHTML = '<div style="text-align:center; padding:50px;"><h1>Device Rebooting...</h1><p>Reconnect to the AP "WorldClock-Setup" to reconfigure WiFi.</p></div>';
    }, 2000);
  } catch (error) {
    console.error('Error resetting WiFi:', error);
    showNotification('Error resetting WiFi credentials', 'error');
  }
}

// Handle screenshot capture - downloads BMP image from TFT display
function handleSnapshot() {
  const btn = document.getElementById('snapshotBtn');
  const originalText = btn.textContent;

  // Show loading state
  btn.textContent = 'Capturing...';
  btn.disabled = true;

  // Create a hidden link to trigger download
  const link = document.createElement('a');
  link.href = '/api/snapshot';
  link.download = 'clock_snapshot.bmp';
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);

  // Reset button after a delay (download starts in background)
  setTimeout(() => {
    btn.textContent = originalText;
    btn.disabled = false;
    showNotification('Screenshot captured!', 'success');
  }, 2000);
}

// Handle debug level change
async function handleDebugLevelChange(event) {
  const level = parseInt(event.target.value);

  try {
    const response = await fetch('/api/debug-level', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ level: level })
    });

    if (!response.ok) throw new Error('Failed to set debug level');

    const levelNames = ['Off', 'Error', 'Warn', 'Info', 'Verbose'];
    showNotification(`Debug level set to ${level} (${levelNames[level]})`, 'success');
    console.log('Debug level changed to:', level);
  } catch (error) {
    console.error('Error setting debug level:', error);
    showNotification('Error setting debug level', 'error');
    loadState(); // Reload to restore actual value
  }
}

// Handle display mode change (portrait/landscape)
async function handleDisplayModeChange(event) {
  const isLandscape = event.target.value === 'landscape';

  try {
    const response = await fetch('/api/config', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ landscapeMode: isLandscape })
    });

    if (!response.ok) throw new Error('Failed to set display mode');

    showNotification(`Display mode changed to ${event.target.value}`, 'success');
    console.log('Display mode changed to:', event.target.value);
  } catch (error) {
    console.error('Error setting display mode:', error);
    showNotification('Error setting display mode', 'error');
    loadState(); // Reload to restore actual value
  }
}

// Handle flip display change (180° rotation)
async function handleFlipDisplayChange(event) {
  const isFlipped = event.target.checked;

  try {
    const response = await fetch('/api/config', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ flipDisplay: isFlipped })
    });

    if (!response.ok) throw new Error('Failed to set flip display');

    showNotification(`Display ${isFlipped ? 'flipped' : 'normal'}`, 'success');
    console.log('Flip display:', isFlipped);
  } catch (error) {
    console.error('Error setting flip display:', error);
    showNotification('Error setting flip display', 'error');
    loadState(); // Reload to restore actual value
  }
}

// Handle temperature unit change (Celsius/Fahrenheit)
async function handleUseFahrenheitChange(event) {
  const useFahrenheit = event.target.checked;

  try {
    const response = await fetch('/api/config', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ useFahrenheit: useFahrenheit })
    });

    if (!response.ok) throw new Error('Failed to set temperature unit');

    showNotification(`Temperature unit: ${useFahrenheit ? '°F' : '°C'}`, 'success');
    console.log('Use Fahrenheit:', useFahrenheit);
  } catch (error) {
    console.error('Error setting temperature unit:', error);
    showNotification('Error setting temperature unit', 'error');
    loadState(); // Reload to restore actual value
  }
}

// Handle screen rotation enable/disable
async function handleScreenRotationChange(event) {
  const enabled = event.target.checked;

  try {
    const response = await fetch('/api/config', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ enableScreenRotation: enabled })
    });

    if (!response.ok) throw new Error('Failed to set screen rotation');

    showNotification(`Screen rotation ${enabled ? 'enabled' : 'disabled'}`, 'success');
    console.log('Screen rotation:', enabled);
  } catch (error) {
    console.error('Error setting screen rotation:', error);
    showNotification('Error setting screen rotation', 'error');
    loadState(); // Reload to restore actual value
  }
}

// Handle screen flip interval change
async function handleFlipIntervalChange(event) {
  const interval = parseInt(event.target.value);

  // Validate range
  if (interval < 3 || interval > 30) {
    showNotification('Interval must be between 3 and 30 seconds', 'error');
    loadState(); // Reload to restore actual value
    return;
  }

  try {
    const response = await fetch('/api/config', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ screenFlipInterval: interval })
    });

    if (!response.ok) throw new Error('Failed to set flip interval');

    showNotification(`Flip interval: ${interval} seconds`, 'success');
    console.log('Flip interval:', interval);
  } catch (error) {
    console.error('Error setting flip interval:', error);
    showNotification('Error setting flip interval', 'error');
    loadState(); // Reload to restore actual value
  }
}

// Helper: Format bytes in human-readable format
function formatBytes(bytes) {
  if (bytes >= 1024 * 1024) {
    return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
  } else if (bytes >= 1024) {
    return (bytes / 1024).toFixed(1) + ' KB';
  } else {
    return bytes + ' B';
  }
}

// Helper: Format uptime in human-readable format
function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;

  if (days > 0) {
    return `${days}d ${hours}h ${minutes}m`;
  } else if (hours > 0) {
    return `${hours}h ${minutes}m ${secs}s`;
  } else if (minutes > 0) {
    return `${minutes}m ${secs}s`;
  } else {
    return `${secs}s`;
  }
}

// Helper: Show notification (simple alert for now)
function showNotification(message, type) {
  // Simple implementation - could be enhanced with a toast library
  const color = type === 'error' ? 'red' : type === 'warning' ? 'orange' : 'green';
  console.log(`[${type.toUpperCase()}] ${message}`);

  // Create a simple notification div
  const notification = document.createElement('div');
  notification.style.cssText = `
    position: fixed;
    top: 20px;
    right: 20px;
    padding: 15px 20px;
    background-color: ${color};
    color: white;
    border-radius: 5px;
    box-shadow: 0 2px 10px rgba(0,0,0,0.3);
    z-index: 1000;
    max-width: 300px;
  `;
  notification.textContent = message;
  document.body.appendChild(notification);

  // Remove after 5 seconds
  setTimeout(() => {
    notification.remove();
  }, 5000);
}

// ====================================
// Canvas Display Mirror - Pixel-accurate TFT recreation
// ====================================

// Layout constants matching main.cpp
const LAYOUT = {
  // Portrait mode: 240x320
  portrait: {
    width: 240,
    height: 320,
    titleHeight: 22,
    dateHeight: 18,
    headerHeight: 40,  // titleHeight + dateHeight
    pad: 8,
    cityRowHeight: 46.7  // (320 - 40) / 6 = 46.67
  },
  // Landscape mode: 320x240
  landscape: {
    width: 320,
    height: 240,
    leftPanelWidth: 120,
    rightPanelWidth: 200,
    remoteRowHeight: 48,  // 240 / 5 = 48
    clock: {
      centerX: 60,
      centerY: 120,  // Centered between date (y~60) and digital time (y=185)
      radius: 50,
      hourHandLen: 25,
      minuteHandLen: 35,
      secondHandLen: 40
    }
  },
  // Colors (matching TFT colors)
  colors: {
    bg: '#000000',
    label: '#FFFFFF',
    time: '#00FF00',
    home: '#00FFFF',
    prevDay: '#FFFF00',
    nextDay: '#00FFFF',
    title: '#00FFFF',
    clockFace: '#404040',
    hourMarker: '#FFFFFF',
    hourHand: '#FFFFFF',
    minuteHand: '#FFFFFF',
    secondHand: '#FF0000',
    envData: '#C0C0C0'  // Light grey for environmental data (TFT_LIGHTGREY)
  },
  // Display scale factor for WebUI (larger screens get scaled up)
  scale: window.innerWidth >= 600 ? 1.5 : 1.0
};

// Canvas context (initialized on first render)
let ctx = null;
let canvas = null;

// Fetch clock data from ESP32
async function fetchClock() {
  try {
    const response = await fetch('/api/mirror', { cache: 'no-store' });
    return await response.json();
  } catch (e) {
    console.warn('Clock fetch error:', e);
    return null;
  }
}

// Initialize canvas with correct dimensions (scaled for visibility)
function initCanvas(isLandscape) {
  canvas = document.getElementById('displayCanvas');
  if (!canvas) return false;

  const scale = LAYOUT.scale;
  const baseWidth = isLandscape ? LAYOUT.landscape.width : LAYOUT.portrait.width;
  const baseHeight = isLandscape ? LAYOUT.landscape.height : LAYOUT.portrait.height;
  const width = Math.round(baseWidth * scale);
  const height = Math.round(baseHeight * scale);

  if (canvas.width !== width || canvas.height !== height) {
    canvas.width = width;
    canvas.height = height;
  }

  ctx = canvas.getContext('2d');
  // Apply scale transform so drawing code uses original coordinates
  ctx.setTransform(scale, 0, 0, scale, 0, 0);
  return true;
}

// Clear canvas with background color (uses original coordinates due to scale transform)
function clearCanvas(isLandscape) {
  const width = isLandscape ? LAYOUT.landscape.width : LAYOUT.portrait.width;
  const height = isLandscape ? LAYOUT.landscape.height : LAYOUT.portrait.height;
  ctx.fillStyle = LAYOUT.colors.bg;
  ctx.fillRect(0, 0, width, height);
}

// Draw text with specified font and color
function drawText(text, x, y, color, fontSize, fontWeight = 'bold', align = 'left') {
  ctx.fillStyle = color;
  ctx.font = `${fontWeight} ${fontSize}px "Courier New", monospace`;
  ctx.textAlign = align;
  ctx.textBaseline = 'top';
  ctx.fillText(text, x, y);
}

// Render portrait mode display
function renderPortrait(data) {
  const L = LAYOUT.portrait;
  const C = LAYOUT.colors;

  clearCanvas(false);

  // Header: "WORLD CLOCK" title
  drawText('WORLD CLOCK', L.width / 2, 4, C.title, 16, 'bold', 'center');

  // Header: Date
  drawText(data.date || '--', L.width / 2, L.titleHeight + 2, C.time, 14, 'bold', 'center');

  // Draw separator line under header
  ctx.strokeStyle = '#333333';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(0, L.headerHeight);
  ctx.lineTo(L.width, L.headerHeight);
  ctx.stroke();

  // Home city (index 0)
  const homeY = L.headerHeight + 4;
  drawText(data.home?.label || '--', L.pad, homeY, C.label, 14, 'bold');
  drawText(data.home?.time || '--:--', L.width - L.pad, homeY, C.time, 22, 'bold', 'right');
  drawText('HOME', L.pad, homeY + 24, C.home, 10, 'normal');
  if (data.home?.prevDay) {
    drawText('PREV DAY', L.pad + 40, homeY + 24, C.prevDay, 10, 'normal');
  } else if (data.home?.nextDay) {
    drawText('NEXT DAY', L.pad + 40, homeY + 24, C.nextDay, 10, 'normal');
  }

  // Remote cities (5 cities)
  if (data.remote && data.remote.length > 0) {
    data.remote.forEach((city, i) => {
      const rowY = L.headerHeight + L.cityRowHeight * (i + 1) + 4;

      // Separator line
      ctx.strokeStyle = '#222222';
      ctx.beginPath();
      ctx.moveTo(0, rowY - 4);
      ctx.lineTo(L.width, rowY - 4);
      ctx.stroke();

      // City label
      drawText(city.label || '--', L.pad, rowY, C.label, 14, 'bold');

      // Time
      drawText(city.time || '--:--', L.width - L.pad, rowY, C.time, 22, 'bold', 'right');

      // Prev day or Next day indicator
      if (city.prevDay) {
        drawText('PREV DAY', L.pad, rowY + 24, C.prevDay, 10, 'normal');
      } else if (city.nextDay) {
        drawText('NEXT DAY', L.pad, rowY + 24, C.nextDay, 10, 'normal');
      }
    });
  }
}

// Render alternate portrait mode display (with analogue clock)
function renderAlternatePortrait(data) {
  const L = LAYOUT.portrait;
  const C = LAYOUT.colors;
  const clockCenterX = 120;
  const clockCenterY = 90;  // Shifted up from 110
  const clockRadius = 55;

  clearCanvas(false);

  // Header: "WORLD CLOCK" title
  drawText('WORLD CLOCK', L.width / 2, 4, C.title, 16, 'bold', 'center');

  // Header: Date
  drawText(data.date || '--', L.width / 2, 22, C.time, 14, 'bold', 'center');

  // Draw analogue clock face
  ctx.strokeStyle = C.clockFace;
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.arc(clockCenterX, clockCenterY, clockRadius, 0, Math.PI * 2);
  ctx.stroke();

  // Hour markers (12 positions)
  for (let i = 0; i < 12; i++) {
    const angleDeg = i * 30;
    const angleRad = (angleDeg - 90) * Math.PI / 180;
    const outerR = clockRadius - 3;
    const innerR = clockRadius - 8;

    const x1 = clockCenterX + innerR * Math.cos(angleRad);
    const y1 = clockCenterY + innerR * Math.sin(angleRad);
    const x2 = clockCenterX + outerR * Math.cos(angleRad);
    const y2 = clockCenterY + outerR * Math.sin(angleRad);

    ctx.strokeStyle = C.hourMarker;
    ctx.lineWidth = (i % 3 === 0) ? 2 : 1;
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
  }

  // Draw clock hands
  if (data.clock) {
    const hourAngle = (data.clock.hour % 12) * 30 + data.clock.minute * 0.5;
    const minuteAngle = data.clock.minute * 6;
    const secondAngle = data.clock.second * 6;

    drawClockHand(28, hourAngle, C.hourHand, 3);
    drawClockHand(40, minuteAngle, C.minuteHand, 2);
    drawClockHand(45, secondAngle, C.secondHand, 1);

    // Center dot
    ctx.fillStyle = C.hourMarker;
    ctx.beginPath();
    ctx.arc(clockCenterX, clockCenterY, 3, 0, Math.PI * 2);
    ctx.fill();
  }

  // Helper function for clock hands (scoped to alternate portrait)
  function drawClockHand(length, angleDeg, color, thickness) {
    const angleRad = (angleDeg - 90) * Math.PI / 180;
    const x2 = clockCenterX + length * Math.cos(angleRad);
    const y2 = clockCenterY + length * Math.sin(angleRad);

    ctx.strokeStyle = color;
    ctx.lineWidth = thickness;
    ctx.lineCap = 'round';
    ctx.beginPath();
    ctx.moveTo(clockCenterX, clockCenterY);
    ctx.lineTo(x2, y2);
    ctx.stroke();
  }

  // Home city name
  drawText(data.home?.label || '--', L.width / 2, 150, C.label, 14, 'bold', 'center');

  // HOME indicator
  drawText('HOME', L.width / 2, 163, C.home, 10, 'normal', 'center');

  // Digital time (smaller font)
  drawText(data.home?.time || '--:--', L.width / 2, 176, C.time, 14, 'bold', 'center');

  // Prev/Next day indicator
  if (data.home?.prevDay) {
    drawText('PREV DAY', L.width / 2, 210, C.prevDay, 9, 'normal', 'center');
  } else if (data.home?.nextDay) {
    drawText('NEXT DAY', L.width / 2, 210, C.nextDay, 9, 'normal', 'center');
  }

  // Environmental data
  if (data.sensorAvailable && data.envData) {
    drawText(data.envData, L.width / 2, 196, C.envData, 10, 'normal', 'center');
  }

  // Draw separator line before remote cities
  ctx.strokeStyle = '#333333';
  ctx.beginPath();
  ctx.moveTo(0, 210);
  ctx.lineTo(L.width, 210);
  ctx.stroke();

  // Remote cities (compact format)
  if (data.remote && data.remote.length > 0) {
    data.remote.forEach((city, i) => {
      const rowY = 210 + (i * 17);

      // Separator lines between cities
      if (i > 0) {
        ctx.strokeStyle = '#222222';
        ctx.beginPath();
        ctx.moveTo(0, rowY);
        ctx.lineTo(L.width, rowY);
        ctx.stroke();
      }

      // City name (left, small font)
      drawText(city.label || '--', L.pad, rowY + 2, C.label, 9, 'bold');

      // Time (right, medium font)
      drawText(city.time || '--:--', L.width - L.pad, rowY + 2, C.time, 14, 'bold', 'right');

      // Prev/Next day indicator (second line)
      if (city.prevDay) {
        drawText('PREV DAY', L.pad + 8, rowY + 14, C.prevDay, 8, 'normal');
      } else if (city.nextDay) {
        drawText('NEXT DAY', L.pad + 8, rowY + 14, C.nextDay, 8, 'normal');
      }
    });
  }
}

// Draw analog clock face (landscape mode)
function drawClockFace() {
  const clk = LAYOUT.landscape.clock;
  const C = LAYOUT.colors;

  // Clock face circle
  ctx.strokeStyle = C.clockFace;
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.arc(clk.centerX, clk.centerY, clk.radius, 0, Math.PI * 2);
  ctx.stroke();

  // Hour markers (12 positions)
  for (let i = 0; i < 12; i++) {
    const angleDeg = i * 30;
    const angleRad = (angleDeg - 90) * Math.PI / 180;
    const outerR = clk.radius - 3;
    const innerR = clk.radius - 8;

    const x1 = clk.centerX + innerR * Math.cos(angleRad);
    const y1 = clk.centerY + innerR * Math.sin(angleRad);
    const x2 = clk.centerX + outerR * Math.cos(angleRad);
    const y2 = clk.centerY + outerR * Math.sin(angleRad);

    ctx.strokeStyle = C.hourMarker;
    ctx.lineWidth = (i % 3 === 0) ? 2 : 1;  // Thicker at 12, 3, 6, 9
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
  }
}

// Draw clock hand
function drawClockHand(length, angleDeg, color, thickness) {
  const clk = LAYOUT.landscape.clock;
  const angleRad = (angleDeg - 90) * Math.PI / 180;
  const x2 = clk.centerX + length * Math.cos(angleRad);
  const y2 = clk.centerY + length * Math.sin(angleRad);

  ctx.strokeStyle = color;
  ctx.lineWidth = thickness;
  ctx.lineCap = 'round';
  ctx.beginPath();
  ctx.moveTo(clk.centerX, clk.centerY);
  ctx.lineTo(x2, y2);
  ctx.stroke();
}

// Draw analog clock with hands
function drawAnalogClock(hour, minute, second) {
  const clk = LAYOUT.landscape.clock;
  const C = LAYOUT.colors;

  // Draw clock face
  drawClockFace();

  // Calculate angles
  // Hour: 30 deg per hour + 0.5 deg per minute (smooth movement)
  const hourAngle = (hour % 12) * 30 + minute * 0.5;
  // Minute: 6 deg per minute
  const minuteAngle = minute * 6;
  // Second: 6 deg per second
  const secondAngle = second * 6;

  // Draw hands (hour first, then minute, then second on top)
  drawClockHand(clk.hourHandLen, hourAngle, C.hourHand, 3);
  drawClockHand(clk.minuteHandLen, minuteAngle, C.minuteHand, 2);
  drawClockHand(clk.secondHandLen, secondAngle, C.secondHand, 1);

  // Center dot
  ctx.fillStyle = C.hourMarker;
  ctx.beginPath();
  ctx.arc(clk.centerX, clk.centerY, 3, 0, Math.PI * 2);
  ctx.fill();
}

// Render landscape mode display
function renderLandscape(data) {
  const L = LAYOUT.landscape;
  const C = LAYOUT.colors;

  clearCanvas(true);

  // === LEFT PANEL (120px) ===
  // Layout: City (y=6) → HOME (y=30) → Date (y=48) → Clock (y=120) → Time (y=185)

  // Home city name (top) - smaller font for long names (>9 chars)
  const homeLabel = data.home?.label || '--';
  const homeFontSize = homeLabel.length > 9 ? 10 : 14;
  drawText(homeLabel, L.leftPanelWidth / 2, 6, C.label, homeFontSize, 'bold', 'center');

  // HOME indicator
  drawText('HOME', L.leftPanelWidth / 2, 30, C.home, 10, 'normal', 'center');

  // Date
  drawText(data.date || '--', L.leftPanelWidth / 2, 48, C.time, 11, 'bold', 'center');

  // Analog clock (centered in middle section)
  if (data.clock) {
    drawAnalogClock(data.clock.hour, data.clock.minute, data.clock.second);
  }

  // Digital time (bottom)
  drawText(data.home?.time || '--:--', L.leftPanelWidth / 2, 185, C.time, 20, 'bold', 'center');

  // Prev day or Next day indicator (if applicable)
  if (data.home?.prevDay) {
    drawText('PREV DAY', L.leftPanelWidth / 2, 210, C.prevDay, 9, 'normal', 'center');
  } else if (data.home?.nextDay) {
    drawText('NEXT DAY', L.leftPanelWidth / 2, 210, C.nextDay, 9, 'normal', 'center');
  }

  // Environmental data (below digital time at y=218, if sensor available)
  if (data.sensorAvailable && data.envData) {
    drawText(data.envData, L.leftPanelWidth / 2, 218, C.envData, 9, 'normal', 'center');
  }

  // Vertical separator line
  ctx.strokeStyle = '#333333';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(L.leftPanelWidth, 0);
  ctx.lineTo(L.leftPanelWidth, L.height);
  ctx.stroke();

  // === RIGHT PANEL (200px) - 5 remote cities ===
  const rightX = L.leftPanelWidth + 8;

  if (data.remote && data.remote.length > 0) {
    data.remote.forEach((city, i) => {
      const rowY = i * L.remoteRowHeight + 4;

      // Separator line (except first row)
      if (i > 0) {
        ctx.strokeStyle = '#222222';
        ctx.beginPath();
        ctx.moveTo(L.leftPanelWidth, rowY - 2);
        ctx.lineTo(L.width, rowY - 2);
        ctx.stroke();
      }

      // City label - smaller font for long names (>9 chars)
      const cityLabel = city.label || '--';
      const cityFontSize = cityLabel.length > 9 ? 9 : 12;
      drawText(cityLabel, rightX, rowY + 2, C.label, cityFontSize, 'bold');

      // Time
      drawText(city.time || '--:--', L.width - 8, rowY + 2, C.time, 20, 'bold', 'right');

      // Prev day or Next day indicator
      if (city.prevDay) {
        drawText('PREV DAY', rightX, rowY + 20, C.prevDay, 9, 'normal');
      } else if (city.nextDay) {
        drawText('NEXT DAY', rightX, rowY + 20, C.nextDay, 9, 'normal');
      }
    });
  }
}

// Main render function - chooses portrait or landscape
function renderClock(data) {
  if (!data) return;

  const isLandscape = data.landscapeMode === true;

  // Initialize/resize canvas if needed
  if (!initCanvas(isLandscape)) return;

  // Render appropriate mode
  if (isLandscape) {
    renderLandscape(data);
  } else {
    // Portrait mode: choose between standard and alternate layouts
    if (data.showingAlternateScreen && data.sensorAvailable) {
      renderAlternatePortrait(data);
    } else {
      renderPortrait(data);
    }
  }
}

// Update clock display
async function updateMirror() {
  const data = await fetchClock();
  renderClock(data);
}
)JS";

// style.css
static const char STYLE_CSS[] PROGMEM = R"CSS(/* CYD World Clock - Web UI Styles */

* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
  background: #1a1a2e;
  color: #eee;
  line-height: 1.6;
  padding: 20px;
  max-width: 900px;
  margin: 0 auto;
}

header {
  background: #16213e;
  padding: 20px;
  border-radius: 10px;
  margin-bottom: 20px;
  border: 1px solid #0f3460;
}

header h1 {
  color: #00d9ff;
  font-size: 2rem;
  margin-bottom: 5px;
}

header .sub {
  color: #aaa;
  font-size: 0.9rem;
}

main {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.card {
  background: #16213e;
  border: 1px solid #0f3460;
  border-radius: 10px;
  padding: 20px;
}

.card h2 {
  color: #00d9ff;
  font-size: 1.4rem;
  margin-bottom: 15px;
  border-bottom: 2px solid #0f3460;
  padding-bottom: 10px;
}

.card h3 {
  color: #00adb5;
  font-size: 1.1rem;
  margin-top: 15px;
  margin-bottom: 10px;
}

/* Canvas Display Mirror */
#canvasContainer {
  display: flex;
  justify-content: center;
  align-items: center;
  background: #000;
  padding: 10px;
  border-radius: 5px;
  border: 2px solid #00d9ff;
}

#displayCanvas {
  image-rendering: pixelated;
  image-rendering: crisp-edges;
  background: #000;
  display: block;
}

/* Status Grid */
.status-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 15px;
}

.status-item {
  padding: 10px;
  background: #0f3460;
  border-radius: 5px;
}

.status-item .label {
  display: block;
  color: #888;
  font-size: 0.85rem;
  margin-bottom: 5px;
}

.status-item span:not(.label) {
  color: #00d9ff;
  font-weight: bold;
}

/* Form Styles */
.form-group {
  margin-bottom: 20px;
}

label {
  display: block;
  color: #aaa;
  margin-bottom: 5px;
  margin-top: 10px;
  font-size: 0.9rem;
}

input[type="text"],
input[type="number"],
select {
  width: 100%;
  padding: 10px;
  background: #0f3460;
  border: 1px solid #00adb5;
  border-radius: 5px;
  color: #eee;
  font-size: 1rem;
}

input[type="text"]:focus,
input[type="number"]:focus,
select:focus {
  outline: none;
  border-color: #00d9ff;
  box-shadow: 0 0 5px rgba(0, 217, 255, 0.3);
}

small {
  display: block;
  color: #888;
  font-size: 0.8rem;
  margin-top: 3px;
}

.city-group {
  background: #0f3460;
  padding: 15px;
  border-radius: 5px;
  margin-bottom: 10px;
  border-left: 3px solid #00adb5;
}

/* Buttons */
.form-actions {
  display: flex;
  gap: 10px;
  margin-top: 20px;
}

button {
  padding: 12px 24px;
  border: none;
  border-radius: 5px;
  font-size: 1rem;
  cursor: pointer;
  transition: all 0.3s;
}

.btn-primary {
  background: #00adb5;
  color: white;
}

.btn-primary:hover {
  background: #00d9ff;
}

.btn-secondary {
  background: #535c68;
  color: white;
}

.btn-secondary:hover {
  background: #6c757d;
}

.btn-warning {
  background: #d63031;
  color: white;
}

.btn-warning:hover {
  background: #e17055;
}

/* Timezone Reference */
.tz-reference {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 10px;
}

.tz-item {
  padding: 8px 12px;
  background: #0f3460;
  border-radius: 5px;
  font-size: 0.9rem;
  border-left: 3px solid #00adb5;
}

.tz-item strong {
  color: #00d9ff;
  margin-right: 5px;
}

/* Footer */
footer {
  margin-top: 30px;
  text-align: center;
  color: #666;
  font-size: 0.85rem;
}

footer a:hover {
  color: #00ffff !important;
  transform: translateY(-2px);
  transition: all 0.3s;
}

footer svg {
  transition: all 0.3s;
}

footer a:hover svg {
  transform: scale(1.1);
}

/* Responsive */
@media (max-width: 600px) {
  header h1 {
    font-size: 1.5rem;
  }

  .form-actions {
    flex-direction: column;
  }

  button {
    width: 100%;
  }

  .status-grid {
    grid-template-columns: 1fr;
  }

  /* Canvas at 1:1 scale on mobile (handled in JS) */
  #canvasContainer {
    padding: 5px;
  }
}
)CSS";
