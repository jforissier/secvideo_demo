This project is a proof-of-concept for secure video playback on
ARM/Trustzone hardware running Linux and OP-TEE.
A linux application relies on OP-TEE and a Trusted Application to
decrypt and display an image in a secure way.

The demo runs on a software model: ARM's FVP_Base_AEMv8A-AEMv8A.

1. TL;DR

Install prerequisites:
```sh
$ sudo apt-get install uuid-dev gcc-arm-linux-gnueabihf

# On x86_64 systems only
$ sudo apt-get install libc6:i386 libstdc++6:i386 libz1:i386
```

Clone, build and run the demo:
```sh
$ git clone https://github.com/jforissier/secvid_proto.git
$ cd secvid_proto
$ git submodule update --init
$ make
$ ./run.sh
```

2. More information

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
- The OP-TEE linux driver is loaded (modprobe optee)
- The normal world TEE daemon is started (tee-supplicant&)
- The secvid-demo application is started. It uses the TEE Client library
  (libteec.so) to open a session with a trusted application.
  * The TEE Client calls the OP-TEE driver
  * OP-TEE Core instantiates the required trusted application
    (ffa39702-9ce0-47e0-a1cb-4048cfdb847d.ta): the binary is loaded by
    tee-supplicant and transfered to secure world through the OP-TEE driver.
- The TA decrypts the image data into secure memory, using the TEE Crypto
  API.
- The TA decodes/copies the image into the framebuffer for display.

The project has the following directories:

  - arm-trusted-firmware
  - edk2
  - linux
  - optee_os
  - optee_client
  - optee_linuxdriver
  - ...

