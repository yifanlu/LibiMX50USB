//
//  imxusb.h
//  iMXUSBTool
//
//  Created by Yifan Lu on 5/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef iMXUSBTool_imxusb_h
#define iMXUSBTool_imxusb_h

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


// all fields are big-endian, convert before sending
struct sdp {
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

#endif
