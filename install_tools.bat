@echo off
REM FreeRTOS PC Scheduler - Gerekli Araclari Yukle
REM Bu script winget kullanarak CMake ve MinGW yukler

echo ============================================
echo FreeRTOS PC Scheduler - Kurulum Araci
echo ============================================
echo.

REM Yonetici haklari kontrolu
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [UYARI] Bu script yonetici haklari gerektirebilir.
    echo Sag tiklayip "Yonetici olarak calistir" secin.
    echo.
)

echo Mevcut araclari kontrol ediyorum...
echo.

REM CMake kontrolu
where cmake >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] CMake zaten yuklu.
    cmake --version
) else (
    echo [YUKLENIYOR] CMake...
    winget install Kitware.CMake --accept-package-agreements --accept-source-agreements
)

echo.

REM GCC/MinGW kontrolu
where gcc >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] GCC zaten yuklu.
    gcc --version
) else (
    echo [YUKLENIYOR] MinGW-w64 (GCC derleyicisi)...
    winget install MSYS2.MSYS2 --accept-package-agreements --accept-source-agreements
    echo.
    echo [BILGI] MSYS2 yuklendi. Simdi GCC'yi yuklemek icin:
    echo   1. MSYS2 MINGW64 terminalini acin
    echo   2. Calistirin: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make
    echo   3. PATH'e ekleyin: C:\msys64\mingw64\bin
)

echo.

REM Git kontrolu
where git >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] Git zaten yuklu.
) else (
    echo [YUKLENIYOR] Git...
    winget install Git.Git --accept-package-agreements --accept-source-agreements
)

echo.
echo ============================================
echo Kurulum tamamlandi!
echo ============================================
echo.
echo Simdi terminali kapatip yeniden acin,
echo sonra build.bat dosyasini calistirin.
echo.
pause
