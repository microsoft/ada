// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _CRC32_H
#define _CRC32_H

#include <stdint.h>

// calculate a checksum on a buffer, length = bytelength
uint32_t crc32(uint8_t *buffer, uint32_t bytelength);

#endif