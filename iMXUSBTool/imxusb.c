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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
            sleep(5);
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
    sleep(10); // this was in the reference implementation
    
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
            *(unsigned int*)&payload[i*sizeof(dcd_t) + 0*sizeof(int)] = BSWAP32(buffer[i].data_format);
            *(unsigned int*)&payload[i*sizeof(dcd_t) + 1*sizeof(int)] = BSWAP32(buffer[i].address);
            *(unsigned int*)&payload[i*sizeof(dcd_t) + 2*sizeof(int)] = BSWAP32(buffer[i].value);
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
