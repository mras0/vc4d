@echo off
setlocal
pushd "%~dp0"
wsl -e bash -l -c "make -C build" || exit /b 1
copy /y build\vc4d.library Warp3D.library
call \tools\send.cmd Warp3D.library
rem c:\tools\teraterm\ttermpro.exe /C=4 /BAUD=115200
c:\tools\teraterm\ttermpro.exe /C=4 /BAUD=345600

rem copy /y build\vc4d.library ..\TestHD\Libs\Warp3D.library
rem call ..\hdtest.cmd

popd
