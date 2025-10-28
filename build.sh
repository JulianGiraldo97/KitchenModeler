#!/bin/bash

# Kitchen CAD Designer Build Script
# This script automates the build process for different platforms

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
RUN_TESTS=false
INSTALL=false
CLEAN=false

# Function to show usage
show_usage() {
    echo "Kitchen CAD Designer Build Script"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -d, --debug         Build in Debug mode (default: Release)"
    echo "  -j, --jobs N        Number of parallel jobs (default: auto-detected)"
    echo "  -t, --test          Run tests after building"
    echo "  -i, --install       Install after building"
    echo "  -c, --clean         Clean build directory before building"
    echo "  --build-dir DIR     Specify build directory (default: build)"
    echo ""
    echo "Examples:"
    echo "  $0                  # Basic release build"
    echo "  $0 -d -t            # Debug build with tests"
    echo "  $0 -j 8 -i          # Release build with 8 jobs and install"
    echo "  $0 -c -d            # Clean debug build"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        -i|--install)
            INSTALL=true
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check prerequisites
print_status "Checking prerequisites..."

if ! command_exists cmake; then
    print_error "CMake is not installed. Please install CMake 3.20 or later."
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
print_status "Found CMake version: $CMAKE_VERSION"

if ! command_exists make && ! command_exists ninja; then
    print_error "Neither make nor ninja build system found. Please install one of them."
    exit 1
fi

# Determine generator
GENERATOR=""
if command_exists ninja; then
    GENERATOR="-GNinja"
    BUILDER="ninja"
    print_status "Using Ninja build system"
else
    BUILDER="make"
    print_status "Using Make build system"
fi

# Clean build directory if requested
if [ "$CLEAN" = true ]; then
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
print_status "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure project
print_status "Configuring project (Build Type: $BUILD_TYPE)..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    $GENERATOR \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ $? -ne 0 ]; then
    print_error "CMake configuration failed!"
    exit 1
fi

print_success "Configuration completed successfully"

# Build project
print_status "Building project with $PARALLEL_JOBS parallel jobs..."
if [ "$BUILDER" = "ninja" ]; then
    ninja -j "$PARALLEL_JOBS"
else
    make -j "$PARALLEL_JOBS"
fi

if [ $? -ne 0 ]; then
    print_error "Build failed!"
    exit 1
fi

print_success "Build completed successfully"

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    print_status "Running tests..."
    if command_exists ctest; then
        ctest --output-on-failure --parallel "$PARALLEL_JOBS"
        if [ $? -eq 0 ]; then
            print_success "All tests passed"
        else
            print_warning "Some tests failed"
        fi
    else
        print_warning "CTest not available, skipping tests"
    fi
fi

# Install if requested
if [ "$INSTALL" = true ]; then
    print_status "Installing..."
    if [ "$BUILDER" = "ninja" ]; then
        ninja install
    else
        make install
    fi
    
    if [ $? -eq 0 ]; then
        print_success "Installation completed successfully"
    else
        print_error "Installation failed"
        exit 1
    fi
fi

# Show final status
print_success "Build process completed!"
print_status "Executable location: $BUILD_DIR/src/KitchenCADDesigner"

if [ "$BUILD_TYPE" = "Debug" ]; then
    print_warning "This is a debug build. Performance may be reduced."
fi

echo ""
print_status "To run the application:"
echo "  cd $BUILD_DIR && ./src/KitchenCADDesigner"
echo ""
print_status "To create a package:"
echo "  cd $BUILD_DIR && cpack"