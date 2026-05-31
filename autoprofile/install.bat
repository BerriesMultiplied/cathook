@echo off
cd ..
echo Installing dependencies...
python -m venv autoprofile\.venv
call autoprofile\.venv\Scripts\activate.bat
python -m pip install --upgrade pip
pip install -r autoprofile\requirements.txt
echo Done!
pause
