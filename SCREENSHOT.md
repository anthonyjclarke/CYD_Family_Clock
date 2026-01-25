# CYD World Clock - Screenshot Capture

This firmware includes screenshot capability to capture the actual TFT display output for documentation purposes.

## Quick Start

### Method 1: Web API (Easiest)

1. Upload firmware: `pio run -t upload`
2. Get device IP from display
3. Trigger screenshot: `curl http://<device-ip>/api/screenshot`
4. Capture output: `python3 capture_screenshot.py /dev/cu.usbserial-0001 <device-ip>`

### Method 2: Serial Only

1. Upload firmware: `pio run -t upload`
2. Run capture script: `python3 capture_screenshot.py /dev/cu.usbserial-0001`
3. Manually visit `http://<device-ip>/api/screenshot` in browser
4. Screenshot will be saved as `screenshot.png`

## Requirements

```bash
pip install pyserial pillow requests
```

## Serial Port Detection

The script auto-detects ESP32 ports. If detection fails, specify manually:

**macOS:**

```bash
python3 capture_screenshot.py /dev/cu.usbserial-0001
```

**Linux:**

```bash
python3 capture_screenshot.py /dev/ttyUSB0
```

**Windows:**

```bash
python capture_screenshot.py COM3
```

## How It Works

1. **Web API** (`/api/screenshot`) triggers the screenshot function
2. **ESP32** reads all TFT pixels via `tft.readPixel()`
3. **RGB565 → RGB888** conversion happens on the device
4. **PPM format** (uncompressed RGB) is sent via serial (115200 baud)
5. **Python script** captures the data and converts to PNG using Pillow

## Technical Details

- **Format**: PPM (Portable Pixmap) - simple uncompressed RGB
- **Size**: ~230KB for 240×320 display
- **Speed**: ~15-20 seconds for full capture at 115200 baud
- **Accuracy**: Pixel-perfect capture of actual TFT output

## Output

Screenshot saved as `screenshot.png` (240×320 pixels)

## Troubleshooting

### "No module named 'serial'"

```bash
pip install pyserial
```

### "Permission denied" on serial port (Linux/Mac)

```bash
sudo chmod 666 /dev/ttyUSB0  # Linux
sudo chmod 666 /dev/cu.usbserial-0001  # Mac
```

### Screenshot appears corrupted

- Ensure no other serial monitors are open
- Check baud rate is 115200
- Try the raw RGB565 format: edit code to call `takeScreenshotRaw()` instead

### Slow capture

- Normal! Reading 76,800 pixels at 115200 baud takes time
- Faster serial speeds may not be reliable for this much data

## Alternative: Use readPixel() from code

For one-off screenshots during development, you can call the function directly:

```cpp
void loop() {
  static bool screenshotTaken = false;
  if (!screenshotTaken && millis() > 10000) {  // After 10 seconds
    takeScreenshot();
    screenshotTaken = true;
  }
  // ... rest of loop
}
```

Then capture serial output to a file and save as .ppm.
