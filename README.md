# FreeRTOS PC Scheduler Simülasyonu

## 4 Seviyeli Öncelikli Görev Sıralayıcısı (Scheduler)

Bu proje, FreeRTOS'un görev sıralayıcısının PC üzerinde POSIX (Linux) veya Windows ortamında nasıl çalıştığını simüle eder.

## 📋 Proje Özellikleri

- **4 Seviyeli Öncelik Sistemi:**
  - Seviye 0: Gerçek Zamanlı (Real-Time) - FCFS algoritması
  - Seviye 1: Yüksek Öncelikli Kullanıcı Görevi
  - Seviye 2: Orta Öncelikli Kullanıcı Görevi
  - Seviye 3: Düşük Öncelikli Kullanıcı Görevi

- **Multi-Level Feedback Queue (MLFQ):** Kullanıcı görevleri için geri beslemeli kuyruk
- **FCFS:** Gerçek zamanlı görevler için First-Come-First-Served algoritması
- **Time Quantum:** 1 saniye zaman dilimi
- **Renkli Çıktı:** Her görev için benzersiz renk şeması

## 📁 Dosya Yapısı

```
FreeRTOS_PC_Scheduler/
├── FreeRTOS/
│   ├── include/           # FreeRTOS header dosyaları
│   ├── source/            # FreeRTOS kernel kaynak kodları
│   └── portable/
│       ├── MSVC-MingW/    # Windows portu
│       ├── ThirdParty/GCC/Posix/  # Linux portu
│       └── MemMang/       # Bellek yönetimi (heap_4.c)
├── src/
│   ├── main_freertos.c    # FreeRTOS kullanan ana program
│   ├── FreeRTOSConfig.h   # FreeRTOS yapılandırması
│   ├── main.c             # Alternatif (pthread tabanlı)
│   ├── scheduler.c        # Scheduler implementasyonu
│   ├── scheduler.h        # Header dosyası
│   └── tasks.c            # Görev fonksiyonları
├── CMakeLists.txt         # CMake build dosyası
├── Makefile               # Linux için Makefile
├── giris.txt              # Görev listesi giriş dosyası
└── README.md              # Bu dosya
```

## 🛠️ Derleme

### Windows (Visual Studio ile)

```powershell
# Build klasörü oluştur
mkdir build
cd build

# CMake ile yapılandır
cmake .. -G "Visual Studio 17 2022"

# Derle
cmake --build . --config Release

# Çalıştır
.\Release\freertos_sim.exe ..\giris.txt
```

### Windows (MinGW ile)

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
./freertos_sim.exe ../giris.txt
```

### Linux / WSL

```bash
mkdir build && cd build
cmake ..
make
./freertos_sim ../giris.txt
```

### Makefile ile (Linux)

```bash
make clean
make
./freertos_sim giris.txt
```

## 📝 Giriş Dosyası Formatı

`giris.txt` dosyası virgülle ayrılmış değerler içerir:

```
<varış_zamanı>, <öncelik>, <görev_süresi>
```

Örnek:
```
0, 1, 2
1, 0, 1
1, 3, 2
```

- **Öncelik 0:** Gerçek Zamanlı (RT) - Kesintisiz çalışır
- **Öncelik 1-3:** Kullanıcı görevleri - MLFQ ile yönetilir

## 🎯 Algoritma Açıklaması

### Gerçek Zamanlı Görevler (Öncelik 0)
- FCFS (First-Come-First-Served) algoritması
- Kesintisiz çalışır, tamamlanana kadar diğer görevler bekler

### Kullanıcı Görevleri (Öncelik 1-3)
- Multi-Level Feedback Queue (MLFQ)
- Time quantum: 1 saniye
- Quantum bittiğinde öncelik düşürülür
- En düşük seviyede Round-Robin

### Zaman Aşımı
- Maksimum görev süresi: 20 saniye
- Süre aşıldığında görev otomatik sonlandırılır

## 📊 Çıktı Formatı

```
0.0000 sn proses basladi     (id:0001 oncelik:1 kalan sure:2 sn)
1.0000 sn proses yurutuluyor (id:0001 oncelik:1 kalan sure:1 sn)
2.0000 sn proses sonlandi    (id:0001 oncelik:1 kalan sure:0 sn)
```

## 👥 Yazar

İşletim Sistemleri Dersi Projesi - 2025

## 📄 Lisans

Bu proje eğitim amaçlıdır. FreeRTOS MIT lisansı altındadır.
