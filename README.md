THIS CODE IS OBSOLETE

This project is a proof-of-concept for secure video playback on
ARM Trustzone hardware running Linux and OP-TEE. A linux application relies
on OP-TEE and a Trusted Application to decrypt and display an image in a
secure way.

Initially, the Arm *Base FVP* software model was used. This branch is an
attempt at running the code with the free Arm-v8A emulator tool called
*Foundation Platform*, available from
https://developer.arm.com/products/system-design/fixed-virtual-platforms.
Unfortunately the boot hangs shortly after entering TF-A BL31. Anyway I am
not even sure the model emulates a display and a TZASC...

I'm keeping the instructions below in case someone would want to experiment
further.

## TL;DR

1. Install the required compilers, tools and libraries (I am using Ubuntu 14.04
x86_64 as my development system):
```sh
$ sudo apt-get install uuid-dev imagemagick
# On x86_64 systems only #
$ sudo apt-get install libc6:i386 libstdc++6:i386 libz1:i386
```

2. Clone and build the demo:
```sh
$ git clone https://github.com/jforissier/secvid_demo.git
$ cd secvid_demo
$ git submodule update --init
$ make
```

3. Download the *Arm-v8A Foundation Platform* software from
https://developer.arm.com/products/system-design/fixed-virtual-platforms and
make sure that `Foundation_Platform` is in your `$PATH`, for instance:
```sh
export PATH=~/Foundation_Platformpkg/models/Linux64_GCC-4.9/:$PATH
```

4. Run the demo:
```sh
$ ./run/run.sh
# FIXME: the boot hangs at BL31 initialization
```
In the FVP terminal:
```sh
modprobe secfb
modprobe optee_armtz
sleep 1
tee-supplicant&
secvideo_demo -h
# -ns: do not make output buffer secure, -r: read data from NS world
# This succeeds: decrypted data can be read from non-secure output buffer
# (0xff's are read which is the white background)
secvideo_demo -ns linaro-logo-web.rgba.aes -r
# Clear screen
secvideo_demo -c
# This fails: image is displayed but NS world can't read from secure
# output buffer (only 0x00's are read)
secvideo_demo linaro-logo-web.rgba.aes -r
# NOTE: BUG: once output buffer is secured it cannot be made non-secure
# unless the FVP is rebooted
```

## More information

The demo performs the following tasks:

- The system is booted.
  * The ARM model loads the 1st level Boot Loader (BL1), which is the ARM
    Trusted Firmware BL1 binary, as well as the Firmware Image Package
    (FIP) composed of: ARM TF BL2, ARM TF BL3.1 (secure monitor), BL3.2
    (OP-TEE), BL3.3 (UEFI - EDK2).
  * Control is transferred to BL1 which will start system bring-up: BL2,
    secure monitor initialization, OP-TEE initialization, then UEFI loads
    and boots the linux kernel.
  * OP-TEE initialization involves the following steps:
    - Reserve memory for the frame buffer and any other buffer required
      for decryption/decoding and configure the TZC400 address space
      controller to restrict access to the buffers to secure world and the
      LCD controller only
    - Initializes the display controller (width, height, frame buffer
      address, image format...)
  * The kernel mounts the root filesystem and drops to a shell on serial
    port #0.
  * The OP-TEE linux driver is loaded (modprobe optee_armtz).
  * The dummy "secure framebuffer" is loaded (modprobe secfb). It implements
    an ioctl() that returns the buffer used by the secure OS as a framebuffer
    (this is the buffer used when configuring the LCD display controller).
- The normal world TEE daemon is started (tee-supplicant&)
- The secvido_demo application is started. It uses the TEE Client library
  (libteec.so) to open a session with a trusted application.
  * The TEE Client calls the OP-TEE driver
  * OP-TEE Core instantiates the required trusted application
    (ffa39702-9ce0-47e0-a1cb-4048cfdb847d.ta): the binary is loaded by
    tee-supplicant and transfered to secure world through the OP-TEE driver.
- The TA decrypts the image data into secure memory, using the TEE Crypto
  API.
- The TA decodes/copies the image into the framebuffer for display.

The project has the following directories:

  - app
    - app/host: Main Linux application, `secvideo_demo`
    - app/ta: Trusted side of the application
  - arm-trusted-firmware
  - downloads: Temporary files downloaded when project is built for the first
  time (BusyBox sources, compilers)
  - edk2: UEFI bootloader
  - gen_rootfs: Utilities to build a minimal root filesystem for Linux, based on BusyBox
  - linux: The Linux kernel, including the OP-TEE driver
  - optee_client: OP-TEE client library
  - optee_os: OP-TEE
  - run: Contains a script to launch the simulation
  - toolchains: GCC cross-compilers for AArch64

