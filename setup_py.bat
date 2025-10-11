@echo off
rem Lab04 Setup Script (Windows)
rem This script helps set up the development environment for the project

setlocal enabledelayedexpansion

set "PROJECT_ROOT=%~dp0"
set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"
set "VENV_PATH=%PROJECT_ROOT%\venv"
set "ALT_VENV_PATH=%PROJECT_ROOT%\.venv"
set "PYTHON_CMD="
set "make_venv=0"

echo === Lab04 Project Setup ===
echo Project root: %PROJECT_ROOT%
echo.

rem Check for Python
where python >nul 2>&1
if %errorlevel% == 0 (
    for /f "tokens=*" %%i in ('python --version 2^>^&1') do set "python_version=%%i"
    echo !python_version! | findstr /R "^Python 3\." >nul
    if !errorlevel! == 0 (
        set "PYTHON_CMD=python"
        echo Python 3 found: !python_version!
    ) else (
        echo Warning: Found Python but not version 3
    )
)

if "%PYTHON_CMD%"=="" (
    where python3 >nul 2>&1
    if !errorlevel! == 0 (
        for /f "tokens=*" %%i in ('python3 --version 2^>^&1') do set "python_version=%%i"
        set "PYTHON_CMD=python3"
        echo Python 3 found: !python_version!
    ) else (
        echo Error: Python 3 not found. Consider installing to use visualization features.
        echo Download from: https://www.python.org/downloads/
        pause
        exit /b 1
    )
)

rem Check for virtual environment
set "venv_exists=0"
if exist "%VENV_PATH%" (
    echo Virtual environment already exists at %VENV_PATH%
    set "venv_exists=1"
) else if exist "%ALT_VENV_PATH%" (
    echo Virtual environment already exists at %ALT_VENV_PATH%
    set "venv_exists=1"
)

if %venv_exists% == 1 (
    set /p "recreate=Recreate it? [y/N]: "
    if /i "!recreate!"=="y" (
        echo Removing existing virtual environment...
        if exist "%VENV_PATH%" rmdir /s /q "%VENV_PATH%"
        if exist "%ALT_VENV_PATH%" rmdir /s /q "%ALT_VENV_PATH%"
        set "make_venv=1"
    ) else (
        echo Using existing virtual environment.
        set "make_venv=0"
    )
) else (
    set /p "create_venv=No existing virtual environment found. Create one? [y/N]: "
    if /i "!create_venv!"=="n" (
        echo Skipping virtual environment creation.
        echo Pip may not be available, thus some features may not work.
        set "make_venv=0"
    ) else (
        set "make_venv=1"
    )
)

rem Create virtual environment if needed
if %make_venv% == 1 (
    echo Creating Python virtual environment...
    %PYTHON_CMD% -m venv "%VENV_PATH%"
    if !errorlevel! == 0 (
        echo Virtual environment created
    ) else (
        echo Error: Failed to create virtual environment
        pause
        exit /b 1
    )
)

rem Activate and install dependencies
echo Activating virtual environment and installing dependencies...

set "ACTIVATE_SCRIPT="
if exist "%VENV_PATH%\Scripts\activate.bat" (
    set "ACTIVATE_SCRIPT=%VENV_PATH%\Scripts\activate.bat"
) else if exist "%ALT_VENV_PATH%\Scripts\activate.bat" (
    set "ACTIVATE_SCRIPT=%ALT_VENV_PATH%\Scripts\activate.bat"
)

if "%ACTIVATE_SCRIPT%"=="" (
    echo Warning: Could not find activation script. Installing packages globally...
    set "PIP_CMD=%PYTHON_CMD% -m pip"
) else (
    call "%ACTIVATE_SCRIPT%"
    set "PIP_CMD=pip"
)

rem Install required Python packages
echo Installing/upgrading pip...
%PIP_CMD% install --upgrade pip
if %errorlevel% neq 0 (
    echo Warning: Failed to upgrade pip
)

echo Installing required packages...
%PIP_CMD% install matplotlib networkx numpy pyqt5 pyqtgraph scipy imageio pandas
if %errorlevel% neq 0 (
    echo Error: Failed to install core packages
    pause
    exit /b 1
)

echo.
echo Setup complete!
echo.
if "%ACTIVATE_SCRIPT%" neq "" (
    echo To activate the virtual environment in the future, run:
    echo %ACTIVATE_SCRIPT%
    echo.
    echo Or use: %VENV_PATH%\Scripts\activate
) else (
    echo Virtual environment activation failed. Packages were installed globally.
)
echo.
echo If you encountered any issues, please refer to the README.md for troubleshooting tips.
echo.
pause