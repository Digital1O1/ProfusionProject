#!/bin/bash
# Directory you want to transfer to said host (The RPI)
LOCAL_DIR="/media/pi/YELLOW_USB/profusionFolder"    
REMOTE_USER="utsw-bmen-laptop"            
REMOTE_HOST="172.17.140.56"

# Destination 
REMOTE_DIR="/home/utsw-bmen-laptop/ProfusionFolder/RawDataFromRPI" 

# Check if LOCAL_DIR exists
if [ ! -d "$LOCAL_DIR" ]; then
  echo "Error: Local directory '$LOCAL_DIR' does not exist."
  exit 1
fi

# ================================ Measure time and execute SCP ================================
echo "Starting SCP transfer..."
START_TIME=$(date +%s%N) # Record start time in nanoseconds

scp -r "$LOCAL_DIR" "$REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR"

# Check the exit status of SCP
# $? : Special bash variable that holds exit status of most recently executed command
# -ne : Comparison operator in bash that means 'not equal'
if [ $? -ne 0 ]; then
  echo "SCP transfer failed."
  exit 1
fi

END_TIME=$(date +%s%N) 
TIME_TAKEN_NS=$((END_TIME - START_TIME)) # Time taken in nanoseconds

# Convert nanoseconds to milliseconds
TIME_TAKEN_MS=$((TIME_TAKEN_NS / 1000000))  

# Convert time to minutes:seconds:milliseconds
SECONDS=$((TIME_TAKEN_MS / 1000))           # Total seconds
MINUTES=$((SECONDS / 60))                  # Minutes part
SECONDS=$((SECONDS % 60))                  # Remaining seconds part
MILLISECONDS=$((TIME_TAKEN_MS % 1000))     # Milliseconds part

# Display the results
echo "SCP transfer completed successfully!"
printf "Time taken: %02d:%02d:%03d (minutes:seconds:milliseconds)\n" "$MINUTES" "$SECONDS" "$MILLISECONDS"
