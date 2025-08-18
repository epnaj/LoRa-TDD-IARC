FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    git \
    curl \
    build-essential \
    gdb \
    python3

RUN echo "ulimit -c unlimited" >> /etc/profile

# Set the working directory inside the container
WORKDIR /workspace
COPY . /workspace
RUN echo "source /workspace/commands.sh" >> ~/.bash_aliases

# Default shell when container starts
ENTRYPOINT [ "/usr/bin/env", "bash", "-l", "-c" ]