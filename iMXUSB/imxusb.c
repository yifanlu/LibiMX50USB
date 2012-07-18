//
//  iMX50 USB Library
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
#include <stdio.h>

#ifndef _WIN32

// posix includes
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// function macros
#define SLEEP(x) usleep(x * 1000)
#define TRACE(msg...) \
    (fprintf(stderr, msg))

#else // windows

// fixed width integers
// unfortunally, VC++ lacks unsigned types
typedef __int8 uint8_t;
typedef __int16 uint16_t;
typedef __int32 uint32_t;
typedef __int64 uint64_t;
// WinRT includes
#include <windows.h>
#include <memory.h>
// function macros
#define SLEEP(x) Sleep(x)
#define TRACE printf

#endif

int g_imx50_log_mask = ERROR_LOG;

/**
    @brief Get a iMX50 usb download device
    
    This function will block until a device is found.
    Remember to free the device with imx50_free_device()
 
    @return A device will be returned on success, NULL on error
*/
IMX50USB_EXPORT imx50_device_t *imx50_init_device() {
    hid_device *handle = NULL;
    struct hid_device_info *dev;
    
    if(IS_LOGGING(DEBUG_LOG)) TRACE("[%s] D:Enumerating devices [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
    while(handle == NULL) {
        dev = hid_enumerate(IMX50_VID, IMX50_PID);
        if(dev == NULL) {
            SLEEP(100);
            continue; // loop
        }
        if(IS_LOGGING(DEBUG_LOG)) TRACE("[%s] D:Opening device VID:%04hX PID:%04hx path: %s [%s:%d]\n", 
            __FUNCTION__, dev->vendor_id, dev->product_id, dev->path, __FILE__, __LINE__);
        handle = hid_open(dev->vendor_id, dev->product_id, dev->serial_number);
        hid_free_enumeration(dev);
    }
    
    return (imx50_device_t*)handle;
}

/**
    @brief Frees a iMX50 usb download device
 
    @param device The device to free.
 */
IMX50USB_EXPORT void imx50_close_device(imx50_device_t *device) {
    if(IS_LOGGING(DEBUG_LOG)) TRACE("[%s] D:Closing device %p [%s:%d]\n", __FUNCTION__, device, __FILE__, __LINE__);
    hid_close((hid_device*)device);
}

/**
    @brief Sets the logging level
 
    @param log_mask Logging level.
 */
IMX50USB_EXPORT void imx50_log_level(int log_mask) {
    g_imx50_log_mask = log_mask;
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
    uint32_t *payload = (uint32_t*)(data+1); // first byte is number
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
    @brief Prints out a HEX dump of data.

    For debugging commands.
 
    @param data Data to dump
    @param size Number of bytes to print
    @param num Number of bytes to print per line
 */
void imx50_hex_dump(unsigned char *data, unsigned int size, unsigned int num) {
    unsigned int i = 0, j = 0, k = 0, l = 0;
    // I hate letters, but I can't come up with good names
    // i = counter, j = bytes printed, k = number of place values, l = temp
    for(l = size/num, k = 1; l > 0; l/=num, k++); // find number of zeros to prepend line number
    while(j < size) {
        // line number
        fprintf(stderr, "%0*X: ", k, j);
        // hex value
        for(i = 0; i < num; i++, j++) {
            if(j < size) {
                fprintf(stderr, "%02X ", data[j]);
            } else { // print blank spaces
                fprintf(stderr, "%s ", "  ");
            }
        }
        // seperator
        fprintf(stderr, "%s", "| ");
        // ascii value
        for(i = num; i > 0; i--) {
            if(j-i < size) {
                fprintf(stderr, "%c", data[j-i] < 32 || data[j-i] > 126 ? '.' : data[j-i]); // print only visible characters
            } else {
                fprintf(stderr, "%s", " ");
            }
        }
        // new line
        fprintf(stderr, "%s", "\n");
    }
}

/**
    @brief Sends the command to the device. (Report 1)
 
    This is the first report, it will pack and then send the 
    requested command.
 
    @param device The HID device to send to.
    @param command The command to send
 
    @return Zero on success, error code otherwise.
**/
IMX50USB_EXPORT int imx50_send_command(imx50_device_t *device, sdp_t *command) {
    // pack the command
    unsigned char *data = imx50_pack_command(command);
    if(!data) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Out of memory [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_OUT_OF_MEMORY; // error packing
    }
    
    // send the report
    if(IS_LOGGING(INFO_LOG)) TRACE("[%s] I:Sending command (report 1) %#04Xh [%s:%d]\n", __FUNCTION__, command->command_type, __FILE__, __LINE__);
    if(IS_LOGGING(DEBUG_LOG)) imx50_hex_dump(data, REPORT_SDP_CMD_SIZE, 0x10);
    if(hid_write((hid_device*)device, data, REPORT_SDP_CMD_SIZE) < 0) {
        free(data);
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error sending data [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_WRITE; // error sending
    }
    if(IS_LOGGING(INFO_LOG)) TRACE("[%s] I:Command sent successfully [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
    
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
IMX50USB_EXPORT int imx50_send_data(imx50_device_t *device, unsigned char *payload, unsigned int size) {
    unsigned char *data;

    if(size+1 > REPORT_DATA_SIZE) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Size of data (%u) is too large. (max:%u) [%s:%d]\n", __FUNCTION__, size, REPORT_DATA_SIZE, __FILE__, __LINE__);
        return ERROR_PARAMETER;
    }
    data = malloc(size + 1);
    if(!data) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Out of memory [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    data[0] = REPORT_ID_DATA;
    if(!memcpy(data+1, payload, size)) {
        free(data);
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot copy data [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_IO; // cannot copy data
    }
    if(IS_LOGGING(INFO_LOG)) TRACE("[%s] I:Sending data (report 2) [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
    if(IS_LOGGING(DEBUG_LOG)) imx50_hex_dump(data, size+1, 0x10);
    if(hid_write((hid_device*)device, data, size+1) < 0) {
        free(data);
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error sending data [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_WRITE; // error sending
    }
    if(IS_LOGGING(INFO_LOG)) TRACE("[%s] I:Data sent successfully [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);

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
IMX50USB_EXPORT int imx50_get_hab_type(imx50_device_t *device) {
    unsigned char *data = malloc(REPORT_HAB_MODE_SIZE);
    int hab_type;
    
    if(!data) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Out of memory [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    
    if(!memset(data, 0, REPORT_HAB_MODE_SIZE)) {
        free(data);
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Access to memory denied [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_IO; // cannot write to memory
    }
    
    if(IS_LOGGING(INFO_LOG)) TRACE("[%s] I:Reading HAB state (report 3) [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
    if(hid_read((hid_device*)device, data, REPORT_HAB_MODE_SIZE) < 0) {
        free(data);
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error reading response [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_READ;
    }
    if(IS_LOGGING(DEBUG_LOG)) imx50_hex_dump(data, REPORT_HAB_MODE_SIZE, 0x10);
    if(IS_LOGGING(INFO_LOG)) TRACE("[%s] I:HAB state read successfully [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
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
IMX50USB_EXPORT int imx50_get_dev_ack(imx50_device_t *device, unsigned char **payload_p, unsigned int *size_p) {
    unsigned char *data = malloc(REPORT_STATUS_SIZE);
    unsigned char *payload;
    if(!data) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Out of memory [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    if(!memset(data, 0, REPORT_STATUS_SIZE)) {
        free(data);
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Access to memory denied [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_IO; // cannot write to memory
    }

    if(IS_LOGGING(INFO_LOG)) TRACE("[%s] I:Recieving response (report 4) [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
    if(hid_read((hid_device*)device, data, REPORT_STATUS_SIZE) < 0) {
        free(data);
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving response [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_READ;
    }
    if(IS_LOGGING(DEBUG_LOG)) imx50_hex_dump(data, REPORT_STATUS_SIZE, 0x10);

    payload = malloc(REPORT_STATUS_SIZE - 1);
    if(!payload) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Out of memory [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_OUT_OF_MEMORY; // cannot alloc memory
    }
    if(!memcpy(payload, data+1, REPORT_STATUS_SIZE-1)) {
        free(payload);
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Access to memory denied [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_IO; // cannot write to memory
    }
    // set return values
    *payload_p = payload;
    *size_p = REPORT_STATUS_SIZE-1;

    if(IS_LOGGING(INFO_LOG)) TRACE("[%s] I:Response recieved successfully [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
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
IMX50USB_EXPORT int imx50_read_memory(imx50_device_t *device, device_addr_t address, unsigned char *buffer, unsigned int count) {
    sdp_t sdpCmd;
    unsigned int max_trans_size = REPORT_STATUS_SIZE - 1;
    unsigned int trans_size;
    unsigned char *data = NULL;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_READ_REGISTER;
    sdpCmd.address = address;
    sdpCmd.format = BITSOF(int); // 32-bits = one byte
    sdpCmd.data_count = count;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot send command [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_COMMAND;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving status [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_RETURN;
    }
    
    while(count > 0) {
        trans_size = (count > max_trans_size) ? max_trans_size : count;
        
        if(imx50_get_dev_ack(device, &data, &max_trans_size) < 0) { // report 4 contains return value
            if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving data [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
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
    @param data The data to write
    @param format How many bits in data to write 
    
    @return Zero on success, error code otherwise
**/
IMX50USB_EXPORT int imx50_write_register(imx50_device_t *device, device_addr_t address, unsigned int data, unsigned char format) {
    sdp_t sdpCmd;
    unsigned int *status_p;
    unsigned int status;
    unsigned int size;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_WRITE_REGISTER;
    sdpCmd.address = address;
    sdpCmd.format = format; // 32-bits = one byte
    sdpCmd.data_count = 1;
    sdpCmd.data = data;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot send command [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_COMMAND;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving status [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_RETURN;
    }

    if(imx50_get_dev_ack(device, (unsigned char**)&status_p, &size) < 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving response [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_READ;
    }
    status = BSWAP32(status_p[0]);
    // we assume status is big-endian, but that's not required
    // return values are same in both endian
    // for ex: 0x128A8A12 is same backwards and forwards
    free(status_p);
    
    if(status != ACK_WRITE_COMPLETE) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Reponse expected: %#08X, got: %#08X [%s:%d]\n", __FUNCTION__, ACK_WRITE_COMPLETE, status, __FILE__, __LINE__);
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
IMX50USB_EXPORT int imx50_write_memory(imx50_device_t *device, device_addr_t address, unsigned char *buffer, unsigned int count) {
    sdp_t sdpCmd;
    unsigned int max_trans_size = REPORT_DATA_SIZE - 1;
    unsigned int trans_size;
    unsigned int *status_p;
    unsigned int status;
    unsigned int size;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_WRITE_FILE;
    sdpCmd.address = address;
    sdpCmd.data_count = count;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot send command [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_COMMAND;
    }
    
    // TODO: Find out if this is required
    SLEEP(10); // this was in the reference implementation
    
    while(count > 0) {
        trans_size = (count > max_trans_size) ? max_trans_size : count;
        
        if(imx50_send_data(device, buffer, trans_size) < 0) { // report 2 contains data
            return ERROR_WRITE;
        }
        
        buffer += trans_size;
        count -= trans_size;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving status [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_RETURN;
    }
    
    if(imx50_get_dev_ack(device, (unsigned char**)&status_p, &size) < 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving response [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_READ;
    }
    status = BSWAP32(status_p[0]);
    free(status_p);
    
    if(status != ACK_FILE_COMPLETE) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Reponse expected: %#08X, got: %#08X [%s:%d]\n", __FUNCTION__, ACK_FILE_COMPLETE, status, __FILE__, __LINE__);
        return ERROR_WRITE;
    }
    
    return 0;
}

/**
    @brief Get's error code from device
    
    @param device the HID device
    
    @return Error status from device (can be zero)
**/
IMX50USB_EXPORT int imx50_error_status(imx50_device_t *device) {
    sdp_t sdpCmd;
    unsigned int *status_p;
    unsigned int status;
    unsigned int size;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_ERROR_STATUS;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot send command [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_COMMAND;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving status [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_RETURN;
    }
    
    if(imx50_get_dev_ack(device, (unsigned char**)&status_p, &size) < 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving response [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
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
IMX50USB_EXPORT int imx50_dcd_write(imx50_device_t *device, dcd_t *buffer, unsigned int count) {
    sdp_t sdpCmd;
    unsigned int i;
    unsigned int size;
    unsigned char *payload;
    unsigned int *status_p;
    unsigned int status_size;
    unsigned int status;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_DCD_WRITE;
    
    while(count > 0) {
        sdpCmd.data_count = (count > MAX_DCD_WRITE_REG_CNT) ? MAX_DCD_WRITE_REG_CNT : count;
        size = sdpCmd.data_count * sizeof(dcd_t);
        
        if(imx50_send_command(device, &sdpCmd) != 0) {
            if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot send command [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
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
            if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot send data [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
            return ERROR_WRITE;
        }
        free(payload);
    
        if(imx50_get_hab_type(device) < 0) {
            if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving status [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
            return ERROR_RETURN;
        }
        
        if(imx50_get_dev_ack(device, (unsigned char**)&status_p, &status_size) < 0) {
            if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving response [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
            return ERROR_READ;
        }
        status = BSWAP32(status_p[0]);
        free(status_p);
        
        if(status != ACK_WRITE_COMPLETE) {
            if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Reponse expected: %#08X, got: %#08X [%s:%d]\n", __FUNCTION__, ACK_WRITE_COMPLETE, status, __FILE__, __LINE__);
            return ERROR_WRITE;
        }
        
        buffer = (dcd_t*)((unsigned char*)buffer + size); // add in bytes
        count -= sdpCmd.data_count;
    }
    
    return 0;
}

/**
    @brief Execute commands and exit
    
    Once this is called, the device will be unloaded and 
    will start executing instructions starting at the address.
    You must still free the device with imx50_free_device().
 
    You can only jump to the start of an IVT header. If your 
    code does not have an IVT header, call imx50_add_header() 
    on the address of your code and jump to the value returned.
    
    @param device the HID device to write to
    @param address The address of header to jump to
    
    @return Zero on success, error code, or status code from device
    @see imx50_add_header
**/
IMX50USB_EXPORT int imx50_jump(imx50_device_t *device, device_addr_t address) {
    sdp_t sdpCmd;
    //unsigned int *status_p;
    //unsigned int status;
    //unsigned int size;
    
    memset(&sdpCmd, 0, sizeof(sdp_t)); // resets the struct 
    sdpCmd.report_number = REPORT_ID_SDP_CMD;
    sdpCmd.command_type = CMD_JUMP_ADDRESS;
    sdpCmd.address = address;
    
    if(imx50_send_command(device, &sdpCmd) != 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot send command [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_COMMAND;
    }
    
    if(imx50_get_hab_type(device) < 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving status [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_RETURN;
    }
    
    /*
    if(imx50_get_dev_ack(device, (unsigned char**)&status_p, &size) < 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error recieving response [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_READ;
    }
    status = BSWAP32(status_p[0]); // assmue status is in big-endian
    free(status_p);
    if(status > 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Return status not zero, got: %#08X [%s:%d]\n", __FUNCTION__, status, __FILE__, __LINE__);
        return status; // error occured
    }
     */
    
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
IMX50USB_EXPORT int imx50_load_file(imx50_device_t *device, device_addr_t address, const char *filename) {
    unsigned int size;
    unsigned int offset;
    unsigned int trans_size;
    unsigned char buffer[MAX_DOWNLOAD_SIZE];
#ifdef _WIN32 //win32 code
    HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD dwBytesRead = 0;
    LARGE_INTEGER lsize;
    if(hFile == INVALID_HANDLE_VALUE) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot access %s [%s:%d]\n", __FUNCTION__, filename, __FILE__, __LINE__);
        return ERROR_IO;
    }
    if(GetFileSizeEx(hFile, &lsize) != 0) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot get file size %s [%s:%d]\n", __FUNCTION__, filename, __FILE__, __LINE__);
        return ERROR_IO;
    }
    size = (unsigned int)lsize.QuadPart;
#else // posix
    FILE *fp = fopen(filename, "r");
    if(!fp) {
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot access %s [%s:%d]\n", __FUNCTION__, filename, __FILE__, __LINE__);
        return ERROR_IO;
    }
    
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp); // get file size
    fseek(fp, 0L, SEEK_SET); // reset fp
#endif
    
    for(offset = 0, trans_size = 0; offset < size; offset += trans_size) {
        trans_size = size - offset;
        if(trans_size > MAX_DOWNLOAD_SIZE){
            trans_size = MAX_DOWNLOAD_SIZE;
        }
        
#ifdef _WIN32
        if(ReadFile(hFile, buffer, trans_size, &dwBytesRead, NULL) == FALSE) {
            CloseHandle(hFile);
            if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot read %s [%s:%d]\n", __FUNCTION__, filename, __FILE__, __LINE__);
            return ERROR_IO;
        }
        if(dwBytesRead < trans_size) {
            CloseHandle(hFile);
            if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:File read incomplete. Read: %u, expected: %u [%s:%d]\n", __FUNCTION__, dwBytesRead, trans_size, __FILE__, __LINE__);
            return ERROR_IO;
        }
#else
        if(fread(buffer, sizeof(char), trans_size, fp) < trans_size) {
            fclose(fp);
            return ERROR_IO;
        }
#endif
        
        if(imx50_write_memory(device, address + offset, buffer, trans_size) != 0) {
            if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error writing to device at %#X [%s:%d]\n", __FUNCTION__, address, __FILE__, __LINE__);
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
    @brief Adds header to code for executing
 
    Code that is to be executed must contain an IVT 
    header. This function takes the address of the 
    code to run, inserts an IVT header, and returns 
    the address of the header.
    Be aware that the 32 bytes before "address" will 
    be overwritten.
 
    @param device the HID device
    @param address location of executable code
 
    @return Zero on error, address of header otherwise
 **/
IMX50USB_EXPORT device_addr_t imx50_add_header(imx50_device_t *device, device_addr_t address) {
    device_addr_t flash_header_address;
    unsigned char flash_header[ROM_TRANSFER_SIZE] = { 0 };
    unsigned char temp_buffer[ROM_TRANSFER_SIZE] = { 0 };
    ivt_t *ivt_header = (ivt_t*)flash_header;
    
    // add the header before the code
    flash_header_address = address - sizeof(ivt_t);
    
    // read the data to add header to
    if(imx50_read_memory(device, flash_header_address, flash_header, ROM_TRANSFER_SIZE) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot read memory at %#X [%s:%d]\n", __FUNCTION__, flash_header_address, __FILE__, __LINE__);
        return 0;
    }
    
    // zero out the header
    memset(flash_header, 0, sizeof(ivt_t));
    
    // set the header parameters
    ivt_header->header = IVT_BARKER_HEADER;
    ivt_header->entry_address = address;
    ivt_header->self_address = flash_header_address;
    
    // send the data + new header
    if(imx50_write_memory(device, flash_header_address, flash_header, ROM_TRANSFER_SIZE) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot write header back at %#X [%s:%d]\n", __FUNCTION__, flash_header_address, __FILE__, __LINE__);
        return 0;
    }
    
    // check to see if everything is written correctly
    if(imx50_read_memory(device, flash_header_address, temp_buffer, ROM_TRANSFER_SIZE) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Cannot reader header back at %#X [%s:%d]\n", __FUNCTION__, flash_header_address, __FILE__, __LINE__);
        return 0;
    }
    
    // compare what we wrote to what is written
    if(memcmp(flash_header, temp_buffer, ROM_TRANSFER_SIZE) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Data written is corrupted [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return 0;
    }
    
    // if all works, return the address of the header
    return flash_header_address;
}

/**
    @brief Sets up an Amazon Kindle's DRAM
    
    iMX50 devices, on startup, must be set up before 
    the entire external RAM can be accessed. Before set 
    up, you can only read/write the iMX50 internal memory.
    The device is programmed with pre-coded register 
    values for Kindle Touch and Kindle 4.
    
    @param device the Kindle to set up
    
    @see imx50_dcd_write
    @return Zero on success, error code otherwise
**/
IMX50USB_EXPORT int imx50_kindle_init(imx50_device_t *device) {
    static dcd_t setup_pll1_1[] = 
        {
            { 32, 0x53FD400C, 0x4 }, // Switch ARM domain to be clocked from LP-APM
            { 32, 0x63F80004, 0x0 }, // disable auto-restart AREN bit
            { 32, 0x63F80008, 0x80 }, { 32, 0x63F8001C, 0x80 }, // clock PLL1
            { 32, 0x63F80010, 0xB4 }, { 32, 0x63F80024, 0xB4 }, // MFN = 180
            { 32, 0x63F8000C, 0xB3 }, { 32, 0x63F80020, 0xB3 }, // MFD = 179
            { 32, 0x63F80000, 0x00001236 } // Set PLM =1, manual restart and enable PLL
        };

    static dcd_t setup_pll1_2[] = 
        {
            { 32, 0x63F80010, 0x3C }, { 32, 0x63F80024, 0x3C }, // set PLL1 to 800Mhz
            { 32, 0x63F80004, 0x1 } // Set the LDREQ bit
        };

    static dcd_t enable_clocks[] = 
        {
            { 32, 0x53FD4068, 0xffffffff }, { 32, 0x53FD406c, 0xffffffff }, { 32, 0x53FD4070, 0xffffffff }, { 32, 0x53FD4074, 0xffffffff }, 
            { 32, 0x53FD4078, 0xffffffff }, { 32, 0x53FD407c, 0xffffffff }, { 32, 0x53FD4080, 0xffffffff }, { 32, 0x53FD4084, 0xffffffff }
        };
    static dcd_t lpddr1_init[] = 
        {
            // IOMUX
            { 32, 0x53fa86AC, 0x0 }, { 32, 0x53fa866C, 0x0 }, { 32, 0x53fa868C, 0x0 }, { 32, 0x53fa8670, 0x0 }, 
            { 32, 0x53fa86A4, 0x00180000 }, { 32, 0x53fa8668, 0x00180000 }, { 32, 0x53fa8698, 0x00180000 }, { 32, 0x53fa86A0, 0x00180000 }, 
            { 32, 0x53fa86A8, 0x00180000 }, { 32, 0x53fa86B4, 0x00180000 }, { 32, 0x53fa8490, 0x00180000 }, { 32, 0x53fa8494, 0x00180000 }, 
            { 32, 0x53fa8498, 0x00180000 }, { 32, 0x53fa849c, 0x00180000 }, { 32, 0x53fa84f0, 0x00180000 }, { 32, 0x53fa8500, 0x00180000 }, 
            { 32, 0x53fa84c8, 0x00180000 }, { 32, 0x53fa8528, 0x00180080 }, { 32, 0x53fa84f4, 0x00180080 }, { 32, 0x53fa84fc, 0x00180080 }, 
            { 32, 0x53fa84cc, 0x00180080 }, { 32, 0x53fa8524, 0x00180080 }, 
            // Static ZQ calibration
            { 32, 0x1400012C, 0x00000408 }, { 32, 0x14000128, 0x05090000 }, { 32, 0x14000124, 0x00310000 }, { 32, 0x14000124, 0x00200000 }, 
            { 32, 0x14000128, 0x05090010 }, { 32, 0x14000124, 0x00310000 }, { 32, 0x14000124, 0x00200000 }, 
            // DDR Controller registers
            { 32, 0x14000000, 0x00000100 }, { 32, 0x14000008, 0x00009c40 }, { 32, 0x1400000C, 0x00000000 }, { 32, 0x14000010, 0x00000000 }, 
            { 32, 0x14000014, 0x20000000 }, { 32, 0x14000018, 0x01010006 }, { 32, 0x1400001c, 0x080b0201 }, { 32, 0x14000020, 0x02000303 }, 
            { 32, 0x14000024, 0x0036b002 }, { 32, 0x14000028, 0x00000606 }, { 32, 0x1400002c, 0x06030400 }, { 32, 0x14000030, 0x01000000 }, 
            { 32, 0x14000034, 0x00000a02 }, { 32, 0x14000038, 0x00000003 }, { 32, 0x1400003c, 0x00001801 }, { 32, 0x14000040, 0x00050612 }, 
            { 32, 0x14000044, 0x00000200 }, { 32, 0x14000048, 0x001c001c }, { 32, 0x1400004c, 0x00010000 }, { 32, 0x1400005c, 0x01000000 }, 
            { 32, 0x14000060, 0x00000001 }, { 32, 0x14000064, 0x00000000 }, { 32, 0x14000068, 0x00320000 }, { 32, 0x1400006c, 0x00000000 }, 
            { 32, 0x14000070, 0x00000000 }, { 32, 0x14000074, 0x00320000 }, { 32, 0x14000080, 0x02000000 }, { 32, 0x14000084, 0x00000100 }, 
            { 32, 0x14000088, 0x02400040 }, { 32, 0x1400008c, 0x01000000 }, { 32, 0x14000090, 0x0a000100 }, { 32, 0x14000094, 0x01011f1f }, 
            { 32, 0x14000098, 0x01010101 }, { 32, 0x1400009c, 0x00030101 }, { 32, 0x140000a4, 0x00010000 }, { 32, 0x140000ac, 0x0000ffff }, 
            { 32, 0x140000c8, 0x02020101 }, { 32, 0x140000cc, 0x00000000 }, { 32, 0x140000d0, 0x01000202 }, { 32, 0x140000d4, 0x00000200 }, 
            { 32, 0x140000d8, 0x00000001 }, { 32, 0x140000dc, 0x0000ffff }, { 32, 0x140000e4, 0x02020000 }, { 32, 0x140000e8, 0x02020202 }, 
            { 32, 0x140000ec, 0x00000202 }, { 32, 0x140000f0, 0x01010064 }, { 32, 0x140000f4, 0x01010101 }, { 32, 0x140000f8, 0x00010101 }, 
            { 32, 0x140000fc, 0x00000064 }, { 32, 0x14000104, 0x02000602 }, { 32, 0x14000108, 0x06120000 }, { 32, 0x1400010c, 0x06120612 }, 
            { 32, 0x14000110, 0x06120612 }, { 32, 0x14000114, 0x01030612 }, { 32, 0x14000118, 0x00010002 }, { 32, 0x1400011C, 0x00001000 }, 
            // DDR PHY setting
            { 32, 0x14000200, 0x00000000 }, { 32, 0x14000204, 0x00000000 }, { 32, 0x14000208, 0x35002725 }, { 32, 0x14000210, 0x35002725 }, 
            { 32, 0x14000218, 0x35002725 }, { 32, 0x14000220, 0x35002725 }, { 32, 0x14000228, 0x35002725 }, { 32, 0x1400020c, 0x380002d0 }, 
            { 32, 0x14000214, 0x380002d0 }, { 32, 0x1400021c, 0x380002d0 }, { 32, 0x14000224, 0x380002d0 }, { 32, 0x1400022c, 0x380002d0 }, 
            { 32, 0x14000230, 0x00000000 }, { 32, 0x14000234, 0x00800006 }, { 32, 0x14000238, 0x60101414 }, { 32, 0x14000240, 0x60101414 }, 
            { 32, 0x14000248, 0x60101414 }, { 32, 0x14000250, 0x60101414 }, { 32, 0x14000258, 0x60101414 }, { 32, 0x1400023c, 0x00101001 }, 
            { 32, 0x14000244, 0x00101001 }, { 32, 0x1400024c, 0x00101001 }, { 32, 0x14000254, 0x00101001 }, { 32, 0x1400025c, 0x00102201 }
        };

    /* Setup PLL1 to be 800 MHz */
    if(imx50_dcd_write(device, setup_pll1_1, sizeof(setup_pll1_1) / sizeof(dcd_t)) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error writing registers [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_WRITE;
    }
    // Wait PLL1 lock
    SLEEP(10);

    if(imx50_dcd_write(device, setup_pll1_2, sizeof(setup_pll1_2) / sizeof(dcd_t)) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error writing registers [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_WRITE;
    }

    // Wait for MFN update to be completed
    SLEEP(10);

    // Switch ARM back to PLL1
    if(imx50_write_register(device, 0x53FD400C, 0x0, BITSOF(int)) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error writing registers [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_WRITE;
    }

    // Enable all clocks (they are disabled by ROM code)
    if(imx50_dcd_write(device, enable_clocks, sizeof(enable_clocks) / sizeof(dcd_t)) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error writing registers [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_WRITE;
    }

    /* Set up LPDDR1-MDDR RAM */

    // Set DDR to be div 4 to get 200MHz
    if(imx50_write_register(device, 0x53FD4098, 0x80000004, BITSOF(int)) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error writing registers [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_WRITE;
    }

    // wait for DDR dividers take effect
    SLEEP(10);

    // set up RAM
    if(imx50_dcd_write(device, lpddr1_init, sizeof(lpddr1_init) / sizeof(dcd_t)) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error writing registers [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_WRITE;
    }

    // Start ddr
    if(imx50_write_register(device, 0x14000000, 0x00000101, BITSOF(int)) != 0){
        if(IS_LOGGING(ERROR_LOG)) TRACE("[%s] E:Error writing registers [%s:%d]\n", __FUNCTION__, __FILE__, __LINE__);
        return ERROR_WRITE;
    }

    // Make sure it's started
    SLEEP(10);

    return 0;
}
