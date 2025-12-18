@echo off
REM FreeRTOS PC Scheduler - Calistirma Script'i

REM PATH'e MinGW ve CMake ekle
set PATH=C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;%PATH%

REM Build klasorune git
cd /d "%~dp0build"

REM Calistir
if exist freertos_sim.exe (
    freertos_sim.exe ..\giris.txt
) else (
    echo [HATA] freertos_sim.exe bulunamadi!
    echo Once build.bat ile derleyin.
    pause
)
