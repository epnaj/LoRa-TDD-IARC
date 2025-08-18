#!/usr/bin/env bash

# Check if a volume name is provided
if [ -z "$1" ]; then
  echo "Error: No volume name provided."
  echo "Usage: $0 <volume_name>"
  exit 1
fi

VOLUME_NAME="$1"

# Check if Docker is installed and accessible
if ! command -v docker &> /dev/null; then
  echo "Error: Docker is not installed or not in PATH."
  exit 1
fi

# Check if volume already exists
if docker volume ls --format '{{.Name}}' | grep -wq "$VOLUME_NAME"; then
  echo "Warning: Volume '$VOLUME_NAME' already exists."
  exit 0
fi

# Create the volume
if docker volume create "$VOLUME_NAME"; then
  echo "Volume '$VOLUME_NAME' created successfully."
else
  echo "Error: Failed to create volume '$VOLUME_NAME'."
  exit 1
fi
