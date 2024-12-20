#!/usr/bin/python3
import time
import threading
import subprocess
import numpy as np
from pidng.core import RAW2DNG, DNGTags, Tag
from pidng.defs import *
from picamera2 import Picamera2
from picamera2.encoders import Encoder

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
save_dir = "/home/pi/ProfusionProject/RecordRawVideo/captureFolder"
recording_counter = 0  # Initialize recording counter

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

# Start parallel recording
# recordCamera0 = threading.Thread(target=start_recording, args=(camera0, encoder0, 'test0', 'timestamp0', save_dir, recording_counter))
# recordCamera1 = threading.Thread(target=start_recording, args=(camera1, encoder1, 'test1', 'timestamp1', save_dir, recording_counter))

print("\n================= Recording video now for " + str(duration) + " seconds =================")

# recordCamera0.start()
# recordCamera1.start()

# Testing non-thread
# picam2.start_recording(encoder, 'test.raw', pts='timestamp.txt')
camera0.start_recording(encoder0,'test0.raw',pts="timestamp0.txt")
camera1.start_recording(encoder1,'test1.raw',pts="timestamp1.txt")

# Start countdown timer
countdown_timer(duration)

# Stop parallel recording
# recordCamera0.join()
# recordCamera1.join()

camera0.stop_recording()
camera1.stop_recording()

print("\n================= Processing Data =================")

# Process raw data
def process_camera_data(filename, size, camera_number):
    raw_file = f"{save_dir}/{filename}_{recording_counter}.raw"
    with open(raw_file, "rb") as f:
        buf = f.read(size[0] * size[1] * 2)
    arr = np.frombuffer(buf, dtype=np.uint16).reshape((size[1], size[0]))
    create_dng(arr, size, camera_number, save_dir)

process_camera_data("test0", size, 0)
process_camera_data("test1", size, 1)

print("\n================= Processing Data COMPLETE =================")
print(f"\n Files saved at: {save_dir}\n\n Executing bash script to transfer data to Linux Laptop...")

# bash_script_path = "/home/pi/Onboard_VS_Streaming_RPI/PythonScripts/CaptureRawVideo/transfer.sh"
# run_bash_script(bash_script_path)

# print("Bash script execution complete.")
