// CYD World Clock - Web UI JavaScript

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
    document.getElementById('displayMode').value = data.landscapeMode ? 'landscape' : 'portrait';
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
    document.getElementById('displayMode').value = data.landscapeMode ? 'landscape' : 'portrait';
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
    secondHand: '#FF0000'
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
    renderPortrait(data);
  }
}

// Update clock display
async function updateMirror() {
  const data = await fetchClock();
  renderClock(data);
}
