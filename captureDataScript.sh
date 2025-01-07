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

