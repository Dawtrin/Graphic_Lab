<#
.SYNOPSIS
    Script cau hinh tu dong moi truong C++ / OpenGL / GLUT tren VS Code (MinGW + CMake).
.DESCRIPTION
    Tu dong kiem tra va tao cac file cau hinh:
    - CMakeLists.txt (thu muc goc)
    - .vscode/tasks.json
    - .vscode/launch.json
    - .vscode/c_cpp_properties.json
    Sau do chay lenh `cmake -B build -G "MinGW Makefiles"` de tao he thong build.
#>

$ErrorActionPreference = "Stop"

Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host "   CAU HINH MOI TRUONG OPENGL / GLUT / CMAKE CHO LAB   " -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan

$WorkspaceRoot = $PSScriptRoot
if (-not $WorkspaceRoot) {
    $WorkspaceRoot = (Get-Location).Path
}

# 1. Kiem tra CMake va trinh bien dich MinGW
Write-Host "`n[1/4] Kiem tra cong cu he thong..." -ForegroundColor Yellow
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
$gpp = Get-Command g++ -ErrorAction SilentlyContinue

if (-not $cmake) {
    Write-Host "`n[LOI] Khong tim thay 'cmake' trong he thong!" -ForegroundColor Red
    Write-Host "Huong dan:" -ForegroundColor White
    Write-Host "  1. Tai va cai dat CMake: https://cmake.org/download/" -ForegroundColor White
    Write-Host "  2. Nho tich chon: 'Add CMake to the system PATH for all users'" -ForegroundColor Yellow
    exit 1
}

if (-not $gpp) {
    Write-Host "`n[LOI] Khong tim thay 'g++' (MinGW) trong he thong!" -ForegroundColor Red
    Write-Host "Huong dan:" -ForegroundColor White
    Write-Host "  Cai dat MinGW/MSYS2 va them thu muc 'bin' vao bien moi truong PATH." -ForegroundColor Yellow
    Write-Host "  (Vi du MSYS2: C:\msys64\ucrt64\bin)" -ForegroundColor White
    exit 1
}

$compilerPathNormalized = ($gpp.Source) -replace '\\', '/'
$compilerDir = Split-Path (Split-Path $gpp.Source -Parent) -Parent
$includePathNormalized = (Join-Path $compilerDir "include") -replace '\\', '/'

Write-Host "  -> CMake: $($cmake.Source)" -ForegroundColor Green
Write-Host "  -> C++ Compiler: $($gpp.Source)" -ForegroundColor Green
Write-Host "  -> Include Dir: $includePathNormalized" -ForegroundColor Green

# 2. Tao thu muc .vscode neu chua co
$vscodeDir = Join-Path $WorkspaceRoot ".vscode"
if (-not (Test-Path $vscodeDir)) {
    New-Item -ItemType Directory -Path $vscodeDir -Force | Out-Null
}

# 3. Tao file CMakeLists.txt neu chua co
Write-Host "`n[2/4] Kiem tra cac file cau hinh..." -ForegroundColor Yellow
$cmakeListsPath = Join-Path $WorkspaceRoot "CMakeLists.txt"
if (-not (Test-Path $cmakeListsPath)) {
    Write-Host "  -> Tao moi CMakeLists.txt..." -ForegroundColor Green
    @'
cmake_minimum_required(VERSION 3.15)
project(OpenGL_GLUT_Lab CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

if(GLUT_INCLUDE_DIRS)
    include_directories(${GLUT_INCLUDE_DIRS})
endif()
if(OPENGL_INCLUDE_DIRS)
    include_directories(${OPENGL_INCLUDE_DIRS})
endif()

file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS "*.cpp")

foreach(SOURCE_FILE ${ALL_SOURCES})
    if(SOURCE_FILE MATCHES "CMakeFiles" OR SOURCE_FILE MATCHES "[/\\\\]build[/\\\\]")
        continue()
    endif()

    get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    target_link_libraries(${TARGET_NAME} PRIVATE
        ${GLUT_LIBRARIES}
        ${OPENGL_LIBRARIES}
        glu32
        opengl32
        freeglut
    )
endforeach()
'@ | Set-Content -Path $cmakeListsPath -Encoding UTF8
} else {
    Write-Host "  -> CMakeLists.txt da ton tai. (Giu nguyen)" -ForegroundColor Gray
}

# Tao .vscode/tasks.json neu chua co
$tasksJsonPath = Join-Path $vscodeDir "tasks.json"
if (-not (Test-Path $tasksJsonPath)) {
    Write-Host "  -> Tao moi .vscode/tasks.json..." -ForegroundColor Green
    @'
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "CMake: Build Active Target",
      "type": "shell",
      "command": "cmake",
      "args": [
        "--build",
        "${workspaceFolder}/build",
        "--target",
        "${fileBasenameNoExtension}"
      ],
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": ["$gcc"],
      "presentation": {
        "echo": true,
        "reveal": "always",
        "focus": false,
        "panel": "shared"
      }
    }
  ]
}
'@ | Set-Content -Path $tasksJsonPath -Encoding UTF8
} else {
    Write-Host "  -> .vscode/tasks.json da ton tai. (Giu nguyen)" -ForegroundColor Gray
}

# Tao .vscode/launch.json neu chua co
$launchJsonPath = Join-Path $vscodeDir "launch.json"
if (-not (Test-Path $launchJsonPath)) {
    Write-Host "  -> Tao moi .vscode/launch.json..." -ForegroundColor Green
    @'
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "(gdb) Launch Active OpenGL Target",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/${fileBasenameNoExtension}.exe",
      "args": [],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb",
      "miDebuggerPath": "gdb.exe",
      "setupCommands": [
        {
          "description": "Enable pretty-printing for gdb",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ],
      "preLaunchTask": "CMake: Build Active Target"
    }
  ]
}
'@ | Set-Content -Path $launchJsonPath -Encoding UTF8
} else {
    Write-Host "  -> .vscode/launch.json da ton tai. (Giu nguyen)" -ForegroundColor Gray
}

# Tao .vscode/c_cpp_properties.json tu dong theo duong dan may hien tai
$cppPropsPath = Join-Path $vscodeDir "c_cpp_properties.json"
if (-not (Test-Path $cppPropsPath)) {
    Write-Host "  -> Tao moi .vscode/c_cpp_properties.json..." -ForegroundColor Green
    $cCppContent = @"
{
  "configurations": [
    {
      "name": "Win32",
      "includePath": [
        "`${workspaceFolder}/**",
        "$includePathNormalized/**"
      ],
      "defines": [
        "_DEBUG",
        "UNICODE",
        "_UNICODE"
      ],
      "compilerPath": "$compilerPathNormalized",
      "cStandard": "c17",
      "cppStandard": "c++17",
      "intelliSenseMode": "windows-gcc-x64",
      "compileCommands": "`${workspaceFolder}/build/compile_commands.json"
    }
  ],
  "version": 4
}
"@
    $cCppContent | Set-Content -Path $cppPropsPath -Encoding UTF8
} else {
    Write-Host "  -> .vscode/c_cpp_properties.json da ton tai. (Giu nguyen)" -ForegroundColor Gray
}

# 4. Chay khoi tao he thong build CMake
Write-Host "`n[3/4] Chay khoi tao he thong build CMake..." -ForegroundColor Yellow
$buildDir = Join-Path $WorkspaceRoot "build"
try {
    cmake -B "$buildDir" -S "$WorkspaceRoot" -G "MinGW Makefiles"
} catch {
    Write-Host "`n[LOI] CMake gap loi khi khoi tao!" -ForegroundColor Red
    Write-Host "Kiem tra xem may ban da cai thu vien FreeGLUT chua." -ForegroundColor Yellow
    Write-Host "Neu dung MSYS2 UCRT64, chay lenh: pacman -S mingw-w64-ucrt-x86_64-freeglut" -ForegroundColor Cyan
    exit 1
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n[LOI] CMake gap loi khi khoi tao (Exit code: $LASTEXITCODE)!" -ForegroundColor Red
    Write-Host "Nguyen nhan pho bien: Chua cai dat FreeGLUT tren may." -ForegroundColor Yellow
    Write-Host "Giai phap: Chay lenh sau trong terminal MSYS2:" -ForegroundColor White
    Write-Host "   pacman -S mingw-w64-ucrt-x86_64-freeglut" -ForegroundColor Cyan
    exit 1
}

Write-Host "`n[4/4] HOAN TAT CAU HINH!" -ForegroundColor Green
Write-Host "-------------------------------------------------------" -ForegroundColor Cyan
Write-Host "Cach su dung hang ngay:" -ForegroundColor White
Write-Host "  1. Mo thu muc Lab trong VS Code" -ForegroundColor White
Write-Host "  2. Mo bat ky file .cpp nao (vd: Lab_1/main.cpp)" -ForegroundColor White
Write-Host "  3. Nhan F5 de bien dich va chay ngay lap tuc!" -ForegroundColor White
Write-Host "-------------------------------------------------------" -ForegroundColor Cyan
