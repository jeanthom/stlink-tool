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

enum DeviceStatus {
  OK = 0x00,
  errTARGET = 0x01,
  errFILE = 0x02,
  errWRITE = 0x03,
  errERASE = 0x04,
  errCHECK_ERASED = 0x05,
  errPROG = 0x06,
  errVERIFY = 0x07,
  errADDRESS = 0x08,
  errNOTDONE = 0x09,
  errFIRMWARE = 0x0A,
  errVENDOR = 0x0B,
  errUSBR = 0x0C,
  errPOR = 0x0D,
  errUNKNOWN = 0x0E,
  errSTALLEDPKT = 0x0F
};

enum DeviceState {
  appIDLE = 0,
  appDETACH = 1,
  dfuIDLE = 2,
  dfuDNLOAD_SYNC = 3,
  dfuDNBUSY = 4,
  dfuDNLOAD_IDLE = 5,
  dfuMANIFEST_SYNC = 6,
  dfuMANIFEST = 7,
  dfuMANIFEST_WAIT_RESET = 8,
  dfuUPLOAD_IDLE = 9,
  dfuERROR = 10
};

struct STLinkInfos {
  uint8_t firmware_key[16];
  uint8_t id[12];
  uint8_t stlink_version;
  uint8_t jtag_version;
  uint8_t swim_version;
  uint16_t loader_version;
};

struct DFUStatus {
  enum DeviceStatus bStatus : 8;
  unsigned int bwPollTimeout : 24;
  enum DeviceState bState : 8;
  unsigned char iString : 8;
};

int stlink_read_infos(libusb_device_handle *dev_handle,
		      struct STLinkInfos *infos);
int stlink_current_mode(libusb_device_handle *dev_handle);
int stlink_dfu_status(libusb_device_handle *dev_handle,
		       struct DFUStatus *status);
int stlink_dfu_download(libusb_device_handle *dev_handle,
			unsigned char *data,
			size_t data_len,
			uint16_t wBlockNum,
			struct STLinkInfos *stlink_infos);
int stlink_erase(libusb_device_handle *dev_handle,
		 uint32_t address);
int stlink_set_address(libusb_device_handle *dev_handle,
		       uint32_t address);
int stlink_flash(libusb_device_handle *dev_handle,
		 const char *filename,
		 unsigned int base_offset,
		 unsigned int chunk_size,
		 struct STLinkInfos *stlink_infos);
