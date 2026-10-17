# ECmacro

A compact USB macro pad using electrostatic capacitive switches (Topre/NIZ style).  
| Model | Key Count | Typical Use |
|-------|-----------|-------------|
| **ECmacro 2V** | 2 keys | Volume / Mute |

---

## 🖼️ Gallery

![Image View](./images/ECmacro-2V.png)

---

## 📦 Features

- Electrostatic capacitive switches (Topre/NIZ style)
- USB HID device with CH552 microcontroller
- Functions:
  - Volume Up (Key 1)
  - Volume Down (Key 2)
  - Mute (Simultaneous press)
- Open-source hardware and firmware
- USB VID/PID from [pid.codes](https://pid.codes/1209/EC02/): `1209:EC02`

---

## 🖥️ Supported OS

- Windows 10/11
- macOS (10.14+)
- Linux (kernel with HID support)

---

## 🔍 Hardware Resources

- [KiCad project](./hardware/2V/)

---

## 🧩 Firmware

- [Firmware](./firmware/ch552/)
  - Built with [SDCC](https://sdcc.sourceforge.net/) and GNU make.  
    See the firmware directory for building, flashing and the pin assignment.

---

## 📄 License

This project uses dual licenses:

- **Firmware and source code**: [MIT License](./LICENSES/MIT.txt)
- **Schematics, PCB layout, and documents**: [CC BY 4.0](./LICENSES/CC-BY-4.0.txt)

© 2025–2026 QuadState

---

## 🆔 USB PID Registration

This project has a **USB Product ID (PID)** from [pid.codes](https://pid.codes/):

- **Vendor ID**: `0x1209` (pid.codes community VID)
- **Product ID**: `0xEC02`
- **PID.codes listing**: [1209/EC02](https://pid.codes/1209/EC02/)

---

## 💬 Contact & Feedback

Please use [GitHub Issues](https://github.com/QuadState/ECmacro/issues) for questions, bugs, or feedback.  
We welcome contributions, suggestions, and improvements!

---
