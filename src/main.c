/*
  Copyright (c) 2018 Jean THOMAS.
  
  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software
  is furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
  OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libusb.h>

#include "stlink.h"

#define STLINK_VID 0x0483
#define STLINK_PID 0x3748
#define STLINK_PIDV21 0x374b
#define STLINK_PIDV21_MSD 0x3752
#define STLINK_PIDV3      0x374f
#define STLINK_PIDV3_BL   0x374d

#define OPENMOKO_VID 0x1d50
#define BMP_APPL_PID 0x6018
#define BMP_DFU_IF   4

void print_help(char *argv[]) {
  printf("Usage: %s [options] [firmware.bin]\n", argv[0]);
  printf("Options:\n");
  printf("\t-p\tProbe the ST-Link adapter\n");
  printf("\t-h\tShow help\n\n");
  printf("\tApplication is started when called without argument or after firmware load\n\n");
}

#include <string.h>
int main(int argc, char *argv[]) {
  struct STLinkInfo info;
  int res = EXIT_FAILURE, i, opt, probe = 0;

  while ((opt = getopt(argc, argv, "hp")) != -1) {
    switch (opt) {
    case 'p': /* Probe mode */
      probe = 1;
      break;
    case 'h': /* Help */
      print_help(argv);
      return EXIT_SUCCESS;
      break;
    default:
      print_help(argv);
      return EXIT_FAILURE;
      break;
    }
  }

  int do_load = (optind < argc);

  res = libusb_init(&info.stinfo_usb_ctx);
rescan:
  info.stinfo_dev_handle = NULL;
  libusb_device **devs;
  int n_devs = libusb_get_device_list(info.stinfo_usb_ctx, &devs);
    if (n_devs < 0)
	goto exit_libusb;
    for (int i = 0;  devs[i]; i++) {
	libusb_device *dev =  devs[i];
	struct libusb_device_descriptor desc;
	int res = libusb_get_device_descriptor(dev, &desc);
	if (res < 0)
	    continue;
	if ((desc.idVendor == OPENMOKO_VID) && (desc.idProduct == BMP_APPL_PID)) {
	    res = libusb_open(dev, &info.stinfo_dev_handle);
	    if (res < 0) {
	       fprintf(stderr, "Can not open BMP/Application!\n");
	       continue;
	    }
	    libusb_claim_interface(info.stinfo_dev_handle, BMP_DFU_IF);
	    res = libusb_control_transfer(
		info.stinfo_dev_handle,
		/* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
		/* bRequest      */ 0, /*DFU_DETACH,*/
		/* wValue        */ 1000,
		/* wIndex        */ BMP_DFU_IF,
		/* Data          */ NULL,
		/* wLength       */ 0,
		5000 );
	    libusb_release_interface(info.stinfo_dev_handle, 0);
	    if (res < 0) {
		fprintf(stderr, "BMP Switch failed\n");
		continue;
	    }
	    libusb_free_device_list(devs, 1);
	    usleep(2000000);
	    goto rescan;
	    break;
	}
	if (desc.idVendor != STLINK_VID)
	    continue;
	switch (desc.idProduct) {
	case STLINK_PID:
	    res = libusb_open(dev, &info.stinfo_dev_handle);
	    if (res < 0) {
	       fprintf(stderr, "Can not open STLINK/Bootloader!\n");
	       continue;
	    }
	    info.stinfo_ep_in  = 1 | LIBUSB_ENDPOINT_IN;
	    info.stinfo_ep_out = 2 | LIBUSB_ENDPOINT_OUT;
	    info.stinfo_bl_type = STLINK_BL_V2;
	    fprintf(stderr, "StlinkV21 Bootloader found\n");
	    break;
	case STLINK_PIDV3_BL:
	    res = libusb_open(dev,  &info.stinfo_dev_handle);
	    if (res < 0) {
	       fprintf(stderr, "Can not open STLINK-V3/Bootloader!\n");
	       continue;
	    }
	    info.stinfo_ep_in  = 1 | LIBUSB_ENDPOINT_IN;
	    info.stinfo_ep_out = 1 | LIBUSB_ENDPOINT_OUT;
	    info.stinfo_bl_type = STLINK_BL_V3;
	    fprintf(stderr, "StlinkV3 Bootloader found\n");
	    break;
	case STLINK_PIDV21:
	case STLINK_PIDV21_MSD:
	case STLINK_PIDV3:
	    res = libusb_open(dev, &info.stinfo_dev_handle);
	    if (res < 0) {
	       fprintf(stderr, "Can not open STLINK/Application!\n");
	       continue;
	    }
	    res = stlink_dfu_mode(info.stinfo_dev_handle, 0);
	    if (res != 0x8000) {
		libusb_release_interface(info.stinfo_dev_handle, 0);
		return 0;
	    }
	    stlink_dfu_mode(info.stinfo_dev_handle, 1);
	    libusb_release_interface(info.stinfo_dev_handle, 0);
	    libusb_free_device_list(devs, 1);
	    fprintf(stderr, "Trying to switch STLINK/Application to bootloader\n");
	    usleep(2000000);
	    goto rescan;
	    break;
	}
	if (info.stinfo_dev_handle)
	    break;}
    libusb_free_device_list(devs, 1);
    if (!info.stinfo_dev_handle) {
	fprintf(stderr, "No ST-Link in DFU mode found. Replug ST-Link to flash!\n");
	return EXIT_FAILURE;
    }

  if (libusb_claim_interface(info.stinfo_dev_handle, 0)) {
    fprintf(stderr, "Unable to claim USB interface ! Please close all programs that "
	    "may communicate with an ST-Link dongle.\n");
    return EXIT_FAILURE;
  }

  if (stlink_read_info(&info)) {
    libusb_release_interface(info.stinfo_dev_handle, 0);
    return EXIT_FAILURE;
  }
  printf("Firmware version : V%dJ%dS%d\n", info.stlink_version,
	 info.jtag_version, info.swim_version);
  printf("Loader version : %d\n", info.loader_version);
  printf("ST-Link ID : ");
  for (i = 0; i < 12; i += 4) {
    printf("%02X", info.id[i + 3]);
    printf("%02X", info.id[i + 2]);
    printf("%02X", info.id[i + 1]);
    printf("%02X", info.id[i + 0]);
  }
  printf("\n");
  printf("Firmware encryption key : ");
  for (i = 0; i < 16; i++) {
    printf("%02X", info.firmware_key[i]);
  }
  printf("\n");

  res = stlink_current_mode(&info);
  if (res < 0) {
    libusb_release_interface(info.stinfo_dev_handle, 0);
    return EXIT_FAILURE;
  }
  printf("Current mode : %d\n", res);

  if (res & 0xfffc) {
    printf("ST-Link dongle is not in the correct mode. Please unplug and plug the dongle again.\n");
    libusb_release_interface(info.stinfo_dev_handle, 0);
    return EXIT_SUCCESS;
  }

  if (!probe) {
    if (do_load)
      stlink_flash(&info, argv[optind]);
    stlink_exit_dfu(&info);
    }
   libusb_release_interface(info.stinfo_dev_handle, 0);
exit_libusb:
  libusb_exit(info.stinfo_usb_ctx);

  return EXIT_SUCCESS;
}
