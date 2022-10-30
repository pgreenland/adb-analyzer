# Apple Desktop Bus (ADB) Analyzer

This analyzer decodes Apple Desktop Bus traffic.

It may be connected directly to the ADB pin of any Apple Desktop Bus network.

It will decode the command and data transactions on the bus.

Attention / sync or start bits will be marked with a green blob.
Stop bits will be marked with a red square.
Service requests during stop bits will be marked with an upward arrow.

An example capture may be generated to demo the behaviors described above.

## Getting Started

### MacOS

Dependencies:
- XCode with command line tools
- CMake 3.13+

Installing command line tools after XCode is installed:
```
xcode-select --install
```

Then open XCode, open Preferences from the main menu, go to locations, and select the only option under 'Command line tools'.

Installing CMake on MacOS:

1. Download the binary distribution for MacOS, `cmake-*-Darwin-x86_64.dmg`
2. Install the usual way by dragging into applications.
3. Open a terminal and run the following:
```
/Applications/CMake.app/Contents/bin/cmake-gui --install
```
*Note: Errors may occur if older versions of CMake are installed.*

Building the analyzer:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### Ubuntu 16.04

Dependencies:
- CMake 3.13+
- gcc 4.8+

Misc dependencies:

```
sudo apt-get install build-essential
```

Building the analyzer:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### Windows

Dependencies:
- Visual Studio 2015 Update 3
- CMake 3.13+

**Visual Studio 2015**

*Note - newer versions of Visual Studio should be fine.*

Setup options:
- Programming Languages > Visual C++ > select all sub-components.

Note - if CMake has any problems with the MSVC compiler, it's likely a component is missing.

**CMake**

Download and install the latest CMake release here.
https://cmake.org/download/

Building the analyzer:
```
mkdir build
cd build -A x64
cmake ..
```

Then, open the newly created solution file located here: `build\adb_analyzer.sln`


## Output Frame Format

### Frame Type: `"data"`

| Property | Type | Description |
| :--- | :--- | :--- |
| `addr` | int | Address of device issued by host |
| `cmd` | string | Command code issued by host |
| `reg` | int | Register index issues by host |
| `data` | bytes | Data transferred to/from device register depending on command |
| `svrreq` | bool | Service request placed in either command or data stop bit |

This is the decoded ADB command and data frames.
