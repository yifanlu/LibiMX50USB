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
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "imxusb.h"

#define REMOVE_ARG      argc--; argv++

extern void imx50_hex_dump(unsigned char *data, unsigned int size, unsigned int num);

const char *HELP = 
    "usage: imxusbtool mode [options] address file|length|value\n"
    "   modes:\n"
    "       -r  Read from the device\n"
    "       -w  Write to the device\n"
    "       -j  Jump to an address\n"
    "       -g  R/W a register\n"
    "   options:\n"
    "       -n  For jumps, do not add header\n"
    "           Device requires header for jumps.\n"
    "       -x  For reading, output as hex dump\n"
    "           instead of binary data.\n"
    "       -k  Set up device as a Kindle\n"
    "       -h  This help\n"
    "       -d  Debug output\n"
    "   address:\n"
    "       All modes. Address to interact with.\n"
    "   file:\n"
    "       Write mode only. Name of file to download.\n"
    "   length:\n"
    "       Read mode only. Number of bytes to read.\n"
    "   value:\n"
    "       Register mode only. uint value to write.\n"
    "       Leave blank to read register.";

typedef enum {
    None,
    Read,
    Write,
    Jump,
    RegisterRead,
    RegisterWrite
} imx50_mode_t;

typedef struct {
    int add_header;
    int hex_dump;
    int kindle;
} imx50_options_t;

int main(int argc, const char * argv[]) {
    imx50_device_t *handle = NULL;
    imx50_mode_t mode = None;
    imx50_options_t options = {1, 0, 0};
    device_addr_t address = 0;
    char *filename = NULL;
    unsigned int length = 0;
    unsigned int value = 0;
    unsigned char *read_buffer;
    
    // default log level
    imx50_log_level(WARNING_LOG);
    
    /* parse arguments */
    REMOVE_ARG; // first argument is useless
    const char *arg;
    while(argc > 0) {
        arg = argv[0];
        if(arg[0] == '-'){ // option
            switch(arg[1]){
                case 'r':
                    mode = Read;
                    break;
                case 'w':
                    mode = Write;
                    break;
                case 'j':
                    mode = Jump;
                    break;
                case 'g':
                    mode = RegisterRead;
                    break;
                case 'n':
                    options.add_header = 0;
                    break;
                case 'x':
                    options.hex_dump = 1;
                    break;
                case 'k':
                    options.kindle = 1;
                    break;
                case 'd':
                    imx50_log_level(DEBUG_LOG);
                    break;
                case '?':
                case 'h':
                default:
                    goto arg_error;
                    break;
            }
            REMOVE_ARG;
        } else { // no more options
            break;
        }
    }
    if(argc > 2) { // too many arguments
        fprintf(stderr, "Too many arguments\n");
        goto arg_error;
    }else if(argc == 0) { // not enough arguments
        fprintf(stderr, "Not enough arguments\n");
        goto arg_error;
    }
    arg = argv[0];
    address = (unsigned int)strtol(arg, NULL, (arg[1] == 'x' || arg[1] == 'X') ? 16 : 10); // get address
    REMOVE_ARG;
    // final error check
    switch(mode){
        case Read:
            if(argc < 1){
                fprintf(stderr, "Not enough arguments\n");
                goto arg_error;
            }else{
                arg = argv[0];
                length = (unsigned int)strtol(arg, NULL, (arg[1] == 'x' || arg[1] == 'X') ? 16 : 10);
                REMOVE_ARG;
            }
            break;
        case Write:
            if(argc < 1){
                fprintf(stderr, "Not enough arguments\n");
                goto arg_error;
            }else{
                filename = strdup(argv[0]);
                REMOVE_ARG;
            }
            break;
        case Jump:
            if(argc > 0){
                fprintf(stderr, "Too many arguments\n");
                goto arg_error;
            }
            break;
        case RegisterRead:
            if(argc > 0){
                arg = argv[0];
                value = (unsigned int)strtol(arg, NULL, (arg[1] == 'x' || arg[1] == 'X') ? 16 : 10);
                REMOVE_ARG;
                mode = RegisterWrite;
            }
            break;
        case None:
        default:
            fprintf(stderr, "No mode selected\n");
            goto arg_error;
    }
    
    /* wait for device */
    fprintf(stderr, "Waiting for device...\n");
    handle = imx50_init_device();
    if(handle == NULL){
        fprintf(stderr, "Error connecting to device.\n");
        return 1;
    }else{
        fprintf(stderr, "Found a device.\n");
    }
    
    /* init the device */
    if(options.kindle && imx50_kindle_init(handle) != 0) {
        fprintf(stderr, "Error initializing the Kindle.\n");
        return 1;
    }
    
    /* do tasks */
    switch(mode) {
        case RegisterRead:
            length = sizeof(int);
        case Read:
            fprintf(stderr, "Reading %0#8X for %u bytes...\n", address, length);
            read_buffer = malloc(length);
            if(imx50_read_memory(handle, address, read_buffer, length) != 0){
                fprintf(stderr, "Error reading from the device.\n");
                goto error;
            }
            if(options.hex_dump){
                imx50_hex_dump(read_buffer, length, 16);
            }else if(mode == RegisterRead){
                fprintf(stdout, "%0#8X\n", *(unsigned int*)read_buffer);
            }else{
                fwrite(read_buffer, sizeof(char), length, stdout);
            }
            free(read_buffer);
            break;
        case Write:
            fprintf(stderr, "Writing %s to %0#8X...\n", filename, address);
            if(imx50_load_file(handle, address, filename) != 0){
                fprintf(stderr, "Error writing to the device.\n");
                goto error;
            }
            break;
        case Jump:
            fprintf(stderr, "Jumping to %0#8X...\n", address);
            if(options.add_header){
                address = imx50_add_header(handle, address);
            }
            if(imx50_jump(handle, address) != 0){
                fprintf(stderr, "Error jumping.\n");
                goto error;
            }
            break;
        case RegisterWrite:
            fprintf(stderr, "Writing %0#8X to %0#8X...\n", value, address);
            if(imx50_write_register(handle, address, value, BITSOF(int)) != 0){
                fprintf(stderr, "Error writing to the device.\n");
                goto error;
            }
        case None:
        default:
            fprintf(stderr, "Unknown error.\n");
            goto error;
    }
    
    /* clean up */
    imx50_close_device(handle);
    
    free(filename);
    return 0;
arg_error:
    fprintf(stderr, "%s\n", HELP);
error:
    return 1;
    
}

