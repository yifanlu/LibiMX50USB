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
#ifdef _WIN32
    #include <windows.h>
#endif
#include "imxusb.h"

int main(int argc, const char * argv[]) {
    imx50_device_t *handle;
    unsigned char data[500];
    unsigned const char *payload = "abcdefghijklmnopqrstuvwxyz";
    unsigned int value = 0xdeadbeef;

    imx50_log_level(DEBUG_LOG);
    handle = imx50_init_device();

    if(imx50_kindle_init(handle) != 0) {
        return 1;
    }

    fprintf(stderr, "%s\n", "!!! Writing memory");
    imx50_write_memory(handle, 0x70800000, payload, strlen(payload));
    fprintf(stderr, "!!! device status: %X\n", imx50_error_status(handle));

    fprintf(stderr, "%s\n", "!!! Reading memory");
    imx50_read_memory(handle, 0x70800000, data, strlen(payload));
    fprintf(stderr, "!!! device status: %X\n", imx50_error_status(handle));

    /*
    for(;;){
        if(imx50_read_memory(handle, 0x63F80004, &value, 4) != 0)
            break;
        printf("%X\n", value);
    }
    */

    imx50_close_device(handle);
    return 0;
}

