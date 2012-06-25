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
    char *data;
    
    imx50_read_register(handle, 0, BITSOF(int), 500, (char**)&data);
    
    for (int i = 0; i < 500; i++)
		printf("%02hhx ", data[i]);
	printf("\n");
    
    return 0;
}

