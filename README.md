# 🎵 Embedded Music Visualizer

A real-time **audio spectrum visualizer** designed for **embedded Linux systems** and **Ubuntu PC**.
The project uses **FFmpeg** for audio decoding, **KissFFT** for frequency analysis, **LVGL** for GUI rendering, and an optional **LED Matrix module** for hardware-based visualization.

All major dependencies are **built manually** to ensure **portability**, **cross-compilation support**, and **embedded compatibility**.

---

## 🚀 Features

* 🎧 Real-time audio decoding using **FFmpeg**
* 📊 Frequency spectrum analysis using **KissFFT**
* 🖥️ Lightweight GUI with **LVGL (SDL backend)**
* 💡 **LED Matrix visualization module** (hardware-oriented output)
* 🍓 Cross-compiled and runnable on **Raspberry Pi 4 (aarch64)**
* 🧪 **SystemC simulation** for architecture and dataflow validation
* 🔧 Minimal system dependencies (SDL2 only)

---

## 🧩 Dependencies

| Library    | Version | Purpose                       |
| ---------- | ------- | ----------------------------- |
| FFmpeg     | 4.4.4   | Audio decoding & input        |
| KissFFT    | 131.1.0 | FFT processing                |
| LVGL       | 8.3.11  | GUI framework                 |
| lv_drivers | master  | SDL backend for LVGL          |
| SDL2       | system  | Windowing & input (PC / Pi)   |
| SystemC    | bundled | Architecture-level simulation |

---

## ⚙️ Environment Setup (Host)

Set the project root directory:

```bash
export PROJECT_DIR=/home/dell/EmbeddedMusicVisualizer
echo $PROJECT_DIR
```

---

## 🎬 Build FFmpeg (Host – Native)

```bash
cd SharedLib/FFmpeg
tar -xf ffmpeg-4.4.4.tar.xz
cd ffmpeg-4.4.4

./configure \
  --enable-shared \
  --enable-static \
  --disable-x86asm \
  --prefix=$PROJECT_DIR/SharedLib/FFmpeg/ffmpeg-build

make -j$(nproc)
make install
```

Output directory:

```
SharedLib/FFmpeg/ffmpeg-build/
```

---

## 🎚️ KissFFT

Already included in the project:

```
MusicProcessor/kissfft/
```

---

## 🖥️ LVGL & lv_drivers Setup

Initialize submodules:

```bash
git submodule update --init --recursive

cd Graphic/lvgl
git checkout v8.3.11

cd ../lv_drivers
git checkout master

cd ../../
```

### Header Include Fix (Only If Needed)

If you encounter errors such as:

```
lv_indev_drv_t unknown type
```

Edit the include path:

```cpp
// From:
#include "lvgl/lvgl.h"

// To:
#include "../../lvgl/lvgl.h"
```

Affected files:

```
Graphic/lv_drivers/sdl/sdl.h
Graphic/lv_drivers/sdl/sdl_common.h
```

---

## 🧱 Build & Run on Ubuntu Host

### Install SDL2 (Host)

```bash
sudo apt-get install -y \
  libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

### Build Project

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### Run

```bash
./musicvisualizer
```

---

## 🍓 Cross Compile & Deploy for Raspberry Pi 4 (aarch64)

> Target OS: **Raspberry Pi OS 64-bit**
> Flow: **HOST build → package → scp → PI run**

---

### 0) Host Environment Variables

```bash
export PROJECT_DIR=/home/dell/EmbeddedMusicVisualizer
export PI_IP=172.20.10.2
```

---

### 1) Raspberry Pi: Install Runtime Dependencies (Once)

```bash
ssh pi@$PI_IP
sudo apt-get update
sudo apt-get install -y \
  libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev \
  alsa-base alsa-utils \
  patchelf
exit
```

---

### 2) Host: Sync Sysroot from Pi

```bash
cd "$PROJECT_DIR/SharedLib_Pi/pi-sysroot"
rsync -avz pi@$PI_IP:/lib .
rsync -avz pi@$PI_IP:/usr/include usr
rsync -avz pi@$PI_IP:/usr/lib usr
```

If multiarch directories are missing:

```bash
rsync -avz pi@$PI_IP:/usr/lib/aarch64-linux-gnu usr/lib/
rsync -avz pi@$PI_IP:/lib/aarch64-linux-gnu lib/
```

---

### 3) Host: Download aarch64 Toolchain

```bash
cd "$PROJECT_DIR/SharedLib_Pi/CrossCompiler"

wget https://developer.arm.com/-/media/Files/downloads/gnu/11.2-2022.02/binrel/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu.tar.xz
tar -xf gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu.tar.xz
```

---

### 4) Host: Cross-build FFmpeg for Raspberry Pi

```bash
cd "$PROJECT_DIR/SharedLib_Pi/FFmpeg/ffmpeg-4.4.4"

./configure \
  --enable-cross-compile \
  --cross-prefix=$PROJECT_DIR/SharedLib_Pi/CrossCompiler/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu- \
  --arch=aarch64 \
  --target-os=linux \
  --sysroot=$PROJECT_DIR/SharedLib_Pi/pi-sysroot \
  --extra-cflags="-I$PROJECT_DIR/SharedLib_Pi/pi-sysroot/usr/include -I$PROJECT_DIR/SharedLib_Pi/pi-sysroot/usr/include/aarch64-linux-gnu" \
  --extra-ldflags="-L$PROJECT_DIR/SharedLib_Pi/pi-sysroot/usr/lib/aarch64-linux-gnu -L$PROJECT_DIR/SharedLib_Pi/pi-sysroot/lib/aarch64-linux-gnu -Wl,--rpath-link=$PROJECT_DIR/SharedLib_Pi/pi-sysroot/lib/aarch64-linux-gnu -Wl,--rpath-link=$PROJECT_DIR/SharedLib_Pi/pi-sysroot/usr/lib/aarch64-linux-gnu" \
  --enable-alsa \
  --enable-static \
  --enable-shared \
  --prefix=$PROJECT_DIR/SharedLib_Pi/FFmpeg/ffmpeg-build

make -j$(nproc)
make install
```

---

### 5) Host: Build Project, Package & Deploy

```bash
cd "$PROJECT_DIR"

rm -rf build deploy
cmake -S . -B build
cmake --build build -j$(nproc)

mkdir -p deploy/lib
cp build/musicvisualizer deploy/
cp -a SharedLib_Pi/FFmpeg/ffmpeg-build/lib/*.so* deploy/lib/

scp -r deploy/ pi@$PI_IP:~/musicvisualizer/
```

---

### 6) Raspberry Pi: Fix Interpreter & Run

```bash
ssh pi@$PI_IP
cd ~/musicvisualizer/deploy

export LD_LIBRARY_PATH="$PWD/lib:$LD_LIBRARY_PATH"
export DISPLAY=:0

# Fix ELF interpreter to avoid "No such file or directory"
patchelf --set-interpreter /lib/ld-linux-aarch64.so.1 ./musicvisualizer

./musicvisualizer
```
---

## 🧪 SystemC Simulation

Used to simulate the **architecture and dataflow** of the music visualizer at a higher abstraction level.

### Build SystemC Simulation

```bash
cd /home/dell/EmbeddedMusicVisualizer

rm -rf build-sc
cmake -S systemc_sim -B build-sc
cmake --build build-sc -j$(nproc)
```

### Run Simulation (Generate VCD)

```bash
./build-sc/musicviz_sc_sim
```

### View Waveform

```bash
gtkwave musicviz_arch.vcd
```

---

## 📁 Project Structure

```
EmbeddedMusicVisualizer/
├── Graphic/
│   ├── lvgl/
│   ├── lv_drivers/
│   ├── graphic.c
│   └── graphic.h
│
├── MusicProcessor/
│   ├── kissfft/
│   ├── musicprocessor.c
│   └── ring_buffer.c
│
├── LedMatrix/
│   ├── led.c
│   └── led.h
│
├── SharedLib/
│   └── FFmpeg/
│
├── SharedLib_Pi/
│   ├── CrossCompiler/
│   ├── FFmpeg/
│   └── pi-sysroot/
│
├── systemc_sim/
├── main.cpp
└── CMakeLists.txt
```

---

## 👥 Contributors

| Name                | Contribution                                         |
| ------------------- | ---------------------------------------------------- |
| **Trần Minh Hà**    | System design, FFmpeg integration, cross-compilation |
| **Trần Trang Linh** | GUI design, LVGL integration, visualization          |




