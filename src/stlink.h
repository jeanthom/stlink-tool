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

enum BlTypes {
    STLINK_BL_V2 = 0,
    STLINK_BL_V3
};

struct STLinkInfo {
  uint8_t firmware_key[16];
  uint8_t id[12];
  uint8_t stlink_version;
  uint8_t jtag_version;
  uint8_t swim_version;
  uint16_t loader_version;
    libusb_context *stinfo_usb_ctx;
    libusb_device_handle *stinfo_dev_handle;
    unsigned char stinfo_ep_in;
    unsigned char stinfo_ep_out;
    enum BlTypes stinfo_bl_type;
};

struct DFUStatus {
  enum DeviceStatus bStatus : 8;
  unsigned int bwPollTimeout : 24;
  enum DeviceState bState : 8;
  unsigned char iString : 8;
};

int stlink_dfu_mode(libusb_device_handle *dev_handle, int trigger);
int stlink_read_info(struct STLinkInfo *info);
int stlink_current_mode(struct STLinkInfo *info);
int stlink_dfu_download(struct STLinkInfo *stlink_info,
			unsigned char *data,
			const size_t data_len,
			const uint16_t wBlockNum);
int stlink_flash(struct STLinkInfo *stlink_info, const char *filename);
int stlink_exit_dfu(struct STLinkInfo *info);
