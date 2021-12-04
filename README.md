# Dump CAN Frames to CSV (Linux Only)

This program helps you to record CAN Bus frames to CSV file with your linux machine.

## How to use

```
Usage:
    ./can-record <interface> <seconds> <path-to-output-file>

Example:
    ./can-record can0 30 /mnt/sdcard/recorded.csv
```

## Build for current machine

```bash
make
```

## Build for other CPU architecture

```bash
CC=/path/to/toolchain/compiler make
```

Change `/path/to/toolchain/compiler` to your compiler path from toolchain as example [Linaro](https://www.linaro.org/downloads/) or [aarch64-linux-gnu-gcc](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads).