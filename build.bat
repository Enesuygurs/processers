@echo off
REM FreeRTOS PC Scheduler - Windows Derleme Script'i

echo ============================================
echo FreeRTOS PC Scheduler - Derleme Araci
echo ============================================
echo.

REM PATH'e MinGW ve CMake ekle
set PATH=C:\msys64\mingw64\bin;C:\Program Files\CMake\bin;%PATH%

REM GCC kontrolu
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [HATA] GCC bulunamadi!
    echo.
    echo Lutfen install_tools.bat dosyasini calistirin.
    pause
    exit /b 1
)

REM CMake kontrolu  
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [HATA] CMake bulunamadi!
    echo.
    echo Lutfen install_tools.bat dosyasini calistirin.
    pause
    exit /b 1
)

REM Build klasoru olustur
if not exist build mkdir build
cd build

REM CMake yapilandirmasi
echo [1/3] CMake yapilandiriliyor...
cmake .. -G "MinGW Makefiles"
if %ERRORLEVEL% NEQ 0 (
    echo [HATA] CMake yapilandirmasi basarisiz!
    cd ..
    pause
    exit /b 1
)

REM Derleme
echo [2/3] Proje derleniyor...
mingw32-make

if %ERRORLEVEL% NEQ 0 (
    echo [HATA] Derleme basarisiz!
    cd ..
    pause
    exit /b 1
)

echo.
echo [3/3] Derleme tamamlandi!
echo.
echo Calistirmak icin:
echo   run.bat
echo veya
echo   build\freertos_sim.exe giris.txt
echo.

cd ..
pause
