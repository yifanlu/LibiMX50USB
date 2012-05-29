//
//  main.c
//  iMXUSBTool
//
//  Created by Yifan Lu on 5/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "hidapi.h"
#include "imxusb.h"

int main(int argc, const char * argv[])
{
    printf("%s\n", "Waiting for device...");
    hid_device *handle = imx50_get_device();
    printf("%s\n", "Found device.");
    
    
    
    
    return 0;
}

