# Multi-stage Dockerfile for Kitchen CAD Designer
# Supports Ubuntu 22.04 LTS for reproducible builds

FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    pkg-config \
    # Qt6 dependencies
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6opengl6-dev \
    libqt6openglwidgets6 \
    libqt6sql6-sqlite \
    # SQLite dependencies
    libsqlite3-dev \
    sqlite3 \
    # OpenCascade dependencies
    libocct-data-exchange-dev \
    libocct-draw-dev \
    libocct-foundation-dev \
    libocct-modeling-algorithms-dev \
    libocct-modeling-data-dev \
    libocct-ocaf-dev \
    libocct-visualization-dev \
    # JSON library
    nlohmann-json3-dev \
    # Testing framework (optional)
    catch2 \
    # Additional utilities
    ninja-build \
    ccache \
    && rm -rf /var/lib/apt/lists/*

# Set up ccache for faster rebuilds
ENV PATH="/usr/lib/ccache:${PATH}"
ENV CCACHE_DIR=/tmp/ccache
RUN mkdir -p ${CCACHE_DIR}

# Create working directory
WORKDIR /app

# Copy source code
COPY . .

# Create build directory
RUN mkdir -p build

# Configure the project
WORKDIR /app/build
RUN cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -GNinja

# Build the project
RUN ninja -j$(nproc)

# Run tests if available
RUN if [ -f "tests/KitchenCADDesigner_tests" ]; then \
        ctest --output-on-failure --parallel $(nproc); \
    else \
        echo "No tests found, skipping test execution"; \
    fi

# Runtime stage (smaller image for deployment)
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install only runtime dependencies
RUN apt-get update && apt-get install -y \
    # Qt6 runtime libraries
    libqt6core6 \
    libqt6gui6 \
    libqt6widgets6 \
    libqt6opengl6 \
    libqt6openglwidgets6 \
    libqt6sql6 \
    libqt6sql6-sqlite \
    # OpenCascade runtime libraries
    libocct-data-exchange-7.6 \
    libocct-foundation-7.6 \
    libocct-modeling-algorithms-7.6 \
    libocct-modeling-data-7.6 \
    libocct-visualization-7.6 \
    # SQLite runtime
    libsqlite3-0 \
    # X11 and OpenGL for GUI
    libx11-6 \
    libxext6 \
    libxrender1 \
    libgl1-mesa-glx \
    libglu1-mesa \
    # Fonts for better UI rendering
    fonts-dejavu-core \
    && rm -rf /var/lib/apt/lists/*

# Create application user
RUN useradd -m -s /bin/bash kitchencad

# Create application directory
RUN mkdir -p /opt/kitchencad/bin /opt/kitchencad/resources
RUN chown -R kitchencad:kitchencad /opt/kitchencad

# Copy built application from builder stage
COPY --from=builder /app/build/src/KitchenCADDesigner /opt/kitchencad/bin/
COPY --from=builder /app/resources/ /opt/kitchencad/resources/

# Set permissions
RUN chmod +x /opt/kitchencad/bin/KitchenCADDesigner

# Switch to application user
USER kitchencad
WORKDIR /home/kitchencad

# Set environment variables
ENV PATH="/opt/kitchencad/bin:${PATH}"
ENV QT_QPA_PLATFORM=xcb
ENV DISPLAY=:0

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD /opt/kitchencad/bin/KitchenCADDesigner --version || exit 1

# Default command
CMD ["/opt/kitchencad/bin/KitchenCADDesigner"]

# Build instructions and metadata
LABEL maintainer="Kitchen CAD Designer Team <support@kitchencaddesigner.com>"
LABEL version="1.0.0"
LABEL description="Professional Kitchen Design Software"
LABEL org.opencontainers.image.title="Kitchen CAD Designer"
LABEL org.opencontainers.image.description="A comprehensive CAD application for designing kitchens, bathrooms, and modular furniture"
LABEL org.opencontainers.image.version="1.0.0"
LABEL org.opencontainers.image.vendor="Kitchen CAD Designer Team"

# Usage instructions in the image
RUN echo "Kitchen CAD Designer Docker Image" > /home/kitchencad/README.txt && \
    echo "=================================" >> /home/kitchencad/README.txt && \
    echo "" >> /home/kitchencad/README.txt && \
    echo "To run with X11 forwarding:" >> /home/kitchencad/README.txt && \
    echo "docker run -it --rm -e DISPLAY=\$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix kitchencad" >> /home/kitchencad/README.txt && \
    echo "" >> /home/kitchencad/README.txt && \
    echo "To run with volume for projects:" >> /home/kitchencad/README.txt && \
    echo "docker run -it --rm -e DISPLAY=\$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v \$(pwd)/projects:/home/kitchencad/projects kitchencad" >> /home/kitchencad/README.txt