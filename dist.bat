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
rem    --keys FOLDER       the license keys (raven_license keygen); by default
rem                        %RAVEN_LICENSE_KEYS%, then %USERPROFILE%\.raven-license
rem    --no-test           do not run the tests
rem    --no-pause          do not wait for a key when something fails
rem
rem  Each library is there twice: the license-free edition (a watermark in
rem  every window) and the Release edition, encrypted beside it (<file>.rvnenc).
rem  A program configured with -DRAVEN_LICENSE_FILE=<license> gets the Release
rem  edition: find_package(raven) checks the license with
rem  bin\raven_license_check.exe and decrypts it.
rem
rem  Result:
rem    dist\raven-<version>-windows-x64-msvc\
rem        include\                 Raven.h, raven\*.h, raven_web.h
rem        bin\raven.dll            and raven.dll.rvnenc (web pages included), raven_designer.exe, raven_license_check.exe
rem        lib\raven.lib            what a program links with to use raven.dll
rem        lib\static\raven.lib     the static library, raven.lib.rvnenc
rem        lib\cmake\raven          find_package(raven) for the DLL
rem        lib\static\cmake\raven   find_package(raven) for the static library
rem        share\raven\             README.md, docs\, bindings\
rem    dist\raven-<version>-windows-x64-msvc.zip
rem    dist\raven-<version>-windows-x64-mingw\  (libraven.dll.a, static\libraven.a)
rem    dist\raven-<version>-windows-x64-mingw.zip
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
set "KEYS="
if defined RAVEN_NO_PAUSE set "NO_PAUSE=1"

:parse
if "%~1"=="" goto parsed
if /I "%~1"=="msvc"       ( set "WHAT=msvc" & shift & goto parse )
if /I "%~1"=="mingw"      ( set "WHAT=mingw" & shift & goto parse )
if /I "%~1"=="all"        ( set "WHAT=all" & shift & goto parse )
if /I "%~1"=="--keys"     ( set "KEYS=%~f2" & shift & shift & goto parse )
if /I "%~1"=="--no-test"  ( set "TEST=0" & shift & goto parse )
if /I "%~1"=="--no-pause" ( set "NO_PAUSE=1" & shift & goto parse )
if "%~1"=="--"            ( shift & goto extra )
echo dist.bat: unknown option %1
goto fail

:extra
if "%~1"=="" goto parsed
set "EXTRA=%EXTRA% %1"
shift
goto extra

:parsed
if not defined KEYS if defined RAVEN_LICENSE_KEYS set "KEYS=%RAVEN_LICENSE_KEYS%"
if not defined KEYS set "KEYS=%USERPROFILE%\.raven-license"
if not exist "%KEYS%\unlock.key" (
	echo dist.bat: no unlock key in "%KEYS%": run raven_license keygen ^(once^), or give --keys.
	goto fail
)
if not exist "%ROOT%\tools\license\public.key" (
	echo dist.bat: tools\license\public.key is missing: copy it from "%KEYS%".
	goto fail
)
fc /b "%ROOT%\tools\license\public.key" "%KEYS%\public.key" >nul 2>nul
if errorlevel 1 (
	echo dist.bat: tools\license\public.key is not the public key of "%KEYS%": the licenses would not be accepted.
	goto fail
)
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
echo Done: see %ROOT%\dist
endlocal
exit /b 0

rem ---------------------------------------------------------------------------
rem  :package <msvc|mingw>
rem  Returns 0 when the package was made, 1 when the compiler is not installed
rem  and "all" was asked (skipped), 2 on a failure.
rem ---------------------------------------------------------------------------
:package
set "TOOL=%~1"
set "NAME=raven-%VERSION%-windows-x64-%TOOL%"
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
if not exist "%OUT%\bin\raven_license_check.exe" (
	echo dist.bat: bin\raven_license_check.exe is missing from the package.
	goto package_failed
)

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
set "BUILD=%ROOT%\build\dist-%TOOL%-%KIND%"
set "RELEASE=%ROOT%\build\dist-%TOOL%-%KIND%-release"
set "CONFIGURED=0"
echo.
echo -- %TOOL%, %KIND% library, license-free edition

call :configure "%BUILD%" %~2 LicenseFree "%OUT%" %~3 ON
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

echo.
echo -- %TOOL%, %KIND% library, Release edition (encrypted)
call :configure "%RELEASE%" %~2 Release "%RELEASE%\stage" %~3 OFF
if errorlevel 1 exit /b 1
cmake --build "%RELEASE%" --config Release --parallel
if errorlevel 1 exit /b 1
if exist "%RELEASE%\stage" rmdir /s /q "%RELEASE%\stage"
cmake --install "%RELEASE%" --config Release
if errorlevel 1 exit /b 1
set "LICENSE_TOOL=%RELEASE%\bin\Release\raven_license.exe"
if not exist "%LICENSE_TOOL%" set "LICENSE_TOOL=%RELEASE%\bin\raven_license.exe"
cmake "-DTOOL=%LICENSE_TOOL%" "-DKEYS=%KEYS%" "-DSTAGE=%RELEASE%\stage" "-DOUT=%OUT%" -P "%ROOT%\tools\license\encrypt_release.cmake"
if errorlevel 1 exit /b 1
exit /b 0

rem ---------------------------------------------------------------------------
rem  :configure <build dir> <shared ON|OFF> <edition> <install prefix> <library folder> <tests ON|OFF>
rem ---------------------------------------------------------------------------
:configure
if /I "%TOOL%"=="mingw" (
	cmake -S "%ROOT%" -B "%~1" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DRAVEN_BUILD_SHARED=%~2 -DRAVEN_EDITION=%~3 -DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TESTS=%~6 "-DCMAKE_INSTALL_PREFIX=%~4" -DCMAKE_INSTALL_LIBDIR=%~5 %EXTRA%
) else (
	cmake -S "%ROOT%" -B "%~1" -A x64 -DRAVEN_BUILD_SHARED=%~2 -DRAVEN_EDITION=%~3 -DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TESTS=%~6 "-DCMAKE_INSTALL_PREFIX=%~4" -DCMAKE_INSTALL_LIBDIR=%~5 %EXTRA%
)
exit /b %ERRORLEVEL%

:fail
if "%NO_PAUSE%"=="0" pause
endlocal
exit /b 1
