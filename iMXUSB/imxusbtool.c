//
//  iMX50 USB Tool
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

#include <stdio.h>
#include "imxusb.h"

int main(int argc, const char * argv[]) {
    imx50_device_t *handle;
    unsigned char data[500];

    imx50_log_level(DEBUG_LOG);

    fprintf(stderr, "%s\n", "Waiting for device...");
    handle = imx50_init_device();
    fprintf(stderr, "%s\n", "Found device.");

    fprintf(stderr, "%s\n", "Set up memory.");
    imx50_init_memory(handle);
    
    fprintf(stderr, "%s\n", "Attempting to read 500 bytes from 0x0.");
    imx50_read_memory(handle, 0x0, data, 500);
    
    fprintf(stderr, "%s\n", "Closing device.");
    imx50_close_device(handle);
    return 0;
}

