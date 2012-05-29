//
//  imxusb.c
//  iMXUSBTool
//
//  Created by Yifan Lu on 5/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "imxusb.h"
#include "hidapi.h"
#include <unistd.h>

// this function will block until it finds a device
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