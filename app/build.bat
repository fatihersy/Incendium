REM Build script for app
@ECHO OFF
SetLocal EnableDelayedExpansion

REM Get a list of all the .c files.
SET cFilenames=
FOR /R %%f in (*.c) do (
    SET cFilenames=!cFilenames! %%f
)

REM echo "Files:" %cFilenames%

SET assembly=app
SET compilerFlags=-g
REM -Wall -Werror
SET includeFlags=-Isrc -I../vendor/include/
SET linkerFlags=-L../bin/ -L../vendor/lib/ -lraylib -lGdi32 -lWinMM -lUser32 -lShell32
SET linker= -Xlinker /NODEFAULTLIB:libcmt
SET defines=-D_DEBUG

ECHO "Building %assembly%%..."
clang %cFilenames% %compilerFlags% -o ../bin/%assembly%.exe %defines% %includeFlags% %linkerFlags% %linker%
