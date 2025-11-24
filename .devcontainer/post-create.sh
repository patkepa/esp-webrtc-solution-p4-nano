#!/bin/bash
set -e

echo "Setting up ESP32 development environment..."

# Source ESP-IDF environment
echo "Sourcing ESP-IDF environment..."
if [ -f "/opt/esp/idf/export.sh" ]; then
    source /opt/esp/idf/export.sh
    echo "✓ ESP-IDF environment sourced"
else
    echo "❌ ESP-IDF export.sh not found"
    exit 1
fi

# Verify ESP-IDF installation
echo "Verifying ESP-IDF installation..."
if command -v idf.py >/dev/null 2>&1; then
    echo "✓ ESP-IDF version: $(idf.py --version)"
else
    echo "❌ ESP-IDF not found in PATH after sourcing"
    exit 1
fi

# Configure git safe directories
echo "Configuring git safe directories..."
git config --global --add safe.directory /opt/esp/idf
git config --global --add safe.directory '*'
echo "✓ Git safe directories configured"

# Check USB device access
echo "Checking USB device access..."

if [ -d "/dev/bus/usb" ]; then
    echo "✓ USB bus access configured"

    # Look for common ESP32 USB-to-serial chips
    if lsusb 2>/dev/null | grep -E "(10c4:ea60|1a86:7523|0403:6001|067b:2303)" >/dev/null 2>&1; then
        echo "✓ ESP32 device detected:"
        lsusb | grep -E "(10c4:ea60|1a86:7523|0403:6001|067b:2303)"
    fi

    # Check for serial ports
    DEVICE_FOUND=false
    DEVICE_ACCESSIBLE=false

    for port in /dev/ttyUSB* /dev/ttyACM*; do
        if [ -e "$port" ]; then
            DEVICE_FOUND=true
            echo "Found: $port ($(ls -l $port | awk '{print $3":"$4" "$1}'))"

            if [ -r "$port" ] && [ -w "$port" ]; then
                DEVICE_ACCESSIBLE=true
                echo "  ✓ Device is accessible!"
            else
                echo "  ⚠️  Device permissions need fixing"
            fi
        fi
    done

    if [ "$DEVICE_FOUND" = false ]; then
        echo "⚠️  No USB serial devices found (this is normal if no device is connected)"
    fi

    if [ "$DEVICE_FOUND" = true ] && [ "$DEVICE_ACCESSIBLE" = false ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "⚠️  USB DEVICE PERMISSION FIX REQUIRED"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "To enable flashing, run this on your HOST machine:"
        echo ""
        echo "  sudo chmod 666 /dev/ttyACM0"
        echo ""
        echo "Or add yourself to the dialout group:"
        echo ""
        echo "  sudo usermod -aG dialout $USER"
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
    fi
fi

# Show current user groups
echo "Current user: $(whoami)"
echo "Groups: $(groups)"

# Fix workspace permissions
echo "Fixing workspace permissions..."
sudo chown -R esp:esp /workspaces/ci-firmware-doorbell-esp32-s3 2>/dev/null || true

echo ""
echo "✅ ESP32 development environment setup complete!"
echo ""
echo "🏗️  Your development environment includes:"
echo "   ✓ ESP-IDF v5.5.1 with all tools"
echo "   ✓ QEMU for ESP32 emulation"
echo "   ✓ USB device access configured"
echo ""
echo "💡 Quick start commands:"
echo "   idf.py build                  - Build the firmware"
echo "   idf.py -p /dev/ttyACM0 flash  - Flash to device"
echo "   idf.py monitor                - Monitor serial output"
echo "   idf.py flash monitor          - Flash and monitor"
echo ""
