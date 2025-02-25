
git clone https://github.com/microsoft/vcpkg.git

set ROOT=%CD%
cd vcpkg && call .\bootstrap-vcpkg.bat
set VCPKG_ROOT=%CD%
set PATH=%VCPKG_ROOT%;%PATH%
.\\vcpkg.exe integrate install
cd ..

vcpkg.exe install  --x-wait-for-lock --triplet "x64-windows-static" --vcpkg-root "%VCPKG_ROOT%\\" "--x-manifest-root=%ROOT%\\" "--x-install-root=%ROOT%\\vcpkg_installed\\x64-windows-static\\"

set VARIANT=Release
@REM assume there's some way to load these from .env
@REM set VERSION=0.9.3.0
@REM set USERNAME=user
@REM set USER_ID=1000

set TOOLCHAIN=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=%VARIANT% -DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN% -S%ROOT% -B%ROOT%/build
@REM cmake --build %ROOT%/build --config %VARIANT% --target all -j 50 --
cmake --build %ROOT%/build --config %VARIANT% --
