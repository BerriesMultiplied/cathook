@echo off
echo Stopping autoprofile...
powershell -Command "Get-WmiObject Win32_Process | Where-Object { $_.CommandLine -match 'autoprofile\.cli' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }"
echo Done!
pause
