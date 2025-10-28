@echo off
REM Kitchen CAD Designer Build Script for Windows
REM This script automates the build process on Windows

setlocal enabledelayedexpansion

REM Default values
set BUILD_TYPE=Release
set BUILD_DIR=build
set PARALLEL_JOBS=%NUMBER_OF_PROCESSORS%
set RUN_TESTS=false
set INSTALL=false
set CLEAN=false
set GENERATOR=

REM Colors (if supported)
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "BLUE=[94m"
set "NC=[0m"

REM Function to print status (simulated with goto)
:print_status
echo %BLUE%[INFO]%NC% %~1
goto :eof

:print_success
echo %GREEN%[SUCCESS]%NC% %~1
goto :eof

:print_warning
echo %YELLOW%[WARNING]%NC% %~1
goto :eof

:print_error
echo %RED%[ERROR]%NC% %~1
goto :eof

:show_usage
echo Kitchen CAD Designer Build Script for Windows
echo.
echo Usage: %0 [OPTIONS]
echo.
echo Options:
echo   /h, /help          Show this help message
echo   /d, /debug         Build in Debug mode (default: Release)
echo   /j N               Number of parallel jobs (default: auto-detected)
echo   /t, /test          Run tests after building
echo   /i, /install       Install after building
echo   /c, /clean         Clean build directory before building
echo   /vs                Use Visual Studio generator
echo   /ninja             Use Ninja generator (if available)
echo.
echo Examples:
echo   %0                 # Basic release build
echo   %0 /d /t           # Debug build with tests
echo   %0 /j 8 /i         # Release build with 8 jobs and install
echo   %0 /c /d /vs       # Clean debug build with Visual Studio
goto :eof

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :check_prereq
if /i "%~1"=="/h" goto :usage
if /i "%~1"=="/help" goto :usage
if /i "%~1"=="/d" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="/debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="/j" (
    set PARALLEL_JOBS=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="/t" (
    set RUN_TESTS=true
    shift
    goto :parse_args
)
if /i "%~1"=="/test" (
    set RUN_TESTS=true
    shift
    goto :parse_args
)
if /i "%~1"=="/i" (
    set INSTALL=true
    shift
    goto :parse_args
)
if /i "%~1"=="/install" (
    set INSTALL=true
    shift
    goto :parse_args
)
if /i "%~1"=="/c" (
    set CLEAN=true
    shift
    goto :parse_args
)
if /i "%~1"=="/clean" (
    set CLEAN=true
    shift
    goto :parse_args
)
if /i "%~1"=="/vs" (
    set GENERATOR=-G "Visual Studio 17 2022"
    shift
    goto :parse_args
)
if /i "%~1"=="/ninja" (
    set GENERATOR=-GNinja
    shift
    goto :parse_args
)
call :print_error "Unknown option: %~1"
goto :usage

:usage
call :show_usage
exit /b 1

REM Start of main script
call :parse_args %*

:check_prereq
call :print_status "Checking prerequisites..."

REM Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    call :print_error "CMake is not installed or not in PATH. Please install CMake 3.20 or later."
    exit /b 1
)

for /f "tokens=3" %%i in ('cmake --version ^| findstr /r "cmake version"') do set CMAKE_VERSION=%%i
call :print_status "Found CMake version: !CMAKE_VERSION!"

REM Auto-detect generator if not specified
if "!GENERATOR!"=="" (
    ninja --version >nul 2>&1
    if not errorlevel 1 (
        set GENERATOR=-GNinja
        call :print_status "Using Ninja build system"
    ) else (
        REM Try to detect Visual Studio
        where cl >nul 2>&1
        if not errorlevel 1 (
            set GENERATOR=-G "Visual Studio 17 2022"
            call :print_status "Using Visual Studio 2022 generator"
        ) else (
            call :print_status "Using default generator"
        )
    )
)

REM Clean build directory if requested
if "!CLEAN!"=="true" (
    call :print_status "Cleaning build directory..."
    if exist "!BUILD_DIR!" rmdir /s /q "!BUILD_DIR!"
)

REM Create build directory
call :print_status "Creating build directory: !BUILD_DIR!"
if not exist "!BUILD_DIR!" mkdir "!BUILD_DIR!"
cd "!BUILD_DIR!"

REM Configure project
call :print_status "Configuring project (Build Type: !BUILD_TYPE!)..."
cmake .. -DCMAKE_BUILD_TYPE=!BUILD_TYPE! !GENERATOR! -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if errorlevel 1 (
    call :print_error "CMake configuration failed!"
    exit /b 1
)

call :print_success "Configuration completed successfully"

REM Build project
call :print_status "Building project with !PARALLEL_JOBS! parallel jobs..."
cmake --build . --config !BUILD_TYPE! --parallel !PARALLEL_JOBS!

if errorlevel 1 (
    call :print_error "Build failed!"
    exit /b 1
)

call :print_success "Build completed successfully"

REM Run tests if requested
if "!RUN_TESTS!"=="true" (
    call :print_status "Running tests..."
    ctest --output-on-failure --parallel !PARALLEL_JOBS! --build-config !BUILD_TYPE!
    if errorlevel 1 (
        call :print_warning "Some tests failed"
    ) else (
        call :print_success "All tests passed"
    )
)

REM Install if requested
if "!INSTALL!"=="true" (
    call :print_status "Installing..."
    cmake --install . --config !BUILD_TYPE!
    
    if errorlevel 1 (
        call :print_error "Installation failed"
        exit /b 1
    ) else (
        call :print_success "Installation completed successfully"
    )
)

REM Show final status
call :print_success "Build process completed!"
call :print_status "Executable location: !BUILD_DIR!\src\!BUILD_TYPE!\KitchenCADDesigner.exe"

if "!BUILD_TYPE!"=="Debug" (
    call :print_warning "This is a debug build. Performance may be reduced."
)

echo.
call :print_status "To run the application:"
echo   cd !BUILD_DIR! ^&^& .\src\!BUILD_TYPE!\KitchenCADDesigner.exe
echo.
call :print_status "To create a package:"
echo   cd !BUILD_DIR! ^&^& cpack

endlocal