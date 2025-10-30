---

# 🎵 Embedded Music Visualizer

## Overview

This project visualizes music in real-time using **FFmpeg**, **KissFFT**, and **LVGL**.
It is designed for both **embedded systems** and **Ubuntu PC**, allowing you to process audio data and visualize its spectrum dynamically.

All libraries are **built manually** for portability and compatibility — this project does **not** use `sudo apt-get install` for dependencies (except SDL2).

---

## 🧩 Dependencies

* **FFmpeg 4.4.4** – Audio decoding and input
* **KissFFT 131.1.0** – FFT processing for frequency visualization
* **LVGL 8.3.11** – GUI rendering and animation
* **lv_drivers (master)** – SDL driver for LVGL display

---

## ⚙️ Environment Setup

Before building any library, define your project path as an environment variable:

```bash
export PROJECT_DIR="<your-project-path>"
# Example:
# export PROJECT_DIR="/home/dell/EmbeddedMusicVisualizer"

# Verify
echo $PROJECT_DIR
```

---

## 🎬 Build FFmpeg

Build FFmpeg manually for full control (instead of system installation):

```bash
cd SharedLib/FFmpeg
# wget https://ffmpeg.org/releases/ffmpeg-4.4.4.tar.xz
tar -xf ffmpeg-4.4.4.tar.xz
cd ffmpeg-4.4.4

./configure --enable-shared --enable-static --disable-x86asm \
            --prefix=$PROJECT_DIR/SharedLib/FFmpeg/ffmpeg-build
make -j$(nproc)
make install
```

After build success, FFmpeg binaries and headers are located in:

```
SharedLib/FFmpeg/ffmpeg-build/
```

---

## 🎚️ KissFFT Library

KissFFT is already included in the source under:

```
MusicProcessor/kissfft/
```

You can also fetch a specific version if needed:

```bash
git clone https://github.com/mborgerding/kissfft.git
# Or download version 131.1.0 from GitHub releases
```

---

## 🖥️ LVGL and lv_drivers Setup

After cloning the project, initialize all submodules:

```bash
git submodule update --init --recursive
cd Graphic/lvgl
git checkout v8.3.11
cd ../lv_drivers
git checkout master
cd ../../
```

> ⚠️ Ensure that `lv_drivers` is compatible with LVGL 8.3.

If you encounter build errors such as missing LVGL types (`lv_indev_drv_t` or `lv_disp_drv_t`),
manually correct the include paths in the driver headers:

```cpp
// From:
#include "lvgl/lvgl.h"

// To:
#include "../../lvgl/lvgl.h"

// In files:
Graphic/lv_drivers/sdl/sdl.h  
Graphic/lv_drivers/sdl/sdl_common.h
```

> 💡 Normally this is not required if you use the provided CMakeLists.txt,
> but this ensures compatibility across different environments (e.g., Windows, Raspberry Pi).

---

## 🧱 Build the Project

If you don’t have SDL2 installed yet, run:

```bash
sudo apt-get install -y libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

Then build the project:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

---

## ▶️ Run the Application

After a successful build, run:

```bash
./musicvisualizer
```

If SDL or display issues occur, try:

```bash
sudo ./musicvisualizer
```

---

## 📁 Project Structure

```
EmbeddedMusicVisualizer/
│
├── Graphic/
│   ├── lvgl/           # LVGL library (submodule)
│   ├── lv_drivers/     # LVGL SDL driver (submodule)
│   ├── graphic.c
│   └── graphic.h
│
├── MusicProcessor/
│   ├── kissfft/        # FFT implementation
│   ├── musicprocessor.c
│   └── ring_buffer.c
│
├── SharedLib/
│   └── FFmpeg/         # FFmpeg build and source files
│
├── main.cpp
└── CMakeLists.txt
```

---

## 🧠 Common Issues

| Problem                       | Solution                                           |
| ----------------------------- | -------------------------------------------------- |
| `lv_indev_drv_t` unknown type | Ensure LVGL = v8.3.11 and lv_drivers = master      |
| `bool undeclared`             | Add `#include <stdbool.h>` if building manually    |
| FFmpeg configure fails        | Install `nasm` / `yasm`, or add `--disable-x86asm` |
| LVGL headers not found        | Make sure `Graphic/lvgl` is in include path        |
| Blank window when running     | Check SDL/OpenGL setup or try software rendering   |

---

## 👥 Contributors

| Name                | Role                                            |
| ------------------- | ----------------------------------------------- |
| **Trần Minh Hà**    | System design, FFmpeg integration, build system |
| **Trần Trang Linh** | GUI design, LVGL setup, visualization module    |

---

Copy this content into your project root as `README.md`, then run:

```bash
git add README.md
git commit -m "Add full README with setup and build instructions"
git push origin main
```

---

