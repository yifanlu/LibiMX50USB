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
#include <windows.h>
#include "imxusb.h"

static dcd_t lpddr2_init[] = 
    {
        { 32, 0x53FD4098, 0x80000003 }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fa86ac, 0x04000000 }, 
        { 32, 0x53fa866c, 0x00000200 }, 
        { 32, 0x53fa868c, 0x0 }, 
        { 32, 0x53fa8670, 0x00000000 }, 
        { 32, 0x53fa86a4, 0x00280000 }, 
        { 32, 0x53fa8668, 0x00280000 }, 
        { 32, 0x53fa8698, 0x00280000 }, 
        { 32, 0x53fa86a0, 0x00280000 }, 
        { 32, 0x53fa86a8, 0x00280000 }, 
        { 32, 0x53fa86b4, 0x00280000 }, 
        { 32, 0x53fa8490, 0x00280000 }, 
        { 32, 0x53fa8494, 0x00280000 }, 
        { 32, 0x53fa8498, 0x00280000 }, 
        { 32, 0x53fa849c, 0x00280000 }, 
        { 32, 0x53fa84f0, 0x00280000 }, 
        { 32, 0x53fa8500, 0x00280000 }, 
        { 32, 0x53fa84c8, 0x00280000 }, 
        { 32, 0x53fa8528, 0x00280000 }, 
        { 32, 0x53fa84f4, 0x00280000 }, 
        { 32, 0x53fa84fc, 0x00280000 }, 
        { 32, 0x53fa84cc, 0x00280000 }, 
        { 32, 0x53fa8524, 0x00280000 }, 
        { 32, 0x1400012C, 0x00000817 }, 
        { 32, 0x14000128, 0x09180000 }, 
        { 32, 0x14000124, 0x00310000 }, 
        { 32, 0x14000124, 0x00200000 }, 
        { 32, 0x14000128, 0x09180010 }, 
        { 32, 0x14000124, 0x00310000 }, 
        { 32, 0x14000124, 0x00200000 }, 
        { 32, 0x14000000, 0x00000500 }, 
        { 32, 0x14000008, 0x0000001b }, 
        { 32, 0x1400000c, 0x0000d056 }, 
        { 32, 0x14000010, 0x0000010b }, 
        { 32, 0x14000014, 0x00000a6b }, 
        { 32, 0x14000018, 0x0202000c }, 
        { 32, 0x1400001c, 0x0c110302 }, 
        { 32, 0x14000020, 0x05020503 }, 
        { 32, 0x14000024, 0x0048eb05 }, 
        { 32, 0x14000028, 0x00000606 }, 
        { 32, 0x1400002c, 0x09040501 }, 
        { 32, 0x14000030, 0x02000000 }, 
        { 32, 0x14000034, 0x00000e02 }, 
        { 32, 0x14000038, 0x00000006 }, 
        { 32, 0x1400003c, 0x00002301 }, 
        { 32, 0x14000040, 0x00050408 }, 
        { 32, 0x14000044, 0x00000300 }, 
        { 32, 0x14000048, 0x00260026 }, 
        { 32, 0x1400004c, 0x00010000 }, 
        { 32, 0x14000050, 0x00000000 }, 
        { 32, 0x14000054, 0x00000000 }, 
        { 32, 0x14000058, 0x00000000 }, 
        { 32, 0x1400005c, 0x02000000 }, 
        { 32, 0x14000060, 0x00000002 }, 
        { 32, 0x14000064, 0x00000000 }, 
        { 32, 0x14000068, 0x00000000 }, 
        { 32, 0x1400006c, 0x00040042 }, 
        { 32, 0x14000070, 0x00000002 }, 
        { 32, 0x14000074, 0x00000000 }, 
        { 32, 0x14000078, 0x00040042 }, 
        { 32, 0x1400007c, 0x00000001 }, 
        { 32, 0x14000080, 0x010b0000 }, 
        { 32, 0x14000084, 0x00000060 }, 
        { 32, 0x14000088, 0x02400018 }, 
        { 32, 0x1400008c, 0x01000e00 }, 
        { 32, 0x14000090, 0x0a010101 }, 
        { 32, 0x14000094, 0x01011f1f }, 
        { 32, 0x14000098, 0x01010101 }, 
        { 32, 0x1400009c, 0x00010101 }, 
        { 32, 0x140000a0, 0x00010000 }, 
        { 32, 0x140000a4, 0x00010000 }, 
        { 32, 0x140000a8, 0x00000000 }, 
        { 32, 0x140000ac, 0x00000fff }, 
        { 32, 0x140000c8, 0x02020101 }, 
        { 32, 0x140000cc, 0x01000000 }, 
        { 32, 0x140000d0, 0x01000201 }, 
        { 32, 0x140000d4, 0x00000200 }, 
        { 32, 0x140000d8, 0x00000102 }, 
        { 32, 0x140000dc, 0x0000ffff }, 
        { 32, 0x140000e0, 0x0000ff00 }, 
        { 32, 0x140000e4, 0x02020000 }, 
        { 32, 0x140000e8, 0x02020202 }, 
        { 32, 0x140000ec, 0x00000202 }, 
        { 32, 0x140000f0, 0x01010064 }, 
        { 32, 0x140000f4, 0x01010101 }, 
        { 32, 0x140000f8, 0x00010101 }, 
        { 32, 0x140000fc, 0x00000064 }, 
        { 32, 0x14000100, 0x00000000 }, 
        { 32, 0x14000104, 0x02000802 }, 
        { 32, 0x14000108, 0x04080000 }, 
        { 32, 0x1400010c, 0x04080408 }, 
        { 32, 0x14000110, 0x04080408 }, 
        { 32, 0x14000114, 0x03060408 }, 
        { 32, 0x14000118, 0x00010002 }, 
        { 32, 0x1400011c, 0x00001000 }, 
        { 32, 0x14000200, 0x00000000 }, 
        { 32, 0x14000204, 0x00000000 }, 
        { 32, 0x14000208, 0x35003a27 }, 
        { 32, 0x14000210, 0x35003a27 }, 
        { 32, 0x14000218, 0x35003a27 }, 
        { 32, 0x14000220, 0x35003a27 }, 
        { 32, 0x14000228, 0x35003a27 }, 
        { 32, 0x1400020c, 0x380002e2 }, 
        { 32, 0x14000214, 0x380002e2 }, 
        { 32, 0x1400021c, 0x380002e2 }, 
        { 32, 0x14000224, 0x380002e2 }, 
        { 32, 0x1400022c, 0x380002e2 }, 
        { 32, 0x14000230, 0x00000000 }, 
        { 32, 0x14000234, 0x00810006 }, 
        { 32, 0x14000238, 0x60101014 }, 
        { 32, 0x14000240, 0x60101014 }, 
        { 32, 0x14000248, 0x60101014 }, 
        { 32, 0x14000250, 0x60101014 }, 
        { 32, 0x14000258, 0x60101014 }, 
        { 32, 0x1400023c, 0x00101201 }, 
        { 32, 0x14000244, 0x00101201 }, 
        { 32, 0x1400024c, 0x00101201 }, 
        { 32, 0x14000254, 0x00101201 }, 
        { 32, 0x1400025c, 0x00101201 }, 
        { 32, 0x14000000, 0x00000501 }
    };

static dcd_t lpddr1_init[] = 
    {
        { 32, 0x53fa86AC, 0x0 }, 
        { 32, 0x53fa866C, 0x0 }, 
        { 32, 0x53fa868C, 0x0 }, 
        { 32, 0x53fa8670, 0x0 }, 
        { 32, 0x53fa86A4, 0x00180000 }, 
        { 32, 0x53fa8668, 0x00180000 }, 
        { 32, 0x53fa8698, 0x00180000 }, 
        { 32, 0x53fa86A0, 0x00180000 }, 
        { 32, 0x53fa86A8, 0x00180000 }, 
        { 32, 0x53fa86B4, 0x00180000 }, 
        { 32, 0x53fa8490, 0x00180000 }, 
        { 32, 0x53fa8494, 0x00180000 }, 
        { 32, 0x53fa8498, 0x00180000 }, 
        { 32, 0x53fa849c, 0x00180000 }, 
        { 32, 0x53fa84f0, 0x00180000 }, 
        { 32, 0x53fa8500, 0x00180000 }, 
        { 32, 0x53fa84c8, 0x00180000 }, 
        { 32, 0x53fa8528, 0x00180080 }, 
        { 32, 0x53fa84f4, 0x00180080 }, 
        { 32, 0x53fa84fc, 0x00180080 }, 
        { 32, 0x53fa84cc, 0x00180080 }, 
        { 32, 0x53fa8524, 0x00180080 }, 
        { 32, 0x1400012C, 0x00000408 }, 
        { 32, 0x14000128, 0x05090000 }, 
        { 32, 0x14000124, 0x00310000 }, 
        { 32, 0x14000124, 0x00200000 }, 
        { 32, 0x14000128, 0x05090010 }, 
        { 32, 0x14000124, 0x00310000 }, 
        { 32, 0x14000124, 0x00200000 }, 
        { 32, 0x14000000, 0x00000100 }, 
        { 32, 0x14000008, 0x00009c40 }, 
        { 32, 0x1400000C, 0x00000000 }, 
        { 32, 0x14000010, 0x00000000 }, 
        { 32, 0x14000014, 0x20000000 }, 
        { 32, 0x14000018, 0x01010006 }, 
        { 32, 0x1400001c, 0x080b0201 }, 
        { 32, 0x14000020, 0x02000303 }, 
        { 32, 0x14000024, 0x0036b002 }, 
        { 32, 0x14000028, 0x00000606 }, 
        { 32, 0x1400002c, 0x06030400 }, 
        { 32, 0x14000030, 0x01000000 }, 
        { 32, 0x14000034, 0x00000a02 }, 
        { 32, 0x14000038, 0x00000003 }, 
        { 32, 0x1400003c, 0x00001801 }, 
        { 32, 0x14000040, 0x00050612 }, 
        { 32, 0x14000044, 0x00000200 }, 
        { 32, 0x14000048, 0x001c001c }, 
        { 32, 0x1400004c, 0x00010000 }, 
        { 32, 0x1400005c, 0x01000000 }, 
        { 32, 0x14000060, 0x00000001 }, 
        { 32, 0x14000064, 0x00000000 }, 
        { 32, 0x14000068, 0x00320000 }, 
        { 32, 0x1400006c, 0x00000000 }, 
        { 32, 0x14000070, 0x00000000 }, 
        { 32, 0x14000074, 0x00320000 }, 
        { 32, 0x14000080, 0x02000000 }, 
        { 32, 0x14000084, 0x00000100 }, 
        { 32, 0x14000088, 0x02400040 }, 
        { 32, 0x1400008c, 0x01000000 }, 
        { 32, 0x14000090, 0x0a000100 }, 
        { 32, 0x14000094, 0x01011f1f }, 
        { 32, 0x14000098, 0x01010101 }, 
        { 32, 0x1400009c, 0x00030101 }, 
        { 32, 0x140000a4, 0x00010000 }, 
        { 32, 0x140000ac, 0x0000ffff }, 
        { 32, 0x140000c8, 0x02020101 }, 
        { 32, 0x140000cc, 0x00000000 }, 
        { 32, 0x140000d0, 0x01000202 }, 
        { 32, 0x140000d4, 0x00000200 }, 
        { 32, 0x140000d8, 0x00000001 }, 
        { 32, 0x140000dc, 0x0000ffff }, 
        { 32, 0x140000e4, 0x02020000 }, 
        { 32, 0x140000e8, 0x02020202 }, 
        { 32, 0x140000ec, 0x00000202 }, 
        { 32, 0x140000f0, 0x01010064 }, 
        { 32, 0x140000f4, 0x01010101 }, 
        { 32, 0x140000f8, 0x00010101 }, 
        { 32, 0x140000fc, 0x00000064 }, 
        { 32, 0x14000104, 0x02000602 }, 
        { 32, 0x14000108, 0x06120000 }, 
        { 32, 0x1400010c, 0x06120612 }, 
        { 32, 0x14000110, 0x06120612 }, 
        { 32, 0x14000114, 0x01030612 }, 
        { 32, 0x14000118, 0x00010002 }, 
        { 32, 0x1400011C, 0x00001000 }, 
        { 32, 0x14000200, 0x00000000 }, 
        { 32, 0x14000204, 0x00000000 }, 
        { 32, 0x14000208, 0x35002725 }, 
        { 32, 0x14000210, 0x35002725 }, 
        { 32, 0x14000218, 0x35002725 }, 
        { 32, 0x14000220, 0x35002725 }, 
        { 32, 0x14000228, 0x35002725 }, 
        { 32, 0x1400020c, 0x380002d0 }, 
        { 32, 0x14000214, 0x380002d0 }, 
        { 32, 0x1400021c, 0x380002d0 }, 
        { 32, 0x14000224, 0x380002d0 }, 
        { 32, 0x1400022c, 0x380002d0 }, 
        { 32, 0x14000230, 0x00000000 }, 
        { 32, 0x14000234, 0x00800006 }, 
        { 32, 0x14000238, 0x60101414 }, 
        { 32, 0x14000240, 0x60101414 }, 
        { 32, 0x14000248, 0x60101414 }, 
        { 32, 0x14000250, 0x60101414 }, 
        { 32, 0x14000258, 0x60101414 }, 
        { 32, 0x1400023c, 0x00101001 }, 
        { 32, 0x14000244, 0x00101001 }, 
        { 32, 0x1400024c, 0x00101001 }, 
        { 32, 0x14000254, 0x00101001 }, 
        { 32, 0x1400025c, 0x00102201 }, 
        { 32, 0x14000000, 0x00000101 }
    };

int main(int argc, const char * argv[]) {
    imx50_device_t *handle;
    unsigned char data[500];
    unsigned const char *payload = "abcdefghijklmnopqrstuvwxyz";
    unsigned int value = 0xdeadbeef;

    imx50_log_level(DEBUG_LOG);
    handle = imx50_init_device();

    if(imx50_write_register(handle, 0x53FD400C, 0x4) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F80004, 0x0) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F80008, 0x80) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F8001C, 0x80) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F80010, 0xB4) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F80024, 0xB4) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F8000C, 0xB3) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F80020, 0xB3) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F80000, 0x00001236) != 0){
        return 1;
    }

    Sleep(2000);

    if(imx50_write_register(handle, 0x63F80010, 0x3C) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F80024, 0x3C) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x63F80004, 0x1) != 0){
        return 1;
    }

    Sleep(2000);

    if(imx50_write_register(handle, 0x53FD400C, 0x0) != 0){
        return 1;
    }

    if(imx50_write_register(handle, 0x53FD4068, 0xffffffff) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x53FD406c, 0xffffffff) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x53FD4070, 0xffffffff) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x53FD4074, 0xffffffff) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x53FD4078, 0xffffffff) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x53FD407c, 0xffffffff) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x53FD4080, 0xffffffff) != 0){
        return 1;
    }
    if(imx50_write_register(handle, 0x53FD4084, 0xffffffff) != 0){
        return 1;
    }

    if(imx50_write_register(handle, 0x53FD4098, 0x80000004) != 0){
        return 1;
    }

    Sleep(2000);

    if(imx50_dcd_write(handle, lpddr1_init, sizeof(lpddr1_init) / sizeof(dcd_t)) != 0){
        fprintf(stderr, "!!! failed\n");
    }

    Sleep(2000);

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

int main2(int argc, const char * argv[]) {
    imx50_device_t *handle;
    unsigned char data[500];
    unsigned const char *payload = "abcdefghijklmnopqrstuvwxyz";

    imx50_log_level(DEBUG_LOG);

    fprintf(stderr, "%s\n", "Waiting for device...");
    handle = imx50_init_device();
    fprintf(stderr, "%s\n", "Found device.");
    fprintf(stderr, "!!! device status: %X\n", imx50_error_status(handle));

    fprintf(stderr, "%s\n", "!!! Set up memory.");
    //imx50_init_memory(handle);
    if(imx50_dcd_write(handle, lpddr1_init, sizeof(lpddr1_init) / sizeof(dcd_t)) != 0){
        fprintf(stderr, "!!! failed\n");
    }
    fprintf(stderr, "!!! device status: %X\n", imx50_error_status(handle));

    Sleep(5000);
    
    //fprintf(stderr, "%s\n", "Attempting to read 500 bytes from 0x0.");
    //imx50_read_memory(handle, 0x0, data, 500);

    fprintf(stderr, "%s\n", "!!! Writing memory");
    imx50_write_memory(handle, 0xF8006000, payload, strlen(payload));
    fprintf(stderr, "!!! device status: %X\n", imx50_error_status(handle));

    fprintf(stderr, "%s\n", "!!! Reading memory");
    imx50_read_memory(handle, 0xF8006000, data, strlen(payload));
    fprintf(stderr, "!!! device status: %X\n", imx50_error_status(handle));
    
    fprintf(stderr, "%s\n", "Closing device.");
    imx50_close_device(handle);
    return 0;
}

