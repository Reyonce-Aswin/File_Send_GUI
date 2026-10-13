# File Send (QT)

A simple cross-platform desktop app for sending files between computers on the same local network. Built with C++ and Qt.

## Features

- Automatic device discovery on the local network (UDP broadcast)
- Send files directly to another device over TCP, with no cloud or internet needed
- Manual IP entry for networks that block broadcast (e.g. phone hotspots)
- Progress bars for sending and receiving
- Configurable device name
- Received files never overwrite existing ones (`name (1).ext`)
- Only the file name is used from the sender, so paths can't escape the save folder

## Requirements

- Qt 6 (Qt 5.15+ should also work) with the **Widgets** and **Network** modules
- CMake 3.16+
- A C++17 compiler (MinGW, MSVC, GCC or Clang)

Developed and tested on Windows with Qt 6.11.1 (MinGW 64-bit) and Qt Creator.

## Build

### Qt Creator

1. **File → Open File or Project…** and select `CMakeLists.txt`
2. Pick a Qt kit and click **Configure Project**
3. Press **Ctrl+R** to build and run

### Command line

```
cmake -S . -B build
cmake --build build
```

## Usage

**Receiving**

1. Open the **Receive** tab and choose a save folder
2. Click **Start Receiving**
3. Note the IP address shown at the top of the tab

**Sending**

1. Open the **Send** tab and click **Refresh** to find devices
2. Select a device, or type its IP address and click **Add**
3. Choose a file and click **Send File**

The receiving device must be in receive mode to be discovered.

## Network details

| Purpose   | Protocol | Port |
|-----------|----------|------|
| Discovery | UDP      | 6000 |
| Transfer  | TCP      | 5000 |

Allow both through your firewall on private networks. Discovery relies on broadcast packets, which some networks (guest Wi-Fi, client isolation, some phone hotspots) block. In that case, use the manual IP option.

### Transfer protocol

All integers are big-endian:

```
uint32  file name length
bytes   file name (UTF-8, base name only)
uint64  file size
bytes   file data
```

## Deploying on Windows (no Qt installed)

1. Build in **Release** mode
2. Copy `filesend.exe` to a clean folder
3. From the Qt command prompt, run:
```
   windeployqt --release --no-translations filesend.exe
```
4. Zip the folder, or package it with an installer such as Inno Setup (`filesend.iss`)

## Troubleshooting

- **No devices found:** make sure the other device is on the Receive tab with *Start Receiving* pressed, both are on the same subnet, and the firewall allows UDP 6000 and TCP 5000.
- **Works with IP but not with discovery:** the network is blocking broadcast. Use the manual IP option.
- **Connection refused or timeout:** check the receiver's firewall and that both devices are on the same network.

## Project structure

| File | Purpose |
|------|---------|
| `main.cpp` | Application entry point |
| `mainwindow.*` | GUI (Send and Receive tabs, log) |
| `discovery.*` | UDP device discovery |
| `filesender.*` | TCP file sender |
| `filereceiver.*` | TCP file receiver |
| `common.h` | Shared constants and name normalization |

## License

Add your license here (for example MIT).
