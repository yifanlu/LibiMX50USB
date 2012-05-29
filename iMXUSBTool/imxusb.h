//
//  imxusb.h
//  iMXUSBTool
//
//  Created by Yifan Lu on 5/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef iMXUSBTool_imxusb_h
#define iMXUSBTool_imxusb_h

#include "hidapi.h"
#include <stdint.h>

#define IMX50_VID       0x15A2
#define IMX50_PID       0x0052

#define READ_REGISTER   0x0101
#define WRITE_REGISTER  0x0202
#define WRITE_FILE      0x0404
#define ERROR_STATUS    0x0505
#define DCD_WRITE       0x0A0A
#define JUMP_ADDRESS    0x0B0B

#define PRODUCTION_MODE 0x12343412
#define ENGINEER_MODE   0x56787856

#define WRITE_COMPLETE  0x128A8A12
#define FILE_COMPLETE   0x88888888

#define BSWAP16 (x) ( (x >> 8) | (x << 8) )
#define BSWAP32 (x) ( (x >> 24) | ((x << 8) & 0x00FF0000) | ((x >> 8 ) & 0x0000FF00) | (x << 24) )
#define BSWAP64 (x) ( (x >> 56) | ((x<<40) & 0x00FF000000000000) | ((x<<24) & 0x0000FF0000000000) | ((x<<8) & 0x000000FF00000000) | ((x>>8) & 0x00000000FF000000) | ((x>>24) & 0x0000000000FF0000) | ((x>>40) & 0x000000000000FF00) | (ull << 56) )

// all fields are big-endian, convert before sending
struct sdp {
    uint8_t report_number;
    uint16_t command_type;
    uint32_t address;
    uint8_t format;
    uint32_t data_count;
    uint32_t data;
    uint8_t reserved;
};

// all fields are big-endian, convert before sending
struct dcd {
    uint32_t data_format;
    uint32_t address;
    uint32_t value;
};

typedef struct sdp sdp_t;
typedef struct dcd dcd_t;

hid_device *imx50_get_device();

#endif
