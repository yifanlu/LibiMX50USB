//
//  imxusb.c
//  iMXUSBTool
//
//  Created by Yifan Lu on 5/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
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
    if(hid_write(device, &command, sizeof(command)) < 0) {
        // error sending data
        return -1;
    }
    // get response
    while(size < need) {
        read = hid_read_timeout(device, buf, sizeof(buf), 60000);
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
