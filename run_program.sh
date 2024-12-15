#!/bin/bash

# Assign the first argument as the target directory
DIR=$1

# Check if the directory exists
if [ ! -d "$DIR" ]; then
    echo "Error: Directory $DIR does not exist."
    exit 1
fi

# Find all .ppm files in the directory
FILES=$(find "$DIR" -maxdepth 1 -type f -name "*.ppm")

# Check if any .ppm files were found
if [ -z "$FILES" ]; then
    echo "No .ppm files found in the directory $DIR."
    exit 1
fi

# Run edge_detector with the .ppm files as arguments
./edge_detector $FILES

# Exit script
exit 0
