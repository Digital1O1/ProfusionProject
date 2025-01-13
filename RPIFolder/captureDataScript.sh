#!/bin/bash

# Run the first Python script
echo "Running camera preview"
python3 /home/pi/ProfusionProject/PreviewVideo/main.py

# Check if the first script ran successfully
if [ $? -ne 0 ]; then
    echo "script1.py failed to execute. Exiting."
    exit 1
fi

# Run the second Python script
echo "Running Raw Video Recording Script..."
python3 /home/pi/ProfusionProject/RecordRawVideo/main.py

# Check if the second script ran successfully
if [ $? -ne 0 ]; then
    echo "script2.py failed to execute. Exiting."
    exit 1
fi


# Directory you want to transfer to said host (The RPI)
LOCAL_DIR="/home/pi/ProfusionProject/RPI_Folder/SCPTransfer/TransferFolder"    
REMOTE_USER="utsw-bmen-laptop"            
REMOTE_HOST="172.17.140.56"

# Destination 
REMOTE_DIR="/home/utsw-bmen-laptop/ProfusionFolder/DataFromRPI" 

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
