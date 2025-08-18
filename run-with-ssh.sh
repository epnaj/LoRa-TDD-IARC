#!/usr/bin/env bash

# === USAGE CHECK ===
if [ $# -lt 1 ]; then
  echo "Usage: $0 <image-name> [container-name] [volume-name]"
  exit 1
fi

# === ARGUMENTS ===
IMAGE_NAME="$1"
CONTAINER_NAME="${2:-ssh-dev}"
VOLUME_NAME="$3"

# === GET GIT CONFIG FROM HOST ===
GIT_NAME=$(git config --global user.name)
GIT_EMAIL=$(git config --global user.email)

# === FALLBACK WARNINGS ===
if [ -z "$GIT_NAME" ] || [ -z "$GIT_EMAIL" ]; then
  echo "Warning: Git user.name or user.email not set globally on host."
  echo "Run 'git config --global user.name \"Your Name\"' and user.email if needed."
fi

# === DOCKER VOLUME HANDLING ===
VOLUME_MOUNT=""
if [ -n "$VOLUME_NAME" ]; then
  # Check if docker is installed and accessible
  if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed or not in PATH."
    exit 1
  fi

  # Check if the volume exists, if not create it
  if docker volume ls --format '{{.Name}}' | grep -wq "$VOLUME_NAME"; then
    echo "Volume '$VOLUME_NAME' already exists. Using it."
  else
    echo "Volume '$VOLUME_NAME' does not exist. Creating it."
    if ! docker volume create "$VOLUME_NAME" > /dev/null; then
      echo "Error: Failed to create volume '$VOLUME_NAME'."
      exit 1
    fi
  fi

  # Mount the volume inside the container at /<volume_name>
  VOLUME_MOUNT="-v $VOLUME_NAME:/$VOLUME_NAME"
fi

# === MAIN SCRIPT ===
docker run -it \
  --name "$CONTAINER_NAME" \
  -v "$HOME/.ssh:/root/.ssh:ro" \
  -v "$HOME:/host-home" \
  $VOLUME_MOUNT \
  -w "/root" \
  -e GIT_AUTHOR_NAME="$GIT_NAME" \
  -e GIT_AUTHOR_EMAIL="$GIT_EMAIL" \
  -e GIT_COMMITTER_NAME="$GIT_NAME" \
  -e GIT_COMMITTER_EMAIL="$GIT_EMAIL" \
  --ulimit core=-1 \
  --cap-add=SYS_PTRACE \
  --security-opt seccomp=unconfined \
  "$IMAGE_NAME" \
  bash -c "
    git config --global user.name \"$GIT_NAME\" && \
    git config --global user.email \"$GIT_EMAIL\"
  "
  # you can add "exec bash" or something here, but this container already runs it with entrypoint
