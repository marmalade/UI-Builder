@echo off
echo.
echo That batch file could help you to create an executible UI Builder
echo.

setlocal

cd %~dp1
call "%S3E_DIR%\bin\mkb" %~dp0\ASD2_0\ASUIEditor\ASUIEditor.mkb --release --clean --rebuild --make --non-interactive --nologo --no-ide

call "%S3E_DIR%\bin\mkb" %~dp0\ASD2_0\ASUIEditor\iwuiviewer\iwuiviewer.mkb --debug --clean --rebuild --make --non-interactive --nologo --no-ide

