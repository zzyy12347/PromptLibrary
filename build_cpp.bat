@echo off
cd /d "%~dp0"
set MSBUILD=E:\VS\MSBuild\Current\Bin\MSBuild.exe
if not exist "%MSBUILD%" (
  echo MSBuild not found: %MSBUILD%
  pause
  exit /b 1
)
"%MSBUILD%" "PromptLibraryCpp.sln" /t:Build /p:Configuration=Release /p:Platform=x64 /m
if errorlevel 1 pause
