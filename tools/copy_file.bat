@echo off

if "%1" == "boot" (
copy "..\..\build\bootloader\List\bootloader.map" "..\..\build\bootloader.map"
copy "..\..\build\bootloader\Obj\bootloader.axf" "..\..\build\bootloader.axf"
copy "..\..\build\bootloader\Obj\bootloader.bin" "..\..\build\bootloader.bin"
copy "..\..\build\bootloader\Obj\bootloader.hex" "..\..\build\bootloader.hex"
) else (
copy ".\build\keil\List\rt-thread.map" ".\build\app.map"
copy ".\build\keil\Obj\rt-thread.axf" ".\build\app.axf"
copy ".\build\keil\Obj\rt-thread.bin" ".\build\app.bin"
copy ".\build\keil\Obj\rt-thread.hex" ".\build\app.hex"
)

