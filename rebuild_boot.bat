
@echo off
set UV=C:\Keil_v5\UV4\UV4.exe
set UV_PRO_PATH="%cd%\bootloader\mdk\project.uvprojx"
echo Init building ...
echo .>boot_build_log.txt
%UV% -j0 -c %UV_PRO_PATH%
%UV% -j0 -b %UV_PRO_PATH% -o %cd%\boot_build_log.txt
type boot_build_log.txt
echo Done.