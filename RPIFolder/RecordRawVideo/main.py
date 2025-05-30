#!/usr/bin/python3
import time
import threading
import subprocess
import numpy as np
from pidng.core import RAW2DNG, DNGTags, Tag
from pidng.defs import *
from picamera2 import Picamera2, Preview
from picamera2.encoders import Encoder
import os

# Function to ensure the save directory exists
def ensure_directory(save_dir):
    if not os.path.exists(save_dir):
        os.makedirs(save_dir)

# Function to run a bash script
def run_bash_script(script_path):
    try:
        result = subprocess.run(["bash", script_path], check=True, text=True, capture_output=True)
        print("Bash script output:")
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print("Error while running bash script:")
        print(e.stderr)

# Function to start recording video
def start_recording(camera, encoder, filename, pts_filename, save_dir, recording_counter):
    rawVideoFilePath = f"{save_dir}/{filename}_{recording_counter}.raw"
    timeStampFilePath = f"{save_dir}/{pts_filename}_{recording_counter}.txt"
    print(f"Recording video to {rawVideoFilePath} and timestamp to {timeStampFilePath}")
    camera.start_recording(encoder, rawVideoFilePath, pts=timeStampFilePath)

# Function for a countdown timer
def countdown_timer(duration):
    while duration:
        mins, secs = divmod(duration, 60)
        print(f'{mins:02d}:{secs:02d}', end='\r')
        time.sleep(1)
        duration -= 1
    print("\n================= Recording COMPLETE =================")

# Function to create DNG files
def create_dng(arr, size, camera_number, save_dir):
    r = RAW2DNG()
    t = DNGTags()
    bpp = 10
    t.set(Tag.ImageWidth, size[0])
    t.set(Tag.ImageLength, size[1])
    t.set(Tag.TileWidth, size[0])
    t.set(Tag.TileLength, size[1])
    t.set(Tag.Orientation, Orientation.Horizontal)
    t.set(Tag.PhotometricInterpretation, PhotometricInterpretation.Color_Filter_Array)
    t.set(Tag.SamplesPerPixel, 1)
    t.set(Tag.BitsPerSample, bpp)
    t.set(Tag.CFARepeatPatternDim, [2, 2])
    t.set(Tag.CFAPattern, CFAPattern.GBRG)
    t.set(Tag.DNGVersion, DNGVersion.V1_4)
    t.set(Tag.DNGBackwardVersion, DNGVersion.V1_2)

    save_path = f"{save_dir}/camera{camera_number}"
    r.options(t, path=save_dir, compress=False)
    r.convert(arr, filename=save_path)

# Main script
size = (1920, 1080)
save_dir = "/media/pi/YELLOW_USB/profusionFolder"
recording_counter = 0  # Initialize recording counter

# Ensure the save directory exists
ensure_directory(save_dir)

print("\n\n\n================= Initializing camera parameters =================")

camera0 = Picamera2(camera_num=0)
camera1 = Picamera2(camera_num=1)

video_config0 = camera0.create_video_configuration(raw={"format": 'SGBRG10', 'size': size})
camera0.configure(video_config0)
camera0.encode_stream_name = "raw"
encoder0 = Encoder()

video_config1 = camera1.create_video_configuration(raw={"format": 'SGBRG10', 'size': size})
camera1.configure(video_config1)
camera1.encode_stream_name = "raw"
encoder1 = Encoder()

print("\n================= DONE =================")

# Input time in seconds
duration = int(input("Enter the recording duration in seconds: "))

# This section was to have a 'preview' mode before recording the raw data and is probably not needed
# # Start preview for both cameras
# camera0.start_preview(Preview.QTGL, x=100, y=100, width=640, height=480)
# camera1.start_preview(Preview.QTGL, x=800, y=100, width=640, height=480)

# preview_config0 = camera0.create_preview_configuration()
# preview_config1 = camera1.create_preview_configuration()

# # Configure both cameras
# camera0.configure(preview_config0)
# camera1.configure(preview_config1)

# # Start both cameras for preview
# camera0.start()
# camera1.start()

# # Wait for user to press Enter to stop the preview
# print("Press Enter to stop the preview...")
# input()

# # Stop the previews after Enter is pressed
# camera0.stop()
# camera1.stop()

print("\n================= Recording video now for " + str(duration) + " seconds =================")

# Start recording for both cameras
camera0.start_recording(encoder0, f'{save_dir}/visibleCamera_{recording_counter}.raw', pts=f"{save_dir}/visibleTimeStamps_{recording_counter}.txt")
camera1.start_recording(encoder1, f'{save_dir}/irCamera_{recording_counter}.raw', pts=f"{save_dir}/irTimeStamps_{recording_counter}.txt")

# Start countdown timer
countdown_timer(duration)

# Stop recording
camera0.stop_recording()
camera1.stop_recording()

# Increment the recording counter for the next recording
recording_counter += 1

print("\n================= Processing Data =================")

# Process raw data
def process_camera_data(filename, size, camera_number):
    raw_file = f"{save_dir}/{filename}_{recording_counter-1}.raw"  # Correct the counter to the previous file
    with open(raw_file, "rb") as f:
        buf = f.read(size[0] * size[1] * 2)
    arr = np.frombuffer(buf, dtype=np.uint16).reshape((size[1], size[0]))
    create_dng(arr, size, camera_number, save_dir)

process_camera_data("visibleCamera", size, 0)
process_camera_data("irCamera", size, 1)

print("\n================= Processing Data COMPLETE =================")
print(f"\nFiles saved at: {save_dir}\n\n")

# Save to USB drive
# sudo chown -R pi:pi /media/pi/d650369d-5fad-4cd4-a76d-262f0b4cdb93/profusionFolder
