@echo off
cd /d "%~dp0"
if not exist "x64\Release\PromptLibrary.exe" (
  call build_cpp.bat
)
if exist "x64\Release\PromptLibrary.exe" (
  start "" "x64\Release\PromptLibrary.exe"
) else (
  echo PromptLibrary.exe was not found.
  pause
)
