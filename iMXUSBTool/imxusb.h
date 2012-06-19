//
//  iMX50 USB Tool Library
//
//  Created by Yifan Lu
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef iMXUSBTool_imxusb_h
#define iMXUSBTool_imxusb_h

#define IMX50_VID               0x15A2
#define IMX50_PID               0x0052

#define CMD_READ_REGISTER       0x0101
#define CMD_WRITE_REGISTER      0x0202
#define CMD_WRITE_FILE          0x0404
#define CMD_ERROR_STATUS        0x0505
#define CMD_HEADER              0x0606 // unused
#define CMD_RE_ENUM             0x0909 // unused
#define CMD_DCD_WRITE           0x0A0A
#define CMD_JUMP_ADDRESS        0x0B0B

#define HAB_PRODUCTION_MODE     0x12343412
#define HAB_ENGINEER_MODE       0x56787856

#define ACK_WRITE_COMPLETE      0x128A8A12
#define ACK_FILE_COMPLETE       0x88888888

#define MAX_DCD_WRITE_REG_CNT   85

#define REPORT_ID_SDP_CMD       1
#define REPORT_ID_DATA          2
#define REPORT_ID_HAB_MODE      3
#define REPORT_ID_STATUS        4

#define REPORT_SDP_CMD_SIZE     17
#define REPORT_DATA_SIZE        1025
#define REPORT_HAB_MODE_SIZE    5
#define REPORT_STATUS_SIZE      65

#define ERROR_OUT_OF_MEMORY     -1
#define ERROR_IO                -2
#define ERROR_WRITE             -3
#define ERROR_READ              -4
#define ERROR_PARAMETER         -5
#define ERROR_COMMAND           -6
#define ERROR_RETURN            -7

#define BITSOF(x)               ( 8 * sizeof(x) )
#define BSWAP16(x)              ( (x >> 8) | (x << 8) )
#define BSWAP32(x)              ( (x >> 24) | ((x << 8) & 0x00FF0000) | ((x >> 8 ) & 0x0000FF00) | (x << 24) )
#define BSWAP64(x)              ( (x >> 56) | ((x<<40) & 0x00FF000000000000) | ((x<<24) & 0x0000FF0000000000) | ((x<<8) & 0x000000FF00000000) | ((x>>8) & 0x00000000FF000000) | ((x>>24) & 0x0000000000FF0000) | ((x>>40) & 0x000000000000FF00) | (ull << 56) )

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

// abstration for hid_device
struct imx50_device;

typedef struct sdp sdp_t;
typedef struct dcd dcd_t;
typedef struct imx50_device imx50_device_t;

// helper functions (hidden to user)
//unsigned char *imx50_pack_command(sdp_t *command);

// device`management
imx50_device_t *imx50_init_device();
void imx50_close_device(imx50_device_t *device);

// reports
int imx50_send_command(imx50_device_t *device, sdp_t *command);
int imx50_send_data(imx50_device_t *device, unsigned char *payload, unsigned int size);
int imx50_get_hab_type(imx50_device_t *device);
int imx50_get_dev_ack(imx50_device_t *device, unsigned char **payload_p, unsigned int *size_p);

// commands
int imx50_read_memory(imx50_device_t *device, unsigned int address, unsigned char *buffer, unsigned int count);
int imx50_write_register(imx50_device_t *device, unsigned int address, unsigned int data);
int imx50_write_memory(imx50_device_t *device, unsigned int address, unsigned char *buffer, unsigned int count);
int imx50_error_status(imx50_device_t *device);
int imx50_dcd_write(imx50_device_t *device, dcd_t *buffer, unsigned int count);
int imx50_jump(imx50_device_t *device, unsigned int address);

#endif
