#!/bin/bash
# Directory where you want to source the data (From Linux Laptop)
LOCAL_DIR="/home/utsw-bmen-laptop/ProfusionProject/LinuxFolder/AlignImages/images/ProcessedImage"    

# Where you want to send the processed data (Back to RPI)
REMOTE_USER="pi"            
REMOTE_HOST="172.17.141.124"
REMOTE_DIR= "/home/pi/ProfusionProject/RPIFolder/AlignImages/images/"
# Santiy check if the directory is actually there
# Check if LOCAL_DIR exists
if [ ! -d "$LOCAL_DIR" ]; then
  echo "Error: Local directory '$LOCAL_DIR' does not exist."
  exit 1
fi
echo "Starting SCP transfer..."

#Copy a local file to a remote host:
#   scp {{path/to/local_file}} {{remote_host}}:{{path/to/remote_file}}

scp -r "$LOCAL_DIR" "$REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR"

echo "Transfer complete..."
