#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <directory>"
    exit 1
fi

DIRECTORY=$1

# List of file extensions to format
Extensions=("*.cc" "*.cpp" "*.hpp")

# Iterate over each extension and format the files
for ext in "${Extensions[@]}"; do
    find "$DIRECTORY" -type f -name "$ext" -exec clang-format -i {} +
done

echo "All files in $DIRECTORY formatted successfully."
