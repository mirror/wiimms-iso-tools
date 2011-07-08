@echo off
if not exist bash.exe cd bin
bash.exe ./windows-uninstall.sh --cygwin
pause
