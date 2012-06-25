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

#include "hidapi.h"
#include "imxusb.h"
#include <stdlib.h>

#ifndef _WIN32
// posix includes
#include <stdint.h>
#include <string.h>
#include <unistd.h>
// sleep
#define SLEEP(x) sleep(x)
#else
// fixed width integers
// unfortunally, VC++ lacks unsigned types
typedef __int8 uint8_t;
typedef __int16 uint16_t;
typedef __int32 uint32_t;
typedef __int64 uint64_t;
// WinRT includes
#include <windows.h>
#include <memory.h
// sleep
#define SLEEP(x) Sleep(x)
#endif

/**
    @brief Get a iMX50 usb download device
    
    This function will block until a device is found.
    Remember to free the device with imx50_free_device()
 
    @return A device will be returned on success, NULL on error
*/
imx50_device_t *imx50_init_device() {
    hid_device *handle = NULL;
    struct hid_device_info *dev;
    
    while(handle == NULL) {
        dev = hid_enumerate(IMX50_VID, IMX50_PID);
        if(dev == NULL) {
            SLEEP(5);
            continue; // loop
        }
        handle = hid_open(dev->vendor_id, dev->product_id, dev->serial_number);
        hid_free_enumeration(dev);
    }
    
    return (imx50_device_t*)handle;
}

/**
    @brief Frees a iMX50 usb download device
 
    @param device The device to free.
 */
void imx50_close_device(imx50_device_t *device) {
    hid_close((hid_device*)device);
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
int imx50_send_command(imx50_device_t *device, sdp_t *command) {
    // pack the command
    unsigned char *data = imx50_pack_command(command);
    if(!data) {
        return ERROR_OUT_OF_MEMORY; // error packing
    }
    
    // send the report
    if(hid_write((hid_device*)device, data, REPORT_ID_SDP_CMD) < 0) {
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
int imx50_send_data(imx50_device_t *device, unsigned char *payload, unsigned int size) {
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
    if(hid_write((hid_device*)device, data, size+1) < 0) {
        free(data);
        return ERROR_WRITE; // error sending
    }
    free(data);
    return 0;
}

/**
    @brief Reads HAB state. (Report 3)
 
    This is the third report. The device will return either 
    HAB_PRODUCTION_MODE (HAB enabled) or 
    HAB_ENGINEER_MODE (HAB disabled).
 
    @param device The HID device to read from.
    
    @return HAB status on success, error code otherwise.
**/
int imx50_get_hab_type(imx50_device_t *device) {
    unsigned char *data = malloc(REPORT_HAB_MODE_SIZE);
    int hab_type;
    
    if(!data) {
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    
    if(!memset(data, 0, REPORT_HAB_MODE_SIZE)) {
        free(data);
        return ERROR_IO; // cannot write to memory
    }
    
    if(hid_read((hid_device*)device, data, REPORT_HAB_MODE_SIZE) < 0) {
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
int imx50_get_dev_ack(imx50_device_t *device, unsigned char **payload_p, unsigned int *size_p) {
    unsigned char *data = malloc(REPORT_STATUS_SIZE);
    unsigned char *payload;
    if(!data) {
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    if(!memset(data, 0, REPORT_STATUS_SIZE)) {
        free(data);
        return ERROR_IO; // cannot write to memory
    }
    if(hid_read((hid_device*)device, data, REPORT_STATUS_SIZE) < 0) {
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

/**
    @brief Reads from the device's memory
    
    @param device the HID device to read from.
    @param address Where to start reading
    @param buffer Buffer to read to
    @param count How much to read (in bytes)
    
    @return Zero on success, error code otherwise
**/
int imx50_read_memory(imx50_device_t *device, unsigned int address, unsigned char *buffer, unsigned int count) {
    sdp_t sdpCmd;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_READ_REGISTER;
    sdpCmd.address = address;
    sdpCmd.format = BITSOF(int); // 32-bits = one byte
    sdpCmd.data_count = count;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        return ERROR_COMMAND;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        return ERROR_RETURN;
    }
    
    unsigned int max_trans_size = REPORT_STATUS_SIZE - 1;
    unsigned int trans_size;
    unsigned char *data;
    
    while(count > 0) {
        trans_size = (count > max_trans_size) ? max_trans_size : count;
        
        memset(data, 0, REPORT_STATUS_SIZE);
        
        if(imx50_get_dev_ack(device, &data, &max_trans_size) < 0) { // report 4 contains return value
            return ERROR_READ;
        }
        
        memcpy(buffer, data, trans_size);
        free(data); // malloc'd in imx50_get_dev_ack()
        buffer += trans_size;
        count -= trans_size;
    }
    
    return 0;
}

/**
    @brief Writes to a single register in memory
    
    @param device the HID device to write to.
    @param address Where to write in memory
    @param data A byte of data to write
    
    @return Zero on success, error code otherwise
**/
int imx50_write_register(imx50_device_t *device, unsigned int address, unsigned int data) {
    sdp_t sdpCmd;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_WRITE_REGISTER;
    sdpCmd.address = address;
    sdpCmd.format = BITSOF(int); // 32-bits = one byte
    sdpCmd.data_count = 1;
    sdpCmd.data = data;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        return ERROR_COMMAND;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        return ERROR_RETURN;
    }
    
    unsigned int *status_p;
    unsigned int status;
    unsigned int size;
    if(imx50_get_dev_ack(device, &status_p, &size) < 0) {
        return ERROR_READ;
    }
    status = BSWAP32(status_p[0]);
    // we assume status is big-endian, but that's not required
    // return values are same in both endian
    // for ex: 0x128A8A12 is same backwards and forwards
    free(status_p);
    
    if(status != ACK_WRITE_COMPLETE) {
        return ERROR_WRITE;
    }
    
    return 0;
}

/**
    @brief Writes to the device's memory
    
    @param device the HID device to write to.
    @param address Where to start writing
    @param buffer Buffer to write from
    @param count How much to write (in bytes)
    
    @return Zero on success, error code otherwise
**/
int imx50_write_memory(imx50_device_t *device, unsigned int address, unsigned char *buffer, unsigned int count) {
    sdp_t sdpCmd;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_WRITE_FILE;
    sdpCmd.address = address;
    sdpCmd.data_count = count;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        return ERROR_COMMAND;
    }
    
    // TODO: Find out if this is required
    SLEEP(10); // this was in the reference implementation
    
    unsigned int max_trans_size = REPORT_DATA_SIZE - 1;
    unsigned int trans_size;
    
    while(count > 0) {
        trans_size = (count > max_trans_size) ? max_trans_size : count;
        
        if(imx50_send_data(device, buffer, trans_size) < 0) { // report 2 contains data
            return ERROR_WRITE;
        }
        
        buffer += trans_size;
        count -= trans_size;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        return ERROR_RETURN;
    }
    
    unsigned int *status_p;
    unsigned int status;
    unsigned int size;
    if(imx50_get_dev_ack(device, &status_p, &size) < 0) {
        return ERROR_READ;
    }
    status = BSWAP32(status_p[0]);
    free(status_p);
    
    if(status != ACK_WRITE_COMPLETE) {
        return ERROR_WRITE;
    }
    
    return 0;
}

/**
    @brief Get's error code from device
    
    @param device the HID device
    
    @return Error status from device (can be zero)
**/
int imx50_error_status(imx50_device_t *device) {
    sdp_t sdpCmd;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = CMD_ERROR_STATUS;
    sdpCmd.command_type = CMD_WRITE_FILE;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        return ERROR_COMMAND;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        return ERROR_RETURN;
    }
    
    unsigned int *status_p;
    unsigned int status;
    unsigned int size;
    if(imx50_get_dev_ack(device, &status_p, &size) < 0) {
        return ERROR_READ;
    }
    status = BSWAP32(status_p[0]); // assmue status is in big-endian
    free(status_p);
    
    return status;
}

/**
    @brief Writes to multiple registers in memory
    
    @param device the HID device to write to
    @param buffer An array of DCD members
    @param count Number of DCD members
    
    @see struct dcd
    @return Zero on success, error code otherwise
**/
int imx50_dcd_write(imx50_device_t *device, dcd_t *buffer, unsigned int count) {
    sdp_t sdpCmd;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_DCD_WRITE;
    
    unsigned int i;
    unsigned int size;
    unsigned char *payload;
    
    while(count > 0) {
        sdpCmd.data_count = (count > MAX_DCD_WRITE_REG_CNT) ? MAX_DCD_WRITE_REG_CNT : count;
        size = sdpCmd.data_count * sizeof(dcd_t);
        
        if(imx50_send_command(device, &sdpCmd) != 0) {
            return ERROR_COMMAND;
        }
        
        // pack and convert dcd to big endian
        payload = malloc(size);
        for(i = 0; i < sdpCmd.data_count; i++) {
            // this looks complicated but all it does is loop through a 2D array of type [dcd_t][int]
            // and sets the value of each element to the byte-swapped version of the DCD member
            // the weird casting is to make sure that everything's packed with one-byte alignment
            // which should never be a problem, but you'd never know...
            *(uint32_t*)&payload[i*sizeof(dcd_t) + 0*sizeof(int)] = BSWAP32(buffer[i].data_format);
            *(uint32_t*)&payload[i*sizeof(dcd_t) + 1*sizeof(int)] = BSWAP32(buffer[i].address);
            *(uint32_t*)&payload[i*sizeof(dcd_t) + 2*sizeof(int)] = BSWAP32(buffer[i].value);
        }
        
        if(imx50_send_data(device, payload, size) < 0) {
            free(payload);
            return ERROR_WRITE;
        }
        free(payload);
    
        if(imx50_get_hab_type(device) < 0) {
            return ERROR_RETURN;
        }
        
        unsigned int *status_p;
        unsigned int status;
        unsigned int size;
        if(imx50_get_dev_ack(device, &status_p, &size) < 0) {
            return ERROR_READ;
        }
        status = BSWAP32(status_p[0]);
        free(status_p);
        
        if(status != ACK_WRITE_COMPLETE) {
            return ERROR_WRITE;
        }
        
        buffer += size;
        count -= sdpCmd.data_count;
    }
    
    return 0;
}

/**
    @brief Execute commands and exit
    
    Once this is called, the device will be unloaded and 
    will start executing instructions starting at the address.
    You must still free the device with imx50_free_device().
    
    @param device the HID device to write to
    @param address The address to jump to
    
    @return Zero on success, error code, or status code from device
**/
int imx50_jump(imx50_device_t *device, unsigned int address) {
    sdp_t sdpCmd;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = CMD_ERROR_STATUS;
    sdpCmd.command_type = CMD_JUMP_ADDRESS;
    sdpCmd.address = address;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        return ERROR_COMMAND;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        return ERROR_RETURN;
    }
    
    unsigned int *status_p;
    unsigned int status;
    unsigned int size;
    if(imx50_get_dev_ack(device, &status_p, &size) < 0) {
        return ERROR_READ;
    }
    status = BSWAP32(status_p[0]); // assmue status is in big-endian
    free(status_p);
    if(status > 0) {
        return status; // error occured
    }
    
    return 0;
}

/**
    @brief Loads an arbitrary file unto the device. 
    
    This function splits the input file into chunks of 
    MAX_DOWNLOAD_SIZE and sends it using imx50_write_memory().
    
    @param device the HID device to write to
    @param address The address to write to on the device
    @param filename The name of the file to load
    
    @see imx50_write_memory
    @return Zero on success, error code otherwise
**/
int imx50_load_file(imx50_device_t *device, unsigned int address, const char *filename) {
    unsigned int size;
    unsigned int offset = 0;
    unsigned int trans_size = 0;
    unsigned char buffer[MAX_DOWNLOAD_SIZE];
#ifdef _WIN32 //win32 code
    HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD dwBytesRead = 0;
    if(hFile == INVALID_HANDLE_VALUE) {
        return ERROR_IO;
    }
#else // posix
    FILE *fp = fopen(filename, "r");
    if(!fp) {
        return ERROR_IO;
    }
    
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp); // get file size
    fseek(fp, 0L, SEEK_SET); // reset fp
#endif
    
    for(; offset < size; offset += trans_size) {
        trans_size = size - offset;
        if(trans_size > MAX_DOWNLOAD_SIZE)£û
            trans_size = MAX_DOWNLOAD_SIZE;
        }
        
#ifdef _WIN32
        if(ReadFile(hFile, buffer, trans_size, &dwBytesRead, NULL) == FALSE) {
            CloseHandle(hFile);
            return ERROR_IO;
        }
        if(dwBytesRead < trans_size) {
            CloseHandle(hFile);
            return ERROR_IO;
        }
#else
        if(fread(buffer, sizeof(char), trans_size, fp) < trans_size) {
            fclose(fp);
            return ERROR_IO;
        }
#endif
        
        if(imx50_write_memory(device, address + offset, buffer, trans_size) != 0) {
            break;
        }
    }
    
    // close file
#ifdef _WIN32
    CloseHandle(hFile);
#else
    fclose(fp);
#endif
    
    return offset >= size ? 0 : ERROR_WRITE; // did the loop complete?
}

/**
    @brief Sets up the device's external memory
    
    iMX50 devices, on startup, must be set up before 
    the entire DDR RAM can be accessed. Before set up, 
    you can only read/write the iMX50 internal memory.
    The device is set up using imx50_dcd_write() with 
    pre-coded DCD members. Experts can set up the RAM 
    manually using that function.
    
    @param device the HID device to set up
    
    @see imx50_dcd_write
    @return imx50_dcd_write()'s return value
**/
int imx50_init_memory(imx50_device_t *device) {
    static dcd_t mx508_lpddr2_init = 
    {
        { 32, 0x53fd400c, 0x00000004 }, { 32, 0x63f80000, 0x00001232 }, { 32, 0x63f80004, 0x00000002 }, { 32, 0x63f80008, 0x00000080 }, { 32, 0x63f8001c, 0x00000080 }, 
        { 32, 0x63f8000c, 0x00000002 }, { 32, 0x63f80020, 0x00000002 }, { 32, 0x63f80010, 0x00000001 }, { 32, 0x63f80024, 0x00000001 }, { 32, 0x63f80000, 0x00001232 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, 
        { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fa86ac, 0x02000000 }, { 32, 0x53fd400c, 0x00000000 }, { 32, 0x53fd4068, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd4070, 0xffffffff }, { 32, 0x53fd4074, 0xffffffff }, { 32, 0x53fd4078, 0xffffffff }, { 32, 0x53fd407c, 0xffffffff }, { 32, 0x53fd4080, 0xffffffff }, 
        { 32, 0x53fd4084, 0xffffffff }, { 32, 0x53FD4098, 0x80000003 }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fd406c, 0xffffffff }, 
        { 32, 0x53fd406c, 0xffffffff }, { 32, 0x53fa86ac, 0x04000000 }, { 32, 0x53fa86a4, 0x00180000 }, { 32, 0x53fa8668, 0x00180000 }, { 32, 0x53fa8698, 0x00180000 }, 
        { 32, 0x53fa86a0, 0x00180000 }, { 32, 0x53fa86a8, 0x00180000 }, { 32, 0x53fa86b4, 0x00180000 }, { 32, 0x53fa8498, 0x00180000 }, { 32, 0x53fa849c, 0x00180000 }, 
        { 32, 0x53fa84f0, 0x00180000 }, { 32, 0x53fa8500, 0x00180000 }, { 32, 0x53fa84c8, 0x00180000 }, { 32, 0x53fa8528, 0x00180000 }, { 32, 0x53fa84f4, 0x00180000 }, 
        { 32, 0x53fa84fc, 0x00180000 }, { 32, 0x53fa84cc, 0x00180000 }, { 32, 0x53fa8524, 0x00180000 }, { 32, 0x14000128, 0x08170101 }, { 32, 0x14000128, 0x00000000 }, 
        { 32, 0x14000128, 0x08170111 }, { 32, 0x14000128, 0x00000000 }, { 32, 0x14000000, 0x00000500 }, { 32, 0x14000004, 0x00000000 }, { 32, 0x14000008, 0x0000001b }, 
        { 32, 0x1400000c, 0x0000d056 }, { 32, 0x14000010, 0x0000010b }, { 32, 0x14000014, 0x00000a6b }, { 32, 0x14000018, 0x02030d0c }, { 32, 0x1400001c, 0x0c110304 }, 
        { 32, 0x14000020, 0x05020503 }, { 32, 0x14000024, 0x0048eb05 }, { 32, 0x14000028, 0x01000403 }, { 32, 0x1400002c, 0x09040501 }, { 32, 0x14000030, 0x02000000 }, 
        { 32, 0x14000034, 0x00000e02 }, { 32, 0x14000038, 0x00000006 }, { 32, 0x1400003c, 0x00002301 }, { 32, 0x14000040, 0x00050300 }, { 32, 0x14000044, 0x00000300 }, 
        { 32, 0x14000048, 0x00260026 }, { 32, 0x1400004c, 0x00010000 }, { 32, 0x1400005c, 0x02000000 }, { 32, 0x14000060, 0x00000002 }, { 32, 0x14000064, 0x00000000 }, 
        { 32, 0x14000068, 0x00000000 }, { 32, 0x1400006c, 0x00040042 }, { 32, 0x14000070, 0x00000001 }, { 32, 0x14000074, 0x00000000 }, { 32, 0x14000078, 0x00040042 }, 
        { 32, 0x1400007c, 0x00000001 }, { 32, 0x14000080, 0x010b0000 }, { 32, 0x14000084, 0x00000060 }, { 32, 0x14000088, 0x02400018 }, { 32, 0x1400008c, 0x01000e00 }, 
        { 32, 0x14000090, 0x0a010101 }, { 32, 0x14000094, 0x01011f1f }, { 32, 0x14000098, 0x01010101 }, { 32, 0x1400009c, 0x00030101 }, { 32, 0x140000a0, 0x00010000 }, 
        { 32, 0x140000a4, 0x00010000 }, { 32, 0x140000a8, 0x00000000 }, { 32, 0x140000ac, 0x0000ffff }, { 32, 0x140000c8, 0x02020101 }, { 32, 0x140000cc, 0x01000000 }, 
        { 32, 0x140000d0, 0x01000201 }, { 32, 0x140000d4, 0x00000200 }, { 32, 0x140000d8, 0x00000102 }, { 32, 0x140000dc, 0x0003ffff }, { 32, 0x140000e0, 0x0000ffff }, 
        { 32, 0x140000e4, 0x02020000 }, { 32, 0x140000e8, 0x02020202 }, { 32, 0x140000ec, 0x00000202 }, { 32, 0x140000f0, 0x01010064 }, { 32, 0x140000f4, 0x01010101 }, 
        { 32, 0x140000f8, 0x00010101 }, { 32, 0x140000fc, 0x00000064 }, { 32, 0x14000100, 0x00000000 }, { 32, 0x14000104, 0x02000802 }, { 32, 0x14000108, 0x04080000 }, 
        { 32, 0x1400010c, 0x04080408 }, { 32, 0x14000110, 0x04080408 }, { 32, 0x14000114, 0x03060408 }, { 32, 0x14000118, 0x01010002 }, { 32, 0x1400011c, 0x00000000 }, 
        { 32, 0x14000200, 0x00000000 }, { 32, 0x14000204, 0x00000000 }, { 32, 0x14000208, 0xf5003a27 }, { 32, 0x14000210, 0xf5003a27 }, { 32, 0x14000218, 0xf5003a27 }, 
        { 32, 0x14000220, 0xf5003a27 }, { 32, 0x14000228, 0xf5003a27 }, { 32, 0x1400020c, 0x074002e1 }, { 32, 0x14000214, 0x074002e1 }, { 32, 0x1400021c, 0x074002e1 }, 
        { 32, 0x14000224, 0x074002e1 }, { 32, 0x1400022c, 0x074002e1 }, { 32, 0x14000230, 0x00000000 }, { 32, 0x14000234, 0x00810006 }, { 32, 0x14000238, 0x60099414 }, 
        { 32, 0x14000240, 0x60099414 }, { 32, 0x14000248, 0x60099414 }, { 32, 0x14000250, 0x60099414 }, { 32, 0x14000258, 0x60099414 }, { 32, 0x1400023c, 0x000a1401 }, 
        { 32, 0x14000244, 0x000a1401 }, { 32, 0x1400024c, 0x000a1401 }, { 32, 0x14000254, 0x000a1401 }, { 32, 0x1400025c, 0x000a1401 }, { 32, 0x14000000, 0x00000501 }
    }
    return imx50_dcd_write(device, &mx508_lpddr2_init, sizeof(mx508_lpddr2_init) / sizeof(dcd_t));
}
