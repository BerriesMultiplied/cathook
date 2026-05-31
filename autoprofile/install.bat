@echo off
cd ..
echo Installing dependencies...
python -m venv autoprofile\.venv
call autoprofile\.venv\Scripts\activate.bat
python -m pip install --upgrade pip
python -m pip install -r autoprofile\requirements.txt
python -m playwright install chromium
echo Done!
pause
