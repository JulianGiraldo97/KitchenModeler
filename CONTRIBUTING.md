# Contributing to Kitchen CAD Designer

Thank you for your interest in contributing to Kitchen CAD Designer! This document provides guidelines and information for contributors.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Submitting Changes](#submitting-changes)
- [Testing](#testing)
- [Documentation](#documentation)

## Code of Conduct

This project adheres to a code of conduct that we expect all contributors to follow. Please be respectful and constructive in all interactions.

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/your-username/kitchen-cad-designer.git
   cd kitchen-cad-designer
   ```
3. **Add the upstream remote**:
   ```bash
   git remote add upstream https://github.com/original-org/kitchen-cad-designer.git
   ```

## Development Setup

### Prerequisites

Ensure you have all the required dependencies installed as described in the [README.md](README.md).

### Building for Development

1. **Create a debug build**:
   ```bash
   ./build.sh --debug --test
   ```

2. **Or use the manual approach**:
   ```bash
   mkdir build-debug && cd build-debug
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   cmake --build . --parallel
   ```

### IDE Setup

#### Visual Studio Code
1. Install the C/C++ extension
2. Install the CMake Tools extension
3. Open the project folder
4. Configure CMake when prompted

#### CLion
1. Open the project folder
2. CLion will automatically detect the CMakeLists.txt
3. Configure the build profiles as needed

#### Qt Creator
1. Open CMakeLists.txt as a project
2. Configure the build settings
3. Set up the run configuration

## Coding Standards

### C++ Guidelines

- **Standard**: Use C++20 features when appropriate
- **Naming Conventions**:
  - Classes: `PascalCase` (e.g., `GeometryEngine`)
  - Functions/Methods: `camelCase` (e.g., `createBox`)
  - Variables: `camelCase` (e.g., `objectId`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_OBJECTS`)
  - Private members: trailing underscore (e.g., `data_`)

- **Code Style**:
  - Use 4 spaces for indentation (no tabs)
  - Maximum line length: 100 characters
  - Always use braces for control structures
  - Place opening braces on the same line

### Example Code Style

```cpp
class GeometryEngine {
private:
    std::unique_ptr<Shape3D> currentShape_;
    
public:
    GeometryEngine() = default;
    ~GeometryEngine() = default;
    
    std::unique_ptr<Shape3D> createBox(const Point3D& origin, 
                                       double width, 
                                       double height, 
                                       double depth) {
        if (width <= 0 || height <= 0 || depth <= 0) {
            throw std::invalid_argument("Dimensions must be positive");
        }
        
        // Implementation here
        return std::make_unique<BoxShape>(origin, width, height, depth);
    }
};
```

### Qt-Specific Guidelines

- Use Qt's naming conventions for Qt-specific code
- Prefer Qt containers when interfacing with Qt APIs
- Use Qt's signal/slot mechanism appropriately
- Always use `Q_OBJECT` macro for classes with signals/slots

### CMake Guidelines

- Use modern CMake (3.20+) features
- Prefer target-based commands over directory-based
- Use `find_package` for dependencies when possible
- Keep CMakeLists.txt files clean and well-commented

## Submitting Changes

### Branch Naming

Use descriptive branch names:
- `feature/add-material-editor`
- `bugfix/fix-export-crash`
- `refactor/improve-geometry-engine`
- `docs/update-build-instructions`

### Commit Messages

Follow conventional commit format:

```
type(scope): brief description

Longer description if needed.

Fixes #123
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

Examples:
```
feat(geometry): add boolean operations support

Implement union, difference, and intersection operations
using OpenCascade's boolean algorithms.

Fixes #45
```

### Pull Request Process

1. **Update your branch** with the latest upstream changes:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Ensure all tests pass**:
   ```bash
   ./build.sh --debug --test
   ```

3. **Create a pull request** with:
   - Clear title and description
   - Reference to related issues
   - Screenshots for UI changes
   - Performance impact notes if applicable

4. **Address review feedback** promptly and professionally

## Testing

### Writing Tests

- Write unit tests for new functionality
- Use Catch2 testing framework
- Place tests in the `tests/` directory
- Follow the naming convention: `test_<component>.cpp`

### Test Structure

```cpp
#include <catch2/catch_test_macros.hpp>
#include "geometry/GeometryEngine.h"

TEST_CASE("GeometryEngine box creation", "[geometry]") {
    GeometryEngine engine;
    
    SECTION("Valid dimensions") {
        auto box = engine.createBox({0, 0, 0}, 1.0, 2.0, 3.0);
        REQUIRE(box != nullptr);
        
        auto bbox = box->getBoundingBox();
        REQUIRE(bbox.size().x == Approx(1.0));
        REQUIRE(bbox.size().y == Approx(2.0));
        REQUIRE(bbox.size().z == Approx(3.0));
    }
    
    SECTION("Invalid dimensions") {
        REQUIRE_THROWS_AS(
            engine.createBox({0, 0, 0}, -1.0, 2.0, 3.0),
            std::invalid_argument
        );
    }
}
```

### Running Tests

```bash
# Run all tests
./build.sh --test

# Run specific test
cd build && ctest -R "test_geometry"

# Run tests with verbose output
cd build && ctest --verbose
```

## Documentation

### Code Documentation

- Use Doxygen-style comments for public APIs
- Document complex algorithms and design decisions
- Include usage examples for public interfaces

```cpp
/**
 * @brief Creates a 3D box geometry
 * 
 * @param origin The corner point of the box
 * @param width Box width (X dimension)
 * @param height Box height (Y dimension) 
 * @param depth Box depth (Z dimension)
 * @return Unique pointer to the created box shape
 * 
 * @throws std::invalid_argument if any dimension is <= 0
 * 
 * @example
 * ```cpp
 * GeometryEngine engine;
 * auto box = engine.createBox({0, 0, 0}, 1.0, 2.0, 3.0);
 * ```
 */
std::unique_ptr<Shape3D> createBox(const Point3D& origin, 
                                   double width, 
                                   double height, 
                                   double depth);
```

### User Documentation

- Update README.md for user-facing changes
- Add examples to the docs/ directory
- Update build instructions if dependencies change

## Performance Considerations

- Profile performance-critical code
- Use appropriate data structures and algorithms
- Consider memory usage in geometry operations
- Optimize rendering pipeline when possible

## Platform-Specific Notes

### Windows
- Test with both MSVC and MinGW if possible
- Ensure proper DLL handling
- Test installer generation

### Linux
- Test on multiple distributions
- Verify AppImage generation
- Check desktop integration

### macOS
- Test on both Intel and Apple Silicon
- Verify app bundle creation
- Check code signing requirements

## Getting Help

- **GitHub Issues**: For bug reports and feature requests
- **GitHub Discussions**: For questions and general discussion
- **Code Review**: Don't hesitate to ask for clarification during reviews

## Recognition

Contributors will be recognized in:
- CONTRIBUTORS.md file
- Release notes for significant contributions
- About dialog in the application

Thank you for contributing to Kitchen CAD Designer!