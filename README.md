# stlink-tool

libusb tool for flashing chinese ST-Link dongles. Please note that similarly to ST's updater, the uploaded firmware won't replace the bootloader (meaning that you should be able to reflash the original afterwards using [ST's firmware update utility](http://www.st.com/en/development-tools/stsw-link007.html)).

```
Usage: ./stlink-tool [options] firmware.bin
Options:
	-p	Probe the ST-Link adapter
	-h	Show help
```

stlink-tool has been tested under Linux and macOS. With [sakana280's fork](https://github.com/sakana280/stlink-tool) you can use it under Windows.

## Compiling

Required dependencies :

 * C compiler (both clang and gcc seems to work great)
 * libusb1
 * git

```
git clone https://github.com/jeanthom/stlink-tool
cd stlink-tool
git submodule init
git submodule update
make
```

## [Writing firmwares for ST-Link dongles](docs/writing-firmware.md)

## Firmware upload protocol

ST's firmware upload protocol is USB's DFU protocol with some twists. Every DFU command is issued with the 0xF3 prefix, and the command set does not exactly match USB's.

Some documentation :
 * http://www.st.com/content/ccc/resource/technical/document/application_note/6a/17/92/02/58/98/45/0c/CD00264379.pdf/files/CD00264379.pdf/jcr:content/translations/en.CD00264379.pdf
 * http://www.usb.org/developers/docs/devclass_docs/DFU_1.1.pdf
