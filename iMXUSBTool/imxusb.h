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

#define REPORT_ID_SDP_CMD  1
#define REPORT_ID_DATA     2
#define REPORT_ID_HAB_MODE 3
#define REPORT_ID_STATUS   4
#define REPORT_SDP_CMD_SIZE   17
#define REPORT_DATA_SIZE   1025
#define REPORT_HAB_MODE_SIZE   5
#define REPORT_STATUS_SIZE   65

#define ERROR_OUT_OF_MEMORY 0x1
#define ERROR_IO 0x2
#define ERROR_WRITE 0x3
#define ERROR_READ 0x4
#define ERROR_PARAMETER 0x5

#define BITSOF(x) ( 8 * sizeof(x) )
#define BSWAP16(x) ( (x >> 8) | (x << 8) )
#define BSWAP32(x) ( (x >> 24) | ((x << 8) & 0x00FF0000) | ((x >> 8 ) & 0x0000FF00) | (x << 24) )
#define BSWAP64(x) ( (x >> 56) | ((x<<40) & 0x00FF000000000000) | ((x<<24) & 0x0000FF0000000000) | ((x<<8) & 0x000000FF00000000) | ((x>>8) & 0x00000000FF000000) | ((x>>24) & 0x0000000000FF0000) | ((x>>40) & 0x000000000000FF00) | (ull << 56) )

// all fields are big-endian, convert before sending
struct sdp {
    unsigned char report_number;
    unsigned short command_type;
    unsigned int address;
    unsigned char format;
    unsigned int data_count;
    unsigned int data;
    unsigned char reserved;
};

// all fields are big-endian, convert before sending
struct dcd {
    unsigned int data_format;
    unsigned int address;
    unsigned int value;
};

typedef struct sdp sdp_t;
typedef struct dcd dcd_t;

// helper functions
hid_device *imx50_get_device();
unsigned char *imx50_pack_command(sdp_t *command);

// reports
int imx50_send_command(hid_device *device, sdp_t *command);
int imx50_send_data(hid_device *device, unsigned char *payload, unsigned int size);
int imx50_get_hab_type(hid_device *device);
int imx50_get_dev_ack(hid_device *device, unsigned char **payload_p, unsigned int *size_p);

#endif
