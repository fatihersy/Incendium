@ECHO OFF
REM Build Everything

ECHO "Building everything..."

PUSHD app
CALL build.bat
POPD
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit)

ECHO "All assemblies built successfully."