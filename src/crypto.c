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
#include <string.h>
#include <arpa/inet.h>

#include "../tiny-AES-c/aes.h"
#include "crypto.h"

static void convert_to_big_endian(unsigned char *array, unsigned int length);

static void convert_to_big_endian(unsigned char *array, unsigned int length) {
  unsigned int i;

  for (i = 0; i < length; i += 4) {
    *(uint32_t*)(array+i) = htonl(*(uint32_t*)(array+i));
  }
}

void encrypt(unsigned char *key, unsigned char *data, unsigned int length) {
  struct AES_ctx ctx;
  unsigned char key_be[16];
  size_t i;

  memcpy(key_be, key, 16);
  convert_to_big_endian(key_be, 16);

  AES_init_ctx(&ctx, key_be);

  convert_to_big_endian(data, length);

  for (i = 0; i < length; i += 16) {
    AES_ECB_encrypt(&ctx, data+i);
  }
  convert_to_big_endian(data, length);
}
