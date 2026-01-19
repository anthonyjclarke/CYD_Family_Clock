#!/usr/bin/env python3
"""
CYD World Clock Screenshot Capture Tool

Captures screenshot from ESP32 via serial and saves as PNG image.

USAGE:
    1. Upload firmware with screenshot support to ESP32
    2. Trigger screenshot via web: curl http://<device-ip>/api/screenshot
    3. Run this script: python3 capture_screenshot.py
    4. Screenshot saved as: screenshot.png

REQUIREMENTS:
    pip install pyserial pillow requests
"""

import serial
import sys
import time
from PIL import Image
import io
import requests

def find_esp32_port():
    """Auto-detect ESP32 serial port"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()

    for port in ports:
        # Look for common ESP32 identifiers
        if 'CP210' in port.description or 'CH340' in port.description or 'USB' in port.description:
            print(f"Found potential ESP32 at: {port.device}")
            return port.device

    return None

def capture_screenshot_ppm(port='/dev/cu.usbserial-330', baudrate=115200, output='screenshot.png'):
    """
    Capture screenshot in PPM format and convert to PNG

    Args:
        port: Serial port (e.g., '/dev/cu.usbserial-0001' on Mac, 'COM3' on Windows)
        baudrate: Serial baud rate (must match ESP32 - default 115200)
        output: Output PNG filename
    """
    print(f"Connecting to {port} at {baudrate} baud...")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(1)  # Wait for connection to stabilize

        print("Connected. Waiting for PPM header...")

        # Read until we find PPM header "P6"
        while True:
            line = ser.readline().decode('ascii', errors='ignore').strip()
            if line == 'P6':
                print("Found PPM header!")
                break
            if line:
                print(f"Debug: {line}")

        # Read dimensions
        dims = ser.readline().decode('ascii').strip().split()
        width = int(dims[0])
        height = int(dims[1])
        print(f"Dimensions: {width}x{height}")

        # Read max color value
        max_val = ser.readline().decode('ascii').strip()
        print(f"Max color value: {max_val}")

        # Read RGB data
        print("Reading pixel data...")
        data_size = width * height * 3  # 3 bytes per pixel (RGB)
        data = bytearray()

        while len(data) < data_size:
            chunk = ser.read(min(4096, data_size - len(data)))
            if not chunk:
                print("Timeout reading data")
                break
            data.extend(chunk)
            progress = (len(data) / data_size) * 100
            print(f"\rProgress: {progress:.1f}%", end='', flush=True)

        print(f"\nReceived {len(data)} bytes")

        if len(data) < data_size:
            print(f"WARNING: Expected {data_size} bytes, got {len(data)}")

        # Convert to PIL Image
        img = Image.frombytes('RGB', (width, height), bytes(data))

        # Save as PNG
        img.save(output)
        print(f"\nScreenshot saved to: {output}")

        ser.close()
        return True

    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return False
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def trigger_screenshot_via_web(device_ip):
    """Trigger screenshot via web API"""
    url = f"http://{device_ip}/api/screenshot"
    print(f"Triggering screenshot at {url}...")
    try:
        response = requests.get(url, timeout=5)
        print(f"Response: {response.text}")
        return True
    except Exception as e:
        print(f"Error triggering screenshot: {e}")
        return False

def main():
    print("CYD World Clock Screenshot Capture")
    print("=" * 50)

    # Auto-detect port or use provided argument
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = find_esp32_port()
        if not port:
            print("\nCouldn't auto-detect ESP32 port.")
            print("Usage: python3 capture_screenshot.py [serial_port]")
            print("Example (Mac): python3 capture_screenshot.py /dev/cu.usbserial-0001")
            print("Example (Linux): python3 capture_screenshot.py /dev/ttyUSB0")
            print("Example (Windows): python3 capture_screenshot.py COM3")
            sys.exit(1)

    # Optional: trigger via web API if device IP provided as second argument
    if len(sys.argv) > 2:
        device_ip = sys.argv[2]
        trigger_screenshot_via_web(device_ip)
        time.sleep(1)  # Wait for screenshot to start
    else:
        print("\nTo trigger screenshot via web API, add device IP as second argument:")
        print(f"  python3 capture_screenshot.py {port} 192.168.1.100")
        print("\nOr manually visit: http://<device-ip>/api/screenshot")
        print("\nWaiting for screenshot data on serial port...")

    # Capture screenshot
    success = capture_screenshot_ppm(port)

    if success:
        print("\n✓ Screenshot captured successfully!")
        print("  Open screenshot.png to view")
    else:
        print("\n✗ Screenshot capture failed")
        sys.exit(1)

if __name__ == '__main__':
    main()
