@echo off
setlocal
pushd "%~dp0"
wsl -e bash -l -c "make -C build" || exit /b 1
copy /y build\vc4d.library ..\TestHD\Libs\Warp3D.library
call ..\hdtest.cmd
popd
