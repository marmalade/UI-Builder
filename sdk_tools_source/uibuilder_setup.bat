@echo off
setlocal
IF EXIST "%S3E_DIR%\python\python.exe" SET pythonpath=%S3E_DIR%\python\
"%pythonpath%python" uibuilder_setup.py
