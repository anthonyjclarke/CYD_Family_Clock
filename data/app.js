// CYD World Clock - Web UI JavaScript

// Global timezone list
let timezones = [];

// Load state on page load
document.addEventListener('DOMContentLoaded', () => {
  loadTimezones();
  loadState();

  // Set up event listeners
  document.getElementById('configForm').addEventListener('submit', handleSaveConfig);
  document.getElementById('reloadBtn').addEventListener('click', loadState);
  document.getElementById('rebootBtn').addEventListener('click', handleReboot);
  document.getElementById('resetWifiBtn').addEventListener('click', handleResetWiFi);

  // Timezone dropdown change listeners
  document.getElementById('homeSelect').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote0Select').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote1Select').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote2Select').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote3Select').addEventListener('change', updateTimezoneFields);
  document.getElementById('remote4Select').addEventListener('change', updateTimezoneFields);

  // Auto-refresh status every 30 seconds
  setInterval(loadState, 30000);
});

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

// Load current state from device
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
    document.getElementById('uptime').textContent = formatUptime(data.uptime || 0);

    // Update form fields
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
      document.body.innerHTML = '<div style="text-align:center; padding:50px;"><h1>Device Rebooting...</h1><p>Reconnect to the AP "CYD-WorldClock-Setup" to reconfigure WiFi.</p></div>';
    }, 2000);
  } catch (error) {
    console.error('Error resetting WiFi:', error);
    showNotification('Error resetting WiFi credentials', 'error');
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
