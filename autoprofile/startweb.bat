@echo off
cd ..
if not exist "autoprofile\.venv\Scripts\activate.bat" (
    echo Virtual environment not found. Please run install.bat first.
    pause
    exit /b 1
)
call autoprofile\.venv\Scripts\activate.bat
set PYTHONPATH=.
python -m autoprofile.cli web
pause
