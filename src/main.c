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
#include <libusb.h>

#include "stlink.h"

#define STLINK_VID 0x0483
#define STLINK_PID 0x3748

int main(int argc, char *argv[]) {
  libusb_context *usb_ctx;
  libusb_device_handle *dev_handle;
  struct STLinkInfos infos;
  int res, i;
  
  if (argc != 2) {
    printf("Usage: %s firmware.bin\n", argv[0]);
    return 0;
  }

  res = libusb_init(&usb_ctx);
  (void)res;

  dev_handle = libusb_open_device_with_vid_pid(usb_ctx,
					       STLINK_VID,
					       STLINK_PID);
  if (!dev_handle) {
    fprintf(stderr, "ST-Link could not be found !\n");
    return EXIT_FAILURE;
  }

  if (stlink_read_infos(dev_handle, &infos)) {
    return EXIT_FAILURE;
  }
  
  printf("Firmware version : V%dJ%dS%d\n", infos.stlink_version,
	 infos.jtag_version, infos.swim_version);
  printf("Loader version : %d\n", infos.loader_version);
  printf("ST-Link ID : ");
  for (i = 0; i < 12; i++) {
    printf("%02X", infos.id[i]);
  }
  printf("\n");
  printf("Firmware encryption key : ");
  for (i = 0; i < 16; i++) {
    printf("%02X", infos.firmware_key[i]);
  }
  printf("\n");

  res = stlink_current_mode(dev_handle);
  if (res < 0) {
    return EXIT_FAILURE;
  }
  printf("Current mode : %d\n", res);

  if (res != 1) {
    printf("ST-Link dongle is not in the correct mode. Please unplug and plug the dongle again.\n");
    return EXIT_SUCCESS;
  }

  if (argc >= 2) {
    stlink_flash(dev_handle, argv[1], 0x8004000, 1024, &infos);
  }

  libusb_exit(usb_ctx);
  
  return EXIT_SUCCESS;
}
