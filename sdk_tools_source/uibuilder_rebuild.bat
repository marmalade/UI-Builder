@echo off
echo.
echo That batch file could help you to create an executible UI Builder
echo.
echo Run it without parameters to create Relese version of UI Builder
echo Run it with parameter "-d" to create Debug version of UI Builder
echo.

setlocal
set option=.mkb
if "%1"=="-d" set option=Debug.mkb

set list=%~dp0\ASD2_0\ASUIEditor\ASUIEditor%option%^
         %~dp0\ASD2_0\ASUIEditor\iwuiviewer\iwuiviewer.mkb

if "%1"=="-d" goto build_debug

for %%A in (%list%) do call :mkb_rebuild_release %%A

goto:eof


:build_debug
for %%A in (%list%) do call :mkb_rebuild_debug %%A
goto:eof


:mkb_rebuild_release
cd %~dp1
call "%S3E_DIR%\bin\mkb" %1 --release --clean --rebuild --make --non-interactive --nologo --no-ide
exit /b

:mkb_rebuild_debug
cd %~dp1
call "%S3E_DIR%\bin\mkb" %1 --debug --clean --rebuild --make --non-interactive --nologo --no-ide
exit /b

