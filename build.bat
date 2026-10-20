@echo off
rem ---------------------------------------------------------------------------
rem  Raven - build.bat
rem  Builds the library, the examples and the tests. The build tree is generated
rem  first (generate-cmake.bat) when it does not exist yet.
rem
rem    build.bat [configuration] [options]
rem
rem    configuration       Debug, Release (default), RelWithDebInfo or MinSizeRel
rem    --paid              the paid edition: no watermark (RAVEN_PAID=ON);
rem                        an existing build tree is switched to it
rem    --free              the free edition: every window shows a watermark
rem                        (the default for a new build tree)
rem    --build-dir DIR     build tree                      (default: build)
rem    --target NAME       build this target only          (raven, raven_editor, ...)
rem    --clean             rebuild everything
rem    --generate          generate the build tree again before building
rem    --test              run the tests after the build
rem    --run NAME          start an example after the build (hello, editor, glass, ...)
rem    --no-pause          do not wait for a key when the build fails
rem
rem  Examples:
rem    build.bat
rem    build.bat debug --test
rem    build.bat --paid
rem    build.bat --run glass
rem
rem  The programs are written to <build tree>\bin\<configuration>.
rem  To choose the generator or the options of the library, run
rem  generate-cmake.bat yourself first.
rem
rem  The configuration (Debug, Release...) and the edition (free, paid) are
rem  two different things.
rem ---------------------------------------------------------------------------
setlocal EnableExtensions

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "BUILD_DIR=%ROOT%\build"
set "CONFIG=Release"
set "PAID="
set "TARGET="
set "CLEAN=0"
set "GENERATE=0"
set "TEST=0"
set "RUN="
set "NO_PAUSE=0"
if defined RAVEN_NO_PAUSE set "NO_PAUSE=1"

:parse
if "%~1"=="" goto parsed
if /I "%~1"=="debug"          ( set "CONFIG=Debug" & shift & goto parse )
if /I "%~1"=="release"        ( set "CONFIG=Release" & shift & goto parse )
if /I "%~1"=="relwithdebinfo" ( set "CONFIG=RelWithDebInfo" & shift & goto parse )
if /I "%~1"=="minsizerel"     ( set "CONFIG=MinSizeRel" & shift & goto parse )
if /I "%~1"=="--paid"         ( set "PAID=ON" & shift & goto parse )
if /I "%~1"=="--free"         ( set "PAID=OFF" & shift & goto parse )
if /I "%~1"=="--build-dir"    ( set "BUILD_DIR=%~f2" & shift & shift & goto parse )
if /I "%~1"=="--target"       ( set "TARGET=%~2" & shift & shift & goto parse )
if /I "%~1"=="--run"          ( set "RUN=%~2" & shift & shift & goto parse )
if /I "%~1"=="--clean"        ( set "CLEAN=1" & shift & goto parse )
if /I "%~1"=="--generate"     ( set "GENERATE=1" & shift & goto parse )
if /I "%~1"=="--test"         ( set "TEST=1" & shift & goto parse )
if /I "%~1"=="--no-pause"     ( set "NO_PAUSE=1" & shift & goto parse )
if /I "%~1"=="--help"         goto usage
if /I "%~1"=="-h"             goto usage
echo [raven] unknown option: %~1
echo.
goto usage

:parsed
if "%BUILD_DIR:~-1%"=="\" set "BUILD_DIR=%BUILD_DIR:~0,-1%"

where cmake >nul 2>nul
if errorlevel 1 (
	echo [raven] cmake was not found in the PATH.
	echo         Install CMake 3.16 or newer from https://cmake.org/download, or start
	echo         this script from a "Developer Command Prompt for Visual Studio".
	goto failed
)

rem ---- build tree -----------------------------------------------------------
set "EDITION_ARGS="
if "%PAID%"=="ON" set "EDITION_ARGS=--paid"
if "%PAID%"=="OFF" set "EDITION_ARGS=--free"
if "%GENERATE%"=="1" goto generate
if exist "%BUILD_DIR%\CMakeCache.txt" goto generated
:generate
call "%ROOT%\generate-cmake.bat" --no-pause --build-dir "%BUILD_DIR%" %EDITION_ARGS%
if errorlevel 1 goto failed
:generated

rem  An existing tree keeps its edition unless --paid or --free asks for the other one.
if not defined PAID goto edition_done
findstr /B /I /C:"RAVEN_PAID:BOOL=%PAID%" "%BUILD_DIR%\CMakeCache.txt" >nul 2>nul
if not errorlevel 1 goto edition_done
echo [raven] switching the build tree to RAVEN_PAID=%PAID%
cmake -DRAVEN_PAID=%PAID% "%BUILD_DIR%"
if errorlevel 1 goto failed
:edition_done

rem  Visual Studio builds any configuration from one build tree. The generators
rem  with a single configuration (Ninja, NMake) choose it when the tree is
rem  generated: switch the tree when it holds another one.
findstr /B /C:"CMAKE_CONFIGURATION_TYPES:" "%BUILD_DIR%\CMakeCache.txt" >nul 2>nul
if not errorlevel 1 goto build
findstr /B /I /C:"CMAKE_BUILD_TYPE:STRING=%CONFIG%" "%BUILD_DIR%\CMakeCache.txt" >nul 2>nul
if not errorlevel 1 goto build
echo [raven] switching the build tree to %CONFIG%
cmake -DCMAKE_BUILD_TYPE=%CONFIG% "%BUILD_DIR%"
if errorlevel 1 goto failed

rem ---- build ----------------------------------------------------------------
:build
set "TARGET_ARGS="
if defined TARGET set "TARGET_ARGS=--target %TARGET%"
set "CLEAN_ARGS="
if "%CLEAN%"=="1" set "CLEAN_ARGS=--clean-first"

set "BUILT_EDITION=free"
findstr /B /I /C:"RAVEN_PAID:BOOL=ON" "%BUILD_DIR%\CMakeCache.txt" >nul 2>nul
if not errorlevel 1 set "BUILT_EDITION=paid"

echo [raven] building %CONFIG%, %BUILT_EDITION% edition
cmake --build "%BUILD_DIR%" --config %CONFIG% --parallel %TARGET_ARGS% %CLEAN_ARGS%
if errorlevel 1 goto failed

rem ---- tests ----------------------------------------------------------------
if not "%TEST%"=="1" goto tested
echo.
echo [raven] running the tests
pushd "%BUILD_DIR%"
ctest -C %CONFIG% --output-on-failure
set "TEST_RESULT=%ERRORLEVEL%"
popd
if not "%TEST_RESULT%"=="0" goto failed
:tested

echo.
echo [raven] build succeeded: %CONFIG%, %BUILT_EDITION% edition, programs in "%BUILD_DIR%\bin".
if "%BUILT_EDITION%"=="free" echo         Free edition: every window shows a watermark ^(--paid for the paid edition^).

rem ---- run ------------------------------------------------------------------
if not defined RUN exit /b 0
set "PROGRAM=%BUILD_DIR%\bin\%CONFIG%\raven_%RUN%.exe"
if not exist "%PROGRAM%" set "PROGRAM=%BUILD_DIR%\bin\raven_%RUN%.exe"
if not exist "%PROGRAM%" (
	echo [raven] there is no example named "%RUN%": raven_%RUN%.exe was not built.
	goto failed
)
echo [raven] starting raven_%RUN%
"%PROGRAM%"
exit /b %ERRORLEVEL%

:failed
echo.
echo [raven] build FAILED.
if "%NO_PAUSE%"=="0" pause
exit /b 1

:usage
echo Usage: build.bat [configuration] [options]
echo.
echo   configuration       Debug, Release, RelWithDebInfo or MinSizeRel; Release by default
echo   --paid              the paid edition: no watermark
echo   --free              the free edition: every window shows a watermark (default)
echo   --build-dir DIR     build tree, "build" by default
echo   --target NAME       build this target only: raven, raven_editor, ...
echo   --clean             rebuild everything
echo   --generate          generate the build tree again before building
echo   --test              run the tests after the build
echo   --run NAME          start an example after the build: hello, editor, glass, ...
echo   --no-pause          do not wait for a key when the build fails
exit /b 1
