winget install Git.Git

#winget install --id=Microsoft.VisualStudio.2022.BuildTools  -e

# nope
# winget install --id=Microsoft.VisualStudio.2022.BuildTools --silent --override "--wait --quiet --add ProductLang En-us --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
# seems like the right things are checked with this:
# winget install --id=Microsoft.VisualStudio.2022.BuildTools --override "--wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
# so try silent
# taking forever so not sure if it works
# winget install --id=Microsoft.VisualStudio.2022.BuildTools --silent --override "--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

#winget install --force --id=Microsoft.VisualStudio.2022.BuildTools --override "--wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

winget install --force --id=Microsoft.VisualStudio.2022.BuildTools --override "--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"


#winget install cmake


git clone https://github.com/microsoft/vcpkg.git

# seems like vscode hangs when trying to do any cmake actions because this hasn't run yet
# powershell
# cd vcpkg; .\bootstrap-vcpkg.bat
# $env:VCPKG_ROOT = "%CWD%"
# $env:PATH = "$env:VCPKG_ROOT;$env:PATH"
# cmd
cd vcpkg && .\bootstrap-vcpkg.bat
set VCPKG_ROOT = "%CWD%"
set PATH = "%VCPKG_ROOT%;%PATH%"
cd .. && cmake --preset=default

# # create files as per https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-powershell
# # getting:
# # cmake --preset=default
# #   CMake Error: CMake was unable to find a build program corresponding to "Ninja".  CMAKE_MAKE_PROGRAM is not set.  You probably need to select a different build tool.
# # try uninstall cmake since it wasn't part of other setup guide
# winget remove cmake
# # now not found

# # now try using instructions from https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/cmake-presets.md
# # - nope

# # https://code.visualstudio.com/docs/cpp/config-msvc

.\\vcpkg.exe integrate install

"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -DCMAKE_TOOLCHAIN_FILE=/scripts/buildsystems/vcpkg.cmake -SC:/nrcan/FireSTARR/tbd -BC:/nrcan/FireSTARR/tbd/build -G Ninja
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build C:/nrcan/FireSTARR/tbd/build --parallel 50 --

cmake --preset=default
cmake.exe -DCMAKE_TOOLCHAIN_FILE=/scripts/buildsystems/vcpkg.cmake -SC:/nrcan/FireSTARR/tbd -BC:/nrcan/FireSTARR/tbd/build -G Ninja
cmake.exe --build C:/nrcan/FireSTARR/tbd/build --parallel 50 --

# specifically for firestarr

cd \nrcan\FireSTARR\data\generated\grid\100m
mklink /D default 2024
