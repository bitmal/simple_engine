@echo off

rem @DESCRIPTION
rem Name: Simple Engine
rem Creation Date: 2019
rem Dev Name: Bitmal
rem AppName: simple_engine
rem Language(s): C99
rem Interface(s): SDL2
rem Renderer: OpenGL

rem @INPUTS
set arch=x64
set config=debug
set interfaces=SDL2
set renderer=OpenGL
set editor=nvim

rem @GLOBAL
rem TODO: make some of these variables as inputs into the script
set name_title=Simple Engine
set name_app=simple_engine
set app_lib=opengl32.lib SDL2main.lib SDL2.lib glew32.lib
set lib_inc_dir=D:\Libraries\include
set app_src=src/engine/main.c

rem @MAIN

call :U_PRINT "SYS" "Launching %name_app% - Console"
goto :APP_BEGIN

rem @BEGIN
:APP_BEGIN
	call :U_PRINT "SYS" "Beginning %name_app% process"

	if "%1"=="" (
		call :APP_RUN
	) else (
	if "%1"=="build" (
		call :APP_BUILD
	) else (
	if "%1"=="build_tools" (
		call :APP_BUILD_TOOLS
	) else (
	if "%1"=="run" (
		call :APP_RUN
	) else (
	if "%1"=="editor" (
		call :APP_EDITOR
	) else (
	if "%1"=="debug" (
		call :APP_RUN_DEBUG
	))))))
goto :APP_END

rem @RUN
:APP_RUN
	call :U_PRINT "SYS" "Running %name_app% app ..."
	
	rem TODO: any pre-run steps
	call :U_PRINT "SYS" "Pre-Run Steps ..."

	rem TODO: any run steps
	call C:\Users\subst\source\repos\%name_app%\build\%arch%\%config%\bin\%name_app%.exe
	
	rem TODO: any post-run steps
exit /B 0

rem @RUN_DEBUG
:APP_RUN_DEBUG
	call :U_PRINT "SYS" "Running %name_app% app ..."
	
	rem TODO: any pre-run steps
	call :U_PRINT "SYS" "Pre-Run Steps ..."
	call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

	rem TODO: any run steps
	call devenv /runexit /nosplash "C:\Users\subst\source\repos\%name_app%\build\%arch%\%config%\bin\%name_app%.exe"
	
	rem TODO: any post-run steps
exit /B 0

rem @BUILD_TOOLS
:APP_BUILD_TOOLS
	call :U_PRINT "SYS" "Building Tool(s) ..."
	pushd "C:/Users/subst/source/repos/"%name_app%/src/tools

	for /D %%G in ("./") do (
		pushd C:/Users/subst/source/repos/%name_app%/src/tools/%%G
		call ./app.cmd build
		popd

		copy /Y /B "C:/Users/subst/source/repos/%name_app%/build/tools/%arch%/%config%/bin/%name_app%.exe" /B "C:/Users/subst/source/repos/%name_app%/tools"
	)
	
	call :U_PRINT "SYS" "Done Building Tool(s) ..."
exit /B 0

rem @BUILD
:APP_BUILD
	call :U_PRINT "SYS" "Building App ..."
	pushd "C:/Users/subst/source/repos/"%name_app%
	rem TODO: any pre-logging
	
	if exist build (
		rmdir /S /Q build 
	)
	
	mkdir build\%arch%\%config%\lib
	mkdir build\%arch%\%config%\bin
	
	rem TODO: any pre-build steps
	call :U_PRINT "SYS" "Pre-Build Steps ..."
	call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
	
	rem TODO: any build steps
	call :U_PRINT "SYS" "Build Steps ..."
	rem TODO: (prebuild logging)
	cl /Od /EHsc /Zi /Febuild/%arch%/%config%/bin/%name_app% /Isrc/engine /TC %app_src% /link /SUBSYSTEM:WINDOWS %app_lib%
	
	rem TODO: any post-build steps
	call :U_PRINT "SYS" "Post-Build Steps ..."
exit /B 0

rem @EDITOR
:APP_EDITOR
	call :U_PRINT "SYS" "OPENING EDITOR "%editor%
	call nvim -u ./.vimrc
exit /B 0
rem

rem @END
:APP_END
	rem TODO: any post-logging
	
	popd
goto :eof

rem @UTILITY
:U_PRINT
	echo "[U_MESSAGE | %~1 | ("%date% %time%")]:"
	echo %~2
exit /B 0
