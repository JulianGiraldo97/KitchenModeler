# Kitchen CAD Designer

A comprehensive CAD application for designing kitchens, bathrooms, and modular furniture with 2D/3D visualization, material management, and export capabilities.

## Features

- **Interactive 2D/3D Design**: Simultaneous 2D plan and 3D perspective views
- **Modular Catalog System**: Extensive library of kitchen modules and components
- **Drag & Drop Interface**: Intuitive placement of modules and elements
- **Material Management**: Real-time material and texture application
- **Bill of Materials**: Automatic generation of material lists and cost calculations
- **CAD Export**: Export to STEP, IGES, STL, and other standard formats
- **Local Storage**: Complete offline functionality with SQLite database
- **Cross-Platform**: Native support for Windows, Linux, and macOS
- **Photorealistic Rendering**: High-quality 3D visualization

## System Requirements

### Minimum Requirements
- **OS**: Windows 10/11, Ubuntu 20.04+, or macOS 10.15+
- **RAM**: 4 GB
- **Storage**: 2 GB available space
- **Graphics**: OpenGL 3.3 compatible graphics card
- **CPU**: Dual-core processor, 2.0 GHz

### Recommended Requirements
- **OS**: Windows 11, Ubuntu 22.04+, or macOS 12+
- **RAM**: 8 GB or more
- **Storage**: 5 GB available space
- **Graphics**: Dedicated graphics card with 2GB VRAM
- **CPU**: Quad-core processor, 3.0 GHz or higher

## Dependencies

### Core Libraries
- **Qt 6.2+**: Cross-platform GUI framework
- **OpenCascade 7.6+**: 3D modeling kernel
- **SQLite 3.35+**: Local database storage
- **nlohmann/json**: JSON parsing and serialization

### Development Dependencies
- **CMake 3.20+**: Build system
- **C++20 compatible compiler**: GCC 10+, Clang 12+, or MSVC 2019+
- **Catch2 3.0+**: Testing framework (optional)

## Building from Source

### Prerequisites

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install qt6-base-dev qt6-tools-dev libqt6opengl6-dev
sudo apt install libocct-*-dev libsqlite3-dev nlohmann-json3-dev
```

#### Fedora/RHEL
```bash
sudo dnf install gcc-c++ cmake git
sudo dnf install qt6-qtbase-devel qt6-qttools-devel qt6-qtbase-private-devel
sudo dnf install opencascade-devel sqlite-devel json-devel
```

#### macOS (with Homebrew)
```bash
brew install cmake qt@6 opencascade sqlite nlohmann-json
```

#### Windows (with vcpkg)
```cmd
vcpkg install qt6 opencascade sqlite3 nlohmann-json
```

### Build Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/your-org/kitchen-cad-designer.git
   cd kitchen-cad-designer
   ```

2. **Create build directory**
   ```bash
   mkdir build && cd build
   ```

3. **Configure with CMake**
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

4. **Build the project**
   ```bash
   cmake --build . --parallel
   ```

5. **Run tests (optional)**
   ```bash
   ctest --output-on-failure
   ```

6. **Install (optional)**
   ```bash
   cmake --install . --prefix /usr/local
   ```

### Docker Build

For a reproducible build environment:

```bash
# Build the Docker image
docker build -t kitchen-cad-designer .

# Run with X11 forwarding (Linux)
docker run -it --rm \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v $(pwd)/projects:/home/kitchencad/projects \
  kitchen-cad-designer
```

## Usage

### Quick Start

1. **Launch the application**
   ```bash
   ./KitchenCADDesigner
   ```

2. **Create a new project**
   - File → New Project
   - Set room dimensions
   - Start designing!

3. **Add modules**
   - Browse the catalog panel
   - Drag modules to the design canvas
   - Adjust positions and properties

4. **Export your design**
   - File → Export
   - Choose format (STEP, IGES, STL, etc.)
   - Generate bill of materials

### Project Structure

```
kitchen-cad-designer/
├── src/                    # Source code
│   ├── main.cpp           # Application entry point
│   ├── ui/                # User interface components
│   ├── core/              # Core business logic
│   ├── geometry/          # Geometric operations
│   ├── data/              # Data access layer
│   └── export/            # Export functionality
├── tests/                 # Unit and integration tests
├── docs/                  # Documentation
├── resources/             # Application resources
│   ├── icons/            # UI icons
│   ├── catalogs/         # Default catalogs
│   └── templates/        # Project templates
├── CMakeLists.txt        # Main build configuration
├── Dockerfile            # Container build configuration
└── README.md             # This file
```

## Configuration

### User Preferences
The application stores user preferences in:
- **Linux**: `~/.config/KitchenCADDesigner/`
- **Windows**: `%APPDATA%/KitchenCADDesigner/`
- **macOS**: `~/Library/Preferences/KitchenCADDesigner/`

### Database Location
Project data is stored in:
- **Linux**: `~/.local/share/KitchenCADDesigner/projects.db`
- **Windows**: `%LOCALAPPDATA%/KitchenCADDesigner/projects.db`
- **macOS**: `~/Library/Application Support/KitchenCADDesigner/projects.db`

## Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Setup

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

### Code Style

- Follow C++20 best practices
- Use consistent naming conventions
- Document public APIs
- Write unit tests for new features

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/your-org/kitchen-cad-designer/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-org/kitchen-cad-designer/discussions)
- **Email**: support@kitchencaddesigner.com

## Roadmap

- [ ] Advanced material editor
- [ ] Plugin system for third-party extensions
- [ ] Cloud synchronization (optional)
- [ ] Mobile companion app
- [ ] VR/AR visualization support
- [ ] Collaborative design features

## Acknowledgments

- [Qt Framework](https://www.qt.io/) for the excellent GUI toolkit
- [OpenCascade](https://www.opencascade.com/) for 3D modeling capabilities
- [SQLite](https://www.sqlite.org/) for reliable local storage
- [nlohmann/json](https://github.com/nlohmann/json) for JSON handling
- [Catch2](https://github.com/catchorg/Catch2) for testing framework

---

**Kitchen CAD Designer** - Professional kitchen design made simple.