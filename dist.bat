@echo off
rem ---------------------------------------------------------------------------
rem  Raven - dist.bat
rem  Builds what is given to people who use Raven without building it: for
rem  Visual C++ and for MinGW-w64, the static library and raven.dll (Release),
rem  the headers, the designer, the bindings and the documentation, each in
rem  one folder and one zip.
rem
rem    dist.bat [msvc | mingw | all] [options] [-- extra cmake arguments]
rem
rem    msvc                Visual C++ only (the newest Visual Studio installed)
rem    mingw               MinGW-w64 only (gcc and mingw32-make must be in the PATH)
rem    all                 both; a compiler that is not installed is skipped (default)
rem    --edition NAME      license-free (default: every window shows a watermark)
rem                        or release (licensed: no watermark, see
rem                        https://oscar-soirey.github.io/Raven/license.html)
rem    --no-test           do not run the tests
rem    --no-pause          do not wait for a key when something fails
rem
rem  Result:
rem    dist\raven-<version>-windows-x64-msvc-<edition>\
rem        include\                 Raven.h, raven\*.h, raven_web.h
rem        bin\raven.dll            and raven_web.dll, raven_designer.exe
rem        lib\raven.lib            what a program links with to use raven.dll (raven_web.lib: raven_web.dll)
rem        lib\static\raven.lib     the static library (and raven_web.lib)
rem        lib\cmake\raven          find_package(raven) for the DLL
rem        lib\static\cmake\raven   find_package(raven) for the static library
rem        share\raven\             README.md, docs\, bindings\
rem    dist\raven-<version>-windows-x64-msvc-<edition>.zip
rem    dist\raven-<version>-windows-x64-mingw-<edition>\  (libraven.dll.a, static\libraven.a)
rem    dist\raven-<version>-windows-x64-mingw-<edition>.zip
rem
rem  The libraries of one compiler are not for the other: a program built with
rem  Visual C++ takes the msvc package, one built with MinGW the mingw package.
rem  raven.dll itself can be loaded by anything (Python, C#, Rust...).
rem ---------------------------------------------------------------------------
setlocal EnableExtensions

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "WHAT=all"
set "TEST=1"
set "NO_PAUSE=0"
set "EXTRA="
set "EDITION=LicenseFree"
set "SUFFIX=license-free"
if defined RAVEN_NO_PAUSE set "NO_PAUSE=1"

:parse
if "%~1"=="" goto parsed
if /I "%~1"=="msvc"       ( set "WHAT=msvc" & shift & goto parse )
if /I "%~1"=="mingw"      ( set "WHAT=mingw" & shift & goto parse )
if /I "%~1"=="all"        ( set "WHAT=all" & shift & goto parse )
if /I "%~1"=="--edition"  ( set "EDITION_ARG=%~2" & shift & shift & goto edition )
if /I "%~1"=="--no-test"  ( set "TEST=0" & shift & goto parse )
if /I "%~1"=="--no-pause" ( set "NO_PAUSE=1" & shift & goto parse )
if "%~1"=="--"            ( shift & goto extra )
echo dist.bat: unknown option %1
goto fail

:edition
if /I "%EDITION_ARG%"=="release"      ( set "EDITION=Release" & set "SUFFIX=release" & goto parse )
if /I "%EDITION_ARG%"=="license-free" ( set "EDITION=LicenseFree" & set "SUFFIX=license-free" & goto parse )
if /I "%EDITION_ARG%"=="licensefree"  ( set "EDITION=LicenseFree" & set "SUFFIX=license-free" & goto parse )
if /I "%EDITION_ARG%"=="free"         ( set "EDITION=LicenseFree" & set "SUFFIX=license-free" & goto parse )
echo dist.bat: unknown edition %EDITION_ARG% (license-free or release)
goto fail

:extra
if "%~1"=="" goto parsed
set "EXTRA=%EXTRA% %1"
shift
goto extra

:parsed
where cmake >nul 2>nul
if errorlevel 1 (
	echo dist.bat: cmake was not found in the PATH. Install CMake 3.16 or newer: https://cmake.org/download/
	goto fail
)

set "VERSION=0.0.0"
for /f "tokens=3" %%v in ('findstr /r /c:"^project.raven VERSION" "%ROOT%\CMakeLists.txt"') do set "VERSION=%%v"

set "BUILT=0"

if /I "%WHAT%"=="mingw" goto after_msvc
call :package msvc
if errorlevel 2 goto fail
:after_msvc

if /I "%WHAT%"=="msvc" goto after_mingw
call :package mingw
if errorlevel 2 goto fail
:after_mingw

if "%BUILT%"=="0" (
	echo dist.bat: nothing was built: no compiler was found.
	goto fail
)
echo.
echo Done (%EDITION% edition): see %ROOT%\dist
endlocal
exit /b 0

rem ---------------------------------------------------------------------------
rem  :package <msvc|mingw>
rem  Returns 0 when the package was made, 1 when the compiler is not installed
rem  and "all" was asked (skipped), 2 on a failure.
rem ---------------------------------------------------------------------------
:package
set "TOOL=%~1"
set "NAME=raven-%VERSION%-windows-x64-%TOOL%-%SUFFIX%"
set "OUT=%ROOT%\dist\%NAME%"

if /I "%TOOL%"=="mingw" (
	where gcc >nul 2>nul
	if errorlevel 1 goto no_compiler
	where mingw32-make >nul 2>nul
	if errorlevel 1 goto no_compiler
)

echo.
echo ===========================================================================
echo  %NAME%
echo ===========================================================================
if exist "%OUT%" rmdir /s /q "%OUT%"
if exist "%OUT%.zip" del /q "%OUT%.zip"

rem the static library last: the designer that stays in bin\ is the one that needs no DLL
call :variant shared ON lib
if errorlevel 1 goto package_failed
call :variant static OFF lib/static
if errorlevel 1 goto package_failed

pushd "%ROOT%\dist"
cmake -E tar cf "%NAME%.zip" --format=zip "%NAME%"
if errorlevel 1 ( popd & goto package_failed )
popd
echo dist\%NAME%.zip
set "BUILT=1"
exit /b 0

:no_compiler
if /I "%WHAT%"=="all" (
	echo.
	echo dist.bat: MinGW-w64 was not found in the PATH ^(gcc, mingw32-make^): its package is skipped.
	exit /b 1
)
echo dist.bat: MinGW-w64 was not found in the PATH ^(gcc, mingw32-make^).
exit /b 2

:package_failed
if /I "%WHAT%"=="all" if /I "%TOOL%"=="msvc" if "%CONFIGURED%"=="0" (
	echo.
	echo dist.bat: Visual C++ could not be used: its package is skipped.
	exit /b 1
)
exit /b 2

rem ---------------------------------------------------------------------------
rem  :variant <name> <shared ON|OFF> <library folder>
rem ---------------------------------------------------------------------------
:variant
set "KIND=%~1"
set "BUILD=%ROOT%\build\dist-%TOOL%-%KIND%-%SUFFIX%"
set "CONFIGURED=0"
echo.
echo -- %TOOL%, %KIND% library, %EDITION% edition

if /I "%TOOL%"=="mingw" (
	cmake -S "%ROOT%" -B "%BUILD%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=%~2 -DRAVEN_EDITION=%EDITION% -DRAVEN_BUILD_EXAMPLES=OFF "-DCMAKE_INSTALL_PREFIX=%OUT%" -DCMAKE_INSTALL_LIBDIR=%~3 %EXTRA%
) else (
	cmake -S "%ROOT%" -B "%BUILD%" -A x64 -DRAVEN_BUILD_SHARED=%~2 -DRAVEN_EDITION=%EDITION% -DRAVEN_BUILD_EXAMPLES=OFF "-DCMAKE_INSTALL_PREFIX=%OUT%" -DCMAKE_INSTALL_LIBDIR=%~3 %EXTRA%
)
if errorlevel 1 exit /b 1
set "CONFIGURED=1"

cmake --build "%BUILD%" --config Release --parallel
if errorlevel 1 exit /b 1

if "%TEST%"=="1" (
	pushd "%BUILD%"
	ctest -C Release --output-on-failure
	if errorlevel 1 ( popd & exit /b 1 )
	popd
)

cmake --install "%BUILD%" --config Release
if errorlevel 1 exit /b 1
exit /b 0

:fail
if "%NO_PAUSE%"=="0" pause
endlocal
exit /b 1
