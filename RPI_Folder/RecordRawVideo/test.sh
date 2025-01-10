#!/bin/bash

# Script to measure the time taken to transfer a local directory to a remote host using SCP.

# Variables (customize these)
LOCAL_DIR="/home/pi/ProfusionProject/RecordRawVideo/recordings/"    # Path to the local directory you want to transfer
REMOTE_USER="digital101"            # Remote server username
REMOTE_HOST="192.168.1.159"        # Remote server hostname or IP
REMOTE_DIR="C:\Users\chris\Desktop\profusionFolder" # Path on the remote server where the directory will be copied

# Check if LOCAL_DIR exists
if [ ! -d "$LOCAL_DIR" ]; then
  echo "Error: Local directory '$LOCAL_DIR' does not exist."
  exit 1
fi

# Measure time and execute SCP
echo "Starting SCP transfer..."
START_TIME=$(date +%s%N) # Record start time in nanoseconds

scp -r "$LOCAL_DIR" "$REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR"

# Check the exit status of SCP
if [ $? -ne 0 ]; then
  echo "SCP transfer failed."
  exit 1
fi

END_TIME=$(date +%s%N) # Record end time in nanoseconds
TIME_TAKEN_MS=$(( (END_TIME - START_TIME) / 1000000 )) # Calculate time taken in milliseconds

# Display the results
echo "SCP transfer completed successfully!"
echo "Time taken: $TIME_TAKEN_MS milliseconds."
