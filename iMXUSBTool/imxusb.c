//
//  imxusb.c
//  iMXUSBTool
//
//  Created by Yifan Lu on 5/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imxusb.h"
#include <unistd.h>

/**
    @brief Get a iMX50 usb download device
    
    This function will block until a device is found.
 
    @return A device will be returned on success, NULL on error
*/
hid_device *imx50_get_device() {
    hid_device *handle = NULL;
    struct hid_device_info *dev;
    
    while(handle == NULL) {
        dev = hid_enumerate(IMX50_VID, IMX50_PID);
        if(dev == NULL) {
            sleep(5);
            continue; // loop
        }
        handle = hid_open(dev->vendor_id, dev->product_id, dev->serial_number);
        hid_free_enumeration(dev);
    }
    
    return handle;
}

/**
    @brief Prepares a command to be sent
 
    The first report is the command. The structure we have 
    is not packed, and is in little-endian. This converts 
    the byte order and packs the command. The returned byte 
    array will always be of length REPORT_SDP_CMD_SIZE.
 
    @param command The command to pack.
    
    @return A dynamically allocated byte array of length 
        REPORT_SDP_CMD_SIZE. NULL if error.
 */
unsigned char *imx50_pack_command(sdp_t *command) {
    unsigned char *data = malloc(REPORT_SDP_CMD_SIZE);
    uint32_t *payload = (uint32_t*)data+1; // first byte is number
    if(!data) {
        return NULL;
    }
    
    memset(data, 0, REPORT_SDP_CMD_SIZE);
    *(uint8_t*)data = REPORT_ID_SDP_CMD; // first report
    payload[0] = (  ((command->address  & 0x00FF0000) << 8) 
                     | ((command->address  & 0xFF000000) >> 8) 
                     |  (command->command_type   & 0x0000FFFF) );
    
	payload[1] = (   (command->data_count & 0xFF000000)
                     | ((command->format   & 0x000000FF) << 16)
                     | ((command->address  & 0x000000FF) <<  8)
                     | ((command->address  & 0x0000FF00) >>  8 ));
    
	payload[2] = (   (command->data     & 0xFF000000)
                     | ((command->data_count & 0x000000FF) << 16)
                     |  (command->data_count & 0x0000FF00)
                     | ((command->data_count & 0x00FF0000) >> 16));
    
	payload[3] = (  ((0x00  & 0x000000FF) << 24)
                     | ((command->data     & 0x00FF0000) >> 16) 
                     |  (command->data     & 0x0000FF00)
                     | ((command->data     & 0x000000FF) << 16));   
    
    return data;
}

/**
    @brief Sends the command to the device. (Report 1)
 
    This is the first report, it will pack and then send the 
    requested command.
 
    @param device The HID device to send to.
    @param command The command to send
 
    @return Zero on success, error code otherwise.
**/
int imx50_send_command(hid_device *device, sdp_t *command) {
    // pack the command
    unsigned char *data = imx50_pack_command(command);
    if(!data) {
        return ERROR_OUT_OF_MEMORY; // error packing
    }
    
    // send the report
    if(hid_write(device, data, REPORT_ID_SDP_CMD) < 0) {
        free(data);
        return ERROR_WRITE; // error sending
    }
    
    free(data);
    return 0;
}

/**
    @brief Sends data to the device. (Report 2)
 
    This is the second report. It is the data sent to the 
    device. The size of the data must be less than REPORT_DATA_SIZE 
    or an error will be returned.
 
    @param device The HID device to send data to.
    @param payload The data to send.
    @param size The length of the payload. MUST be less than or
        equal to REPORT_DATA_SIZE-1 (one byte for report number).
 
    @return Zero on success, error code otherwise.
**/
int imx50_send_data(hid_device *device, unsigned char *payload, unsigned int size) {
    if(size+1 > REPORT_DATA_SIZE) {
        return ERROR_PARAMETER;
    }
    unsigned char *data = malloc(size + 1);
    if(!data) {
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    data[0] = REPORT_ID_DATA;
    if(!memcpy(data+1, payload, REPORT_ID_DATA)) {
        free(data);
        return ERROR_IO; // cannot copy data
    }
    if(hid_write(device, data, size+1) < 0) {
        free(data);
        return ERROR_WRITE; // error sending
    }
    free(data);
    return 0;
}

/**
    @brief Reads HAB state. (Report 3)
 
    This is the third report. The device will return either 
    PRODUCTION_MODE (HAB enabled) or ENGINEER_MODE (HAB disabled).
 
    @param device The HID device to read from.
    
    @return HAB status on success, error code otherwise.
**/
int imx50_get_hab_type(hid_device *device) {
    unsigned char *data = malloc(REPORT_HAB_MODE_SIZE);
    int hab_type;
    
    if(!data) {
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    
    if(!memset(data, 0, REPORT_HAB_MODE_SIZE)) {
        free(data);
        return ERROR_IO; // cannot write to memory
    }
    
    if(hid_read(device, data, REPORT_HAB_MODE_SIZE) < 0) {
        free(data);
        return ERROR_READ;
    }
    hab_type = *(int*)(data+1);
    free(data);
    
    return hab_type;
}

/**
    @brief Gets the device's response. (Report 4)
 
    This is the fourth report. The device sends a response 
    back to the host.
 
    @param device the HID device to read from.
    @param payload_p A pointer to the buffer to read to. This 
        will be dynamically allocated.
    @param size_p A pointer to the size of the buffer.
 
    @return Zero on success, error code otherwise.
**/
int imx50_get_dev_ack(hid_device *device, unsigned char **payload_p, unsigned int *size_p) {
    unsigned char *data = malloc(REPORT_STATUS_SIZE);
    unsigned char *payload;
    if(!data) {
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    if(!memset(data, 0, REPORT_STATUS_SIZE)) {
        free(data);
        return ERROR_IO; // cannot write to memory
    }
    if(hid_read(device, data, REPORT_STATUS_SIZE) < 0) {
        free(data);
        return ERROR_READ;
    }
    payload = malloc(REPORT_STATUS_SIZE - 1);
    if(!payload) {
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    if(!memcpy(payload, data+1, REPORT_STATUS_SIZE-1)) {
        free(payload);
        return ERROR_IO; // cannot write to memory
    }
    // set return values
    *payload_p = payload;
    *size_p = REPORT_STATUS_SIZE-1;
    return 0;
}

// comment out old code
#if 0
/**
    @brief Reads values from the device
    
    This returns data read directly from the device at 
    the specified address. It returns an array containing 
    the data and must be freed.
 
    @param device The HID device to read from.
    @param address The address to start reading.
    @param format The size of each item in bits.
    @param count How many items to read.
    @param data The data to be dynamically allocated.
 
    @return Response code on success, -1 on error
*/
int imx50_read_register(hid_device *device, int address, char format, int count, void **data) {
    sdp_t command;
    unsigned char buf[65];
    char *data_p = NULL;
    int need = 0;
    int size = 0;
    int read;
    uint32_t ret = -1;
    
    // set up command container
    command.report_number = 1;
    command.command_type = BSWAP16(READ_REGISTER);
    command.address = BSWAP32((uint32_t)address);
    command.format = format;
    command.data_count = BSWAP32((uint32_t)count);
    need = (format / 8) * count;
    // send command
    fprintf(stderr, "%lu\n", sizeof(command));
    if(hid_write(device, (unsigned char*)&command, sizeof(command)) < 0) {
        // error sending data
        return -1;
    }
    // get response
    while(size < need) {
        read = hid_read_timeout(device, buf, sizeof(buf), 60000);
        if(read < 0)
            break;
        int report_number = (int)buf[0];
        void *buf_data = &buf[1];
        if(report_number == 3) { // our reponse code
            ret = *(uint32_t*)buf_data;
            ret = BSWAP32(ret);
        } else { // we got data
            // the +/- 1 is there to account for the extra byte at the beginning
            data_p = realloc(data_p, size + read-1);
            memcpy(data_p + read-1, buf_data, read-1);
            size += read-1;
        }
    }
    *data = data_p;
    
    return ret;
}
#endif

