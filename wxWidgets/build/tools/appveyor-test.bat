echo.
echo --- Running tests.
echo.

cd c:\projects\wxwidgets\tests

goto %TOOLSET%

:msbuild
PATH=C:\projects\wxwidgets\lib\vc_x64_dll;%PATH%
if "%CONFIGURATION%"=="DLL Release" set suffix=dll
if "%CONFIGURATION%"=="DLL Debug" set suffix=ddll
if "%CONFIGURATION%"=="Debug" set suffix=d
.\vc_x64_mswu%suffix%\test.exe
if %errorlevel% NEQ 0 goto :error
.\vc_x64_mswu%suffix%\test_gui.exe
goto :eof

:nmake
if "%BUILD%"=="debug" set debug_suffix=d
if "%ARCH%"=="amd64" set arch_suffix=_x64
.\vc%arch_suffix%_mswu%debug_suffix%\test.exe
if %errorlevel% NEQ 0 goto :error
.\vc%arch_suffix%_mswu%debug_suffix%\test_gui.exe
goto :eof

:mingw
.\gcc_mswud\test.exe
if %errorlevel% NEQ 0 goto :error
.\gcc_mswud\test_gui.exe
goto :eof

:msys2
PATH=C:\projects\wxwidgets\lib;%PATH%
.\test.exe
if %errorlevel% NEQ 0 goto :error
.\test_gui.exe
goto :eof

:cygwin
PATH=C:\projects\wxwidgets\lib;%PATH%
.\test.exe
.\test_gui.exe
echo.
echo --- Note: ignoring possible test failures under Cygwin
echo.
exit /b 0

:cmake_qt
PATH=%QT5DIR%\bin;%PATH%
set QT_QPA_PLATFORM_PLUGIN_PATH=%QT5DIR%\plugins\platforms
set QT_USE_NATIVE_WINDOWS=1
goto :cmake

:cmake
if "%CONFIGURATION%"=="" set CONFIGURATION=Release
if "%CMAKE_TEST_REGEX%"=="" set CMAKE_TEST_REGEX="test_drawing"
cd ..\build_cmake
ctest -V -C %CONFIGURATION% -E %CMAKE_TEST_REGEX% --output-on-failure --interactive-debug-mode 0 .
if %errorlevel% NEQ 0 goto :error
goto :eof

:error
echo.
echo !!! Non-GUI test failed.
echo.
echo --- httpbin output ---
type c:\projects\wxwidgets\httpbin.log
echo --- httpbin output end ---
exit /b 1
