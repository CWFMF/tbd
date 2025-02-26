set VARIANT=%1
if "%VARIANT%" == "" set VARIANT=Release

cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=%VARIANT%  -S"." -B"./build"
cmake --build "./build" --config %VARIANT% --target tbd -j 50 --
