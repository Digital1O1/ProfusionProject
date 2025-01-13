#!/usr/bin/python3

# Normally the QtGlPreview implementation is recommended as it benefits
# from GPU hardware acceleration.

import time
from picamera2 import Picamera2, Preview

# Initialize both cameras
camera0 = Picamera2(camera_num=0)
camera1 = Picamera2(camera_num=1)

# Start preview for both cameras
camera0.start_preview(Preview.QTGL, x=100, y=100, width=640, height=480)
camera1.start_preview(Preview.QTGL, x=800, y=100, width=640, height=480)

# Create preview configurations for both cameras
preview_config0 = camera0.create_preview_configuration()
preview_config1 = camera1.create_preview_configuration()

# Configure both cameras with the preview configurations
camera0.configure(preview_config0)
camera1.configure(preview_config1)

# Start both cameras
camera0.start()
camera1.start()

# Wait for user to press Enter to stop the preview
print("Press Enter to stop the preview...")
input()

# Stop the previews after Enter is pressed
camera0.stop()
camera1.stop()

print("Preview stopped.")
