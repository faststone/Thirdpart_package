/*
* Test suite program based of libusb-0.1-compat testlibusb
* Copyright (c) 2013 Nathan Hjelm <hjelmn@mac.ccom>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "libusb.h"
#include "libusb_cmd_def.h"

static uint16_t VID = 0x111B;
static uint16_t PID = 0x1238;
#define MY_CONFIG 1
#define MY_INIF 0
int verbose = 0;

static int perr(char const *format, ...)
{
    va_list args;
    int r;

    va_start(args, format);
    r = vfprintf(stderr, format, args);
    va_end(args);

    return r;
}

bool dev_get_last_cmd_status(libusb_device_handle *dev, cmdarg_cmd_status_t* status)
{
    int ret;
    ret = libusb_control_transfer(dev, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_ENDPOINT_IN, IF_CMD_USB_GLOBAL, IF_CMD_USB_GLOBAL_LAST_STATUS, 0, (uint8_t*)status, sizeof(cmdarg_cmd_status_t), 1000);
    if (ret != sizeof(cmdarg_cmd_status_t)) {
        perr("get last command status failed: %s\n", libusb_error_name(ret));
        return false;
    }
    return true;
}

bool dev_check_last_cmd(libusb_device_handle *dev, uint8_t cmd_type, uint8_t cmd_val)
{
    cmdarg_cmd_status_t status;
    if (!dev_get_last_cmd_status(dev, &status) | status.cmd_req_type != cmd_type || status.cmd_req_val != cmd_val)
    {
        return false;
    }
    if (status.cmd_status == IF_CMD_STATUS_FAIL) {
        perr("%s\n", status.msg);
        return false;
    }
    return true;
}


bool dev_capture_stop(libusb_device_handle *dev)
{
    int ret;
    uint8_t cmd_type = IF_CMD_SENSOR_CTRL;
    uint8_t cmd_val = IF_CMD_SENSOR_CTRL_SAMPLING_STOP;
    ret = libusb_control_transfer(dev,LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT| LIBUSB_ENDPOINT_OUT, cmd_type, cmd_val, 0, (uint8_t *)&ret,1, 1000);
    if (ret <= 0) {
        perr("control transfer failed: %s\n", libusb_error_name(ret));
        return false;
    }
    return dev_check_last_cmd(dev, cmd_type, cmd_val);
}

bool dev_reg_read(libusb_device_handle *dev, uint16_t reg_addr, void *reg_val,uint16_t val_cnt)
{
    int ret;
    uint8_t cmd_type = IF_CMD_SENSOR_CTRL;
    uint8_t cmd_val = IF_CMD_SENSOR_CTRL_GET_REG;
    cmdarg_reg_rw_t _arg;
    cmdarg_reg_rw_t *arg = &_arg;

    arg->baseaddr = reg_addr;
    arg->target = REG_RW_TARGET_OPT9221;
    arg->val_cnt = val_cnt;

    ret = libusb_control_transfer(dev,LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT| LIBUSB_ENDPOINT_OUT, cmd_type, cmd_val, 0, (uint8_t *)arg,sizeof(cmdarg_reg_rw_t), 1000);

    if (ret != sizeof(cmdarg_reg_rw_t)) {
        perr("dev_reg_read: write phase failed\n");
        return false;
    }
    if (!dev_check_last_cmd(dev, cmd_type, cmd_val)) {
        return false;
    }

    /* 2. read-back the data */
    ret = libusb_control_transfer(dev,LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT| LIBUSB_ENDPOINT_IN, cmd_type, cmd_val, 0,(uint8_t *)reg_val, val_cnt, 1000);

    if (ret != val_cnt) {
        perr("dev_reg_read: read phase failed\n");
        return false;
    }
    return dev_check_last_cmd(dev, cmd_type, cmd_val);
}

bool dev_reg_write(libusb_device_handle *dev, uint16_t reg_addr, void *reg_val,
    uint16_t val_cnt)
{
    int ret;
    uint8_t cmd_type = IF_CMD_SENSOR_CTRL;//0xc2
    uint8_t cmd_val = IF_CMD_SENSOR_CTRL_SET_REG;//0x00
    cmdarg_reg_rw_t *arg = malloc(sizeof(cmdarg_reg_rw_t) + val_cnt);
    if (!arg) {
        perr("malloc failed\n");
        return 0;
    }

    arg->baseaddr = reg_addr;
    arg->target = REG_RW_TARGET_OPT9221;
    memcpy(arg->vals, reg_val, val_cnt);
    arg->val_cnt = val_cnt;

    ret = libusb_control_transfer(dev, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_ENDPOINT_OUT, cmd_type, cmd_val, 0, (uint8_t*)arg, sizeof(cmdarg_reg_rw_t) + val_cnt, 1000);
    free(arg);
    if (ret <= 0) {
        perr("control transfer failed: %s\n", libusb_error_name(ret));
        return false;
    }
    return 0;// dev_check_last_cmd(dev, cmd_type, cmd_val);

}

bool dev_set_mode(libusb_device_handle *dev, int m)
{
    int ret;
    //cmdarg_dev_mode_t arg;
    dmif_param_item_t item;
    item.param_id = 2;
    item.param_val_len = 1;
    uint8_t cmd_type = IF_CMD_DEV_CTRL;//0xC0
    uint8_t cmd_val = DMIF_CMD_DEV_PARAM_GET;//0x04
    //arg.mode = m;

    ret = libusb_control_transfer(dev, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_ENDPOINT_OUT, cmd_type, cmd_val, 0, (uint8_t*)&item, sizeof(item), 1000);
    if (ret <= 0) {
        perr("control transfer failed: %s\n", libusb_error_name(ret));
        return false;
    }
    return dev_check_last_cmd(dev, cmd_type, cmd_val);
}

void test_cmd(uint16_t vid, uint16_t pid)
{
    libusb_device_handle *handle;

    int r;
    int i;
    int j;

    for (j = 0; j < 1; j++)
    {
        printf("Opening device %04X:%04X...\n", vid, pid);
        handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
        if (handle == NULL)
        {
            perr(" open device failed\n");
            return;
        }

        r = libusb_set_configuration(handle, MY_CONFIG);
        if (r != LIBUSB_SUCCESS)
        {
            libusb_close(handle);
            perr("libusb_set_configuration failed: %s\n", libusb_error_name(r));
            return;
        }
        /* We need to claim the first interface */
        libusb_set_auto_detach_kernel_driver(handle, 1);
        r = libusb_claim_interface(handle, MY_INIF);
        if (r != LIBUSB_SUCCESS) {
            libusb_close(handle);
            perr("libusb_claim_interface failed: %s\n", libusb_error_name(r));
            return;
        }
        for (i = 0; i < 1; i++) 
        {
#define REG_WR_LEN 1
            uint8_t wr_vals[REG_WR_LEN];
            uint8_t rd_vals[REG_WR_LEN];

            int i;

            for (i = 0; i < sizeof(wr_vals); i++) {
                wr_vals[i] = i * 2;
            }
            memset(rd_vals, 0, sizeof(rd_vals));
             printf(" => DEV reset test ...\n");
             printf(" => DEV reset test ...%d\n", APP_WORK_MODE_TEST_USB);
             if (!dev_set_mode(handle, APP_WORK_MODE_TEST_USB))
             {
                 printf(" set mode failed\n");
             }
//              printf(" => REG read/write test ...\n");
//              dev_reg_write(handle, 0, wr_vals, sizeof(wr_vals));
//              dev_reg_read(handle, 0, rd_vals, sizeof(rd_vals));
//              if (memcmp(rd_vals, wr_vals, sizeof(rd_vals)) == 0) {
//                  printf("[OK]\n");
//              }
//              else {
//                  printf("[NG]\n");
//              }
             dev_capture_stop(handle);
        }
        //libusb_release_interface(handle, MY_INIF);
        printf("Closing device...\n");
        libusb_close(handle);
    }
}

void test_bulk_read(uint16_t vid, uint16_t pid)
{
}

void list_devices(libusb_context* ctx)
{
    libusb_device **list;
    int num_devs = libusb_get_device_list(ctx, &list);
    struct timeval to;
    int* loop_stop;
    to.tv_sec = 0;
    to.tv_usec = 200 * 1000;
    int r;
    int i;
    int j;

    libusb_device_handle *handle;

    printf("Opening device %04X:%04X...\n", VID, PID);
    handle = libusb_open_device_with_vid_pid(NULL, VID, PID);
    if (handle == NULL)
    {
        perr(" open device failed\n");
        return;
    }

    r = libusb_set_configuration(handle, MY_CONFIG);
    if (r != LIBUSB_SUCCESS)
    {
        libusb_close(handle);
        perr("libusb_set_configuration failed: %s\n", libusb_error_name(r));
        return;
    }
    /* We need to claim the first interface */
    libusb_set_auto_detach_kernel_driver(handle, 1);
    r = libusb_claim_interface(handle, MY_INIF);
    if (r != LIBUSB_SUCCESS) {
        libusb_close(handle);
        perr("libusb_claim_interface failed: %s\n", libusb_error_name(r));
        return;
    }

    do {
        int r = libusb_handle_events_timeout_completed( ctx, &to, loop_stop);

        if (r < 0) {
            if (r == LIBUSB_ERROR_INTERRUPTED) {
                
                /* just retry */
            }
            else if (r == LIBUSB_ERROR_TIMEOUT) {
 
                break;
            }
            else {
                /* There was an error. */
 
            }

            /* Break out of this loop only on fatal error.*/
            if (r != LIBUSB_ERROR_BUSY && r != LIBUSB_ERROR_TIMEOUT
                && r != LIBUSB_ERROR_OVERFLOW && r != LIBUSB_ERROR_INTERRUPTED) {
                break;
            }
        }
    } while (loop_stop && !(*loop_stop));
}
int main(int argc, char *argv[])
{
    const struct libusb_version* version;
    int r;
    version = libusb_get_version();
    printf("Using libusb v%d.%d.%d.%d\n\n", version->major, version->minor,
        version->micro, version->nano);
    r = libusb_init(NULL);
    //printf("cmd = %x=", (LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_ENDPOINT_OUT )&0x80);
    if (r < 0)
    {
        return r;
    }
    test_cmd(VID, PID);
    //test_bulk_read(VID,PID);
	return 0;
}
