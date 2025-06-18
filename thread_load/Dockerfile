FROM fedora:42

# Install necessary packages
RUN dnf install -y \
    gcc \
    make \
    glibc-devel \
    glibc-headers \
    perf \
    procps-ng \
    && dnf clean all

# Set working directory
WORKDIR /app

# Copy the C source code into the container
COPY thread_load_optimized.c .

# Compile the program
RUN gcc -pthread -o thread_load thread_load_optimized.c

# Default command to run the test loop
CMD bash -c 'for num in `seq 5 5 100`; do perf stat -v -- ./thread_load -b $num -m $num -d 10 -o 10000000 -t 1 -l 10; sleep 5; done'
