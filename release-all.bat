@ECHO OFF
REM Build Everything

ECHO "Building everything..."

REM App
compiledb make -f "Makefile.app.windows.release.mak" all
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && exit)

ECHO "All assemblies built successfully."
