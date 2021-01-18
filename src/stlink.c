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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <libusb.h>

#include "crypto.h"
#include "stlink.h"

#define USB_TIMEOUT 5000

#define DFU_DETACH 0x00
#define DFU_DNLOAD 0x01
#define DFU_UPLOAD 0x02
#define DFU_GETSTATUS 0x03
#define DFU_CLRSTATUS 0x04
#define DFU_GETSTATE 0x05
#define DFU_ABORT 0x06
#define DFU_EXIT  0x07
#define ST_DFU_INFO   0xF1
#define ST_DFU_MAGIC  0xF3

#define GET_COMMAND 0x00
#define SET_ADDRESS_POINTER_COMMAND 0x21
#define ERASE_COMMAND 0x41
#define ERASE_SECTOR_COMMAND 0x42
#define READ_UNPROTECT_COMMAND 0x92

static int stlink_erase(struct STLinkInfo *info,  uint32_t address);
static int stlink_set_address(struct STLinkInfo *info, uint32_t address);
static int stlink_dfu_status(struct STLinkInfo *info, struct DFUStatus *status);

int stlink_dfu_mode(libusb_device_handle *dev_handle, int trigger) {
    unsigned char data[16];
    int rw_bytes, res;

    memset(data, 0, sizeof(data));

    data[0] = 0xF9;
    if (trigger) data[1] = DFU_DNLOAD;
    /* Write */
    res = libusb_bulk_transfer(dev_handle,
			       1 | LIBUSB_ENDPOINT_OUT,
			       data,
			       sizeof(data),
			       &rw_bytes,
			       USB_TIMEOUT);
    if (res) {
	fprintf(stderr, "USB transfer failure\n");
	return -1;
    }
    if (!trigger) {
	/* Read */
	libusb_bulk_transfer(dev_handle,
			     1 | LIBUSB_ENDPOINT_IN,
			     data,
			     2,
			     &rw_bytes,
			     USB_TIMEOUT);
	if (res) {
	    fprintf(stderr, "stlink_read_info() failure\n");
	    return -1;
	}
    }
    return data[0] << 8 | data[1];
}

int stlink_read_info(struct STLinkInfo *info) {
  unsigned char data[20];
  int res, rw_bytes;

  memset(data, 0, sizeof(data));

  data[0] = ST_DFU_INFO;
  data[1] = 0x80;

  /* Write */
  res = libusb_bulk_transfer(info->stinfo_dev_handle,
			     info->stinfo_ep_out,
			     data,
			     16,
			     &rw_bytes,
			     USB_TIMEOUT);
  if (res) {
    fprintf(stderr, "stlink_read_info out transfer failure\n");
    return -1;
  }

  /* Read */
  res = libusb_bulk_transfer(info->stinfo_dev_handle,
		       info->stinfo_ep_in,
		       data,
		       6,
		       &rw_bytes,
		       USB_TIMEOUT);
  if (res) {
    fprintf(stderr, "stlink_read_info in transfer failure\n");
    return -1;
  }

  info->stlink_version = data[0] >> 4;
  info->jtag_version = (data[0] & 0x0F) << 2 | (data[1] & 0xC0) >> 6;
  info->swim_version = data[1] & 0x3F;
  info->loader_version = data[5] << 8 | data[4];

  memset(data, 0, sizeof(data));

  data[0] = ST_DFU_MAGIC;
  data[1] = 0x08;

  /* Write */
  res = libusb_bulk_transfer(info->stinfo_dev_handle,
			     info->stinfo_ep_out,
			     data,
			     16,
			     &rw_bytes,
			     USB_TIMEOUT);
  if (res) {
    fprintf(stderr, "USB transfer failure\n");
    return -1;
  }

  /* Read */
  libusb_bulk_transfer(info->stinfo_dev_handle,
		       info->stinfo_ep_in,
		       data,
		       20,
		       &rw_bytes,
		       USB_TIMEOUT);
  if (res) {
    fprintf(stderr, "USB transfer failure\n");
    return -1;
  }

  memcpy(info->id, data+8, 12);

  /* Firmware encryption key generation */
  memcpy(info->firmware_key, data, 4);
  memcpy(info->firmware_key+4, data+8, 12);
  my_encrypt((unsigned char*)"I am key, wawawa", info->firmware_key, 16);

  return 0;
}

int stlink_current_mode(struct STLinkInfo *info) {
  unsigned char data[16];
  int rw_bytes, res;

  memset(data, 0, sizeof(data));

  data[0] = 0xF5;

  /* Write */
  res = libusb_bulk_transfer(info->stinfo_dev_handle,
			     info->stinfo_ep_out,
			     data,
			     sizeof(data),
			     &rw_bytes,
			     USB_TIMEOUT);
  if (res) {
    fprintf(stderr, "USB transfer failure\n");
    return -1;
  }

  /* Read */
  libusb_bulk_transfer(info->stinfo_dev_handle,
		       info->stinfo_ep_in,
		       data,
		       2,
		       &rw_bytes,
		       USB_TIMEOUT);
  if (res) {
    fprintf(stderr, "stlink_read_info() failure\n");
    return -1;
  }

  return data[0] << 8 | data[1];
}

uint16_t stlink_checksum(const unsigned char *firmware,
			 size_t len) {
  unsigned int i;
  int ret = 0;

  for (i = 0; i < len; i++) {
    ret += firmware[i];
  }

  return (uint16_t)ret & 0xFFFF;
}

int stlink_dfu_download(struct STLinkInfo *info,
			unsigned char *data,
			const size_t data_len,
			const uint16_t wBlockNum) {
  unsigned char download_request[16];
  struct DFUStatus dfu_status;
  int rw_bytes, res;
  memset(download_request, 0, sizeof(download_request));

  download_request[0] = ST_DFU_MAGIC;
  download_request[1] = DFU_DNLOAD;
  *(uint16_t*)(download_request+2) = wBlockNum; /* wValue */
  *(uint16_t*)(download_request+4) = stlink_checksum(data, data_len); /* wIndex */
  *(uint16_t*)(download_request+6) = data_len; /* wLength */

  if (wBlockNum >= 2) {
    my_encrypt(info->firmware_key, data, data_len);
  }

  res = libusb_bulk_transfer(info->stinfo_dev_handle,
			       info->stinfo_ep_out,
			       download_request,
			       sizeof(download_request),
			       &rw_bytes,
			       USB_TIMEOUT);
  if (res || rw_bytes != sizeof(download_request)) {
    fprintf(stderr, "USB transfer failure\n");
    return -1;
  }
  res = libusb_bulk_transfer(info->stinfo_dev_handle,
			     info->stinfo_ep_out,
			     data,
			     data_len,
			     &rw_bytes,
			     USB_TIMEOUT);
  if (res || rw_bytes != (int)data_len) {
    fprintf(stderr, "USB transfer failure 1\n");
    return -1;
  }

  if (stlink_dfu_status(info, &dfu_status)) {
    return -1;
  }

  if (dfu_status.bState != dfuDNBUSY) {
    fprintf(stderr, "Unexpected DFU state : %d\n", dfu_status.bState);
    return -2;
  }

  if (dfu_status.bStatus != OK) {
    fprintf(stderr, "Unexpected DFU status : %d\n", dfu_status.bStatus);
    return -3;
  }

  usleep(dfu_status.bwPollTimeout * 1000);

  if (stlink_dfu_status(info, &dfu_status)) {
    return -1;
  }

  if (dfu_status.bState != dfuDNLOAD_IDLE) {
    if (dfu_status.bStatus == errVENDOR) {
      fprintf(stderr, "Read-only protection active\n");
      return -3;
    } else if (dfu_status.bStatus == errTARGET) {
      fprintf(stderr, "Invalid address error\n");
      return -3;
    } else {
      fprintf(stderr, "Unknown error : %d\n", dfu_status.bStatus);
      return -3;
    }
  }

  return 0;
}

int stlink_dfu_status(struct STLinkInfo *info, struct DFUStatus *status) {
  unsigned char data[16];
  int rw_bytes, res;

  memset(data, 0, sizeof(data));

  data[0] = ST_DFU_MAGIC;
  data[1] = DFU_GETSTATUS;
  data[6] = 0x06; /* wLength */

  res = libusb_bulk_transfer(info->stinfo_dev_handle,
			     info->stinfo_ep_out,
			     data,
			     16,
			     &rw_bytes,
			     USB_TIMEOUT);
  if (res || rw_bytes != 16) {
    fprintf(stderr, "USB transfer failure\n");
    return -1;
  }
  res = libusb_bulk_transfer(info->stinfo_dev_handle,
			      info->stinfo_ep_in,
			     data,
			     6,
			     &rw_bytes,
			     USB_TIMEOUT);
  if (res || rw_bytes != 6) {
    fprintf(stderr, "USB transfer failure\n");
    return -1;
  }

  status->bStatus = data[0];
  status->bwPollTimeout = data[1] | data[2] << 8 | data[3] << 16;
  status->bState = data[4];
  status->iString = data[5];
  
  return 0;
}

int stlink_erase(struct STLinkInfo *info, uint32_t address) {
  unsigned char command[5];
  int res;

  command[0] = ERASE_COMMAND;
  command[1] = address & 0xFF;
  command[2] = (address >> 8) & 0xFF;
  command[3] = (address >> 16) & 0xFF;
  command[4] = (address >> 24) & 0xFF;

  res =  stlink_dfu_download(info, command, sizeof(command), 0);

  return res;
}

int stlink_sector_erase(struct STLinkInfo *info, uint32_t sector) {
  unsigned char command[5];
  int res;

  command[0] = ERASE_SECTOR_COMMAND;
  command[1] = sector & 0xFF;
  command[2] = 0;
  command[3] = 0;
  command[4] = 0;

  res =  stlink_dfu_download(info, command, sizeof(command), 0);

  return res;
}

int stlink_set_address(struct STLinkInfo *info,  uint32_t address) {
  unsigned char set_address_command[5];
  int res;

  set_address_command[0] = SET_ADDRESS_POINTER_COMMAND;
  set_address_command[1] = address & 0xFF;
  set_address_command[2] = (address >> 8) & 0xFF;
  set_address_command[3] = (address >> 16) & 0xFF;
  set_address_command[4] = (address >> 24) & 0xFF;

  res = stlink_dfu_download(info, set_address_command, sizeof(set_address_command), 0);
  return res;
}

int stlink_flash(struct STLinkInfo *info, const char *filename) {
  unsigned int file_size;
  int fd, res;
  struct stat firmware_stat;
  unsigned char *firmware;

  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "File opening failed\n");
    return -1;
  }
  
  fstat(fd, &firmware_stat);

  file_size = firmware_stat.st_size;
  if (!file_size)
      return -1;

  firmware = mmap(NULL, file_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (firmware == MAP_FAILED) {
    fprintf(stderr, "mmap failure\n");
    return -1;
  }

  printf("Type %s\n",  (info->stinfo_bl_type == STLINK_BL_V3) ? "V3" : "V2");
  unsigned int base_offset;
  base_offset =  (info->stinfo_bl_type == STLINK_BL_V3) ? 0x08020000 : 0x08004000;
  int chunk_size = (1 << 10);
  unsigned int flashed_bytes = 0;
  while (flashed_bytes < file_size) {
    unsigned int cur_chunk_size;
    if ((flashed_bytes + chunk_size) > file_size) {
	cur_chunk_size = file_size - flashed_bytes;
    } else {
	cur_chunk_size = chunk_size;
    }
    int wdl = 2;
    if (info->stinfo_bl_type == STLINK_BL_V3) {
	if (((base_offset + flashed_bytes) & ((1 << 14) - 1)) == 0) {
	    uint32_t sector_start[8] = {0x08000000, 0x08004000, 0x08008000, 0x0800C000,
					0x08010000, 0x08020000, 0x08040000, 0x08060000};
	    int sector = -1;
	    int i = 0;
	    for (; i < 8; i++) {
		if (sector_start[i] == base_offset + flashed_bytes) {
		    sector = i;
		    break;
		}
	    }
	    if (i < 0) {
		fprintf(stderr, "No sector match for address %08x\n", base_offset + flashed_bytes);
		return i;
	    }
	    printf("Erase sector %d\n", sector);
	    res = stlink_sector_erase(info, sector);
	    if (res) {
		fprintf(stderr, "Erase sector %d failed\n", sector);
		return res;
	    }
	    printf("Erase sector %d done\n", sector);
	} else {
	    wdl = 3;
	}
    } else {
	res = stlink_erase(info, base_offset + flashed_bytes);
	if (res) {
	    fprintf(stderr, "Erase error at 0x%08x\n", base_offset + flashed_bytes);
	    return res;
	}
    }
    res = stlink_set_address(info, base_offset+flashed_bytes);
    if (res) {
      fprintf(stderr, "set address error at 0x%08x\n", base_offset + flashed_bytes);
      return res;
    } else {
	printf("set address to 0x%08x done\n", base_offset + flashed_bytes);
    }
    res = stlink_dfu_download(info, firmware + flashed_bytes, chunk_size, wdl);
    if (res) {
	fprintf(stderr, "Download error at 0x%08x\n", base_offset + flashed_bytes);
	return res;
    } else {
	printf("Download at 0x%08x done\n", base_offset + flashed_bytes);
    }

    printf(".");
    fflush(stdout); /* Flush stdout buffer */

    flashed_bytes += cur_chunk_size;
  }

  munmap(firmware, file_size);
  close(fd);

  printf("\n");

  return 0;
}

int stlink_exit_dfu(struct STLinkInfo *info) {
  unsigned char data[16];
  int rw_bytes, res;
  
  memset(data, 0, sizeof(data));
  
  data[0] = ST_DFU_MAGIC;
  data[1] = DFU_EXIT;
  
  res = libusb_bulk_transfer(info->stinfo_dev_handle,
			     info->stinfo_ep_out,
			     data,
			     16,
			     &rw_bytes,
			     USB_TIMEOUT);
  if (res || rw_bytes != 16) {
    fprintf(stderr, "USB transfer failure\n");
    return -1;
  }
  return 0;
}
