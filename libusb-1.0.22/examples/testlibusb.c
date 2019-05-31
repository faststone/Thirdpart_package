#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdbool.h>
#include <inttypes.h>

#include "libusb.h"
#include "pthread.h"
#include "libusb_cmd_def.h"
#include "dm_port.h"

static uint16_t VID = 0x111B;
static uint16_t PID = 0x1238;
#define MY_CONFIG 1
#define MY_INTF 0
#define EP_IN 0x81
#define EP_OUT 0x02
static int perr(char const *format, ...)
{
    va_list args;
    int r;

    va_start(args, format);
    r = vfprintf(stderr, format, args);
    va_end(args);

    return r;
}

bool dev_get_last_cmd_status(libusb_device_handle *dev, cmdarg_cmd_status_t *s)
{
    int r;

    r = libusb_control_transfer(dev,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT
        | LIBUSB_ENDPOINT_IN,
        IF_CMD_USB_GLOBAL, IF_CMD_USB_GLOBAL_LAST_STATUS, 0, (uint8_t *)s,
        sizeof(cmdarg_cmd_status_t), 1000);
    if (r != sizeof(cmdarg_cmd_status_t)) {
        perr("get last command status failed: %s\n", libusb_error_name(r));
        return false;
    }
    return true;
}

bool dev_check_last_cmd(libusb_device_handle *dev, uint8_t cmd_type,
    uint8_t cmd_val)
{
    cmdarg_cmd_status_t status;

    if (!dev_get_last_cmd_status(dev, &status)
        || status.cmd_req_type != cmd_type
        || status.cmd_req_val != cmd_val) {
        return false;
    }
    if (status.cmd_status == IF_CMD_STATUS_FAIL) {
        perr("%s\n", status.msg);
        return false;
    }
    return true;
}

bool dev_set_mode(libusb_device_handle *dev, int m)
{
    int r;
    cmdarg_dev_mode_t arg;
    uint8_t cmd_type = IF_CMD_DEV_CTRL;
    uint8_t cmd_val = IF_CMD_DEV_CTRL_SET_MODE;

    arg.mode = m;
    //Sleep(1);
    r = libusb_control_transfer(dev,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT
        | LIBUSB_ENDPOINT_OUT, cmd_type, cmd_val, 0,
        (uint8_t *)&arg, sizeof(arg), 1000);
    if (r <= 0) {
        perr("control transfer failed: %s\n", libusb_error_name(r));
        return false;
    }
    return dev_check_last_cmd(dev, cmd_type, cmd_val);
}

bool dev_reg_write(libusb_device_handle *dev, uint16_t reg_addr, void *reg_val,
    uint16_t val_cnt)
{
    int r;
    uint8_t cmd_type = IF_CMD_SENSOR_CTRL;
    uint8_t cmd_val = IF_CMD_SENSOR_CTRL_SET_REG;

    cmdarg_reg_rw_t *arg = malloc(sizeof(cmdarg_reg_rw_t)+val_cnt);

    if (!arg) {
        perr("malloc failed\n");
        return 0;
    }
    printf("sizeof(cmdarg_reg_rw_t)+val_cnt = %d\n", sizeof(cmdarg_reg_rw_t)+val_cnt);
    arg->baseaddr = reg_addr;
    arg->target = REG_RW_TARGET_OPT9221; // todo
    memcpy(arg->vals, reg_val, val_cnt);
    arg->val_cnt = val_cnt;

    r = libusb_control_transfer(dev,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT
        | LIBUSB_ENDPOINT_OUT, cmd_type, cmd_val, 0, (uint8_t *)arg,
        sizeof(cmdarg_reg_rw_t)+val_cnt, 1000);
    free(arg);
    if (r <= 0) {
        perr("control transfer failed: %s\n", libusb_error_name(r));
        return false;
    }
    return dev_check_last_cmd(dev, cmd_type, cmd_val);
}

bool dev_reg_read(libusb_device_handle *dev, uint16_t reg_addr, void *reg_val,
    uint16_t val_cnt)
{
    int r;
    uint8_t cmd_type = IF_CMD_SENSOR_CTRL;
    uint8_t cmd_val = IF_CMD_SENSOR_CTRL_GET_REG;
    cmdarg_reg_rw_t _arg;
    cmdarg_reg_rw_t *arg = &_arg;

      /* 1. write the reg block cmd with base addr and length */
      arg->baseaddr = 0x21b4;
      arg->target = REG_RW_TARGET_OPT9221; // todo
      arg->val_cnt =1;
  
      r = libusb_control_transfer(dev,
          LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT
          | LIBUSB_ENDPOINT_OUT, cmd_type, cmd_val, 0, (uint8_t *)arg,
          sizeof(cmdarg_reg_rw_t), 1000);
      //DATA: [01 00 00 00 00 00 00 00 20 00 4E 00 ] 
  
      if (r != sizeof(cmdarg_reg_rw_t)) {
          perr("dev_reg_read: write phase failed\n");
          return false;
      }
      if (!dev_check_last_cmd(dev, cmd_type, cmd_val)) {
          printf("dev_reg_read : 01,dev_check_last_cmd failed\n");
          return false;
      }

    /* 2. read-back the data */
    r = libusb_control_transfer(dev,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT
        | LIBUSB_ENDPOINT_IN, cmd_type, cmd_val, 0,
        (uint8_t *)reg_val, val_cnt, 1000);

    if (r != val_cnt) {
        perr("dev_reg_read: read phase failed\n");
        return false;
    }

    return dev_check_last_cmd(dev, cmd_type, cmd_val);
}

bool dev_capture_start(libusb_device_handle *dev, int frame_cnt)
{
#if 0
    int r;
    // start trigger the transfer from device
    char cmd = 0xE2;
    int rd_bytes;
    r = libusb_bulk_transfer(dev, EP_OUT, (uint8_t*)&cmd, 1, &rd_bytes, 2000);
    if (r < 0 && r != LIBUSB_ERROR_TIMEOUT) {
        perr(" bulk write on EP1 failed: %s\n", libusb_error_name(r));
    }
    return true;
#else
    int r;
    cmdarg_frame_cnt_t arg;
    uint8_t cmd_type = IF_CMD_SENSOR_CTRL;
    uint8_t cmd_val = IF_CMD_SENSOR_CTRL_SAMPLING_START;
    arg.cnt = frame_cnt;

    r = libusb_control_transfer(dev,LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT| LIBUSB_ENDPOINT_OUT, cmd_type, cmd_val, 0, (uint8_t *)&arg, sizeof(arg), 1000);
    if (r <= 0) {
        perr("control transfer failed: %s\n", libusb_error_name(r));
        return false;
    }
    return dev_check_last_cmd(dev, cmd_type, cmd_val);
#endif

}

bool dev_capture_stop(libusb_device_handle *dev)
{
#if 0
    // stop trigger the transfer from device
    char cmd = 0xE8;
    int rd_bytes;
    int r;
    r = libusb_bulk_transfer(dev, EP_OUT, (uint8_t*)&cmd, 1, &rd_bytes,
        2000);
    if (r < 0 && r != LIBUSB_ERROR_TIMEOUT) {
        perr(" bulk write on EP1 failed: %s\n", libusb_error_name(r));
    }
    return true;
#else
    int r;
    uint8_t cmd_type = IF_CMD_SENSOR_CTRL;
    uint8_t cmd_val = IF_CMD_SENSOR_CTRL_SAMPLING_STOP;

    r = libusb_control_transfer(dev,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT
        | LIBUSB_ENDPOINT_OUT, cmd_type, cmd_val, 0, (uint8_t *)&r,
        1, 1000);
    if (r <= 0) {
        perr("control transfer failed: %s\n", libusb_error_name(r));
        return false;
    }
    return dev_check_last_cmd(dev, cmd_type, cmd_val);
#endif
}

void test_cmd(uint16_t vid, uint16_t pid)
{
    libusb_device_handle *handle;
    //	libusb_device *dev;
    int r;
    int i, j;

    for (j = 0; j < 1; j++) {
        printf("Opening device %04X:%04X...\n", vid, pid);
        handle = libusb_open_device_with_vid_pid(NULL, vid, pid);

        if (handle == NULL) {
            perr("  Failed.\n");
            return;
        }

        r = libusb_set_configuration(handle, MY_CONFIG);
        if (r != LIBUSB_SUCCESS) {
            libusb_close(handle);
            perr("libusb_set_configuration failed: %s\n", libusb_error_name(r));
            return;
        }

        /* We need to claim the first interface */
        libusb_set_auto_detach_kernel_driver(handle, 1);
        r = libusb_claim_interface(handle, MY_INTF);
        if (r != LIBUSB_SUCCESS) {
            libusb_close(handle);
            perr("libusb_claim_interface failed: %s\n", libusb_error_name(r));
            return;
        }

        for (i = 0; i < 1; i++) {

#define REG_WR_LEN 2
            uint8_t wr_vals[REG_WR_LEN];
            uint8_t rd_vals[REG_WR_LEN];
            int i;

//             for (i = 0; i < sizeof(wr_vals); i++) {
//                 wr_vals[i] = i;
//             }
            wr_vals[0] = 0x40;
            wr_vals[1] = 0x62;
            memset(rd_vals, 0, sizeof(rd_vals));

//             printf(" => DEV reset test ...\n");
//             if (!dev_set_mode(handle, APP_WORK_MODE_TEST_USB)) {
//                 printf(" set mode failed\n");
//             }
//  
            printf(" => REG read/write test ...\n");
            //int ret = dev_reg_write(handle, 0, wr_vals, sizeof(wr_vals));
            //printf("dev_reg_write=%d\n",ret);
            dev_reg_read(handle, 0x21B4, rd_vals, sizeof(rd_vals));
 
            for (i = 0; i < sizeof(rd_vals); i++) {
                printf("[%d] : %02x:%02x \n", i, wr_vals[i],rd_vals[i]);
            }
            if (memcmp(rd_vals, wr_vals, sizeof(rd_vals)) == 0) {
                printf("[OK]\n");
            }
            else {
                printf("[NG]\n");
            }

            /* Test write with zero length */
            dev_capture_stop(handle);
        }
        libusb_release_interface(handle, MY_INTF);
        printf("Closing device...\n");
        libusb_close(handle);
    }
}
void* th_test_entry(void *arg)
{
    printf(" [th] enter test thread. sleeping 2 seconds ... \n");
    sleep(2);
    printf(" [th] exit test thread ... \n");
    return NULL;
}

void test_pthread(void)
{
    pthread_t th_reading;

    printf("--> Test pthread: create reading thread\n");
    if (pthread_create(&th_reading, NULL, th_test_entry, NULL) < 0) {
        perr(" create thread failed\n");
        return;
    }
    printf(" wait reading thread ... \n");
    pthread_join(th_reading, NULL);
}
typedef union {
    uint8_t reserved[8];
    struct {
        int16_t temp_avrg;
        int16_t temp_ib;
    }temp;
}epc_frame_reserv_u;
typedef struct {
    uint32_t frame_size;   // size in bytes 
    uint16_t frame_idx;    // incremented frame index 
    uint16_t frame_format; // frame format code (sensor dependent)
    //compressed packet: 0x0200|[0:raw; 2:4xdcs]; packet 
    //picture : 0x0000 | [0:gray; 2: DCSx4]; 
    union {
        uint8_t val[12];
        struct { //for original frame 
            uint16_t  frame_width; // frame property (sensor dependent) 
            uint16_t  frame_height; // frame property (sensor dependent) 
            epc_frame_reserv_u  reserved; // frame property (sensor dependent) 
        }f;
        struct { //for h264 compressed packet 
            uint32_t fhi_len;
            //uint32_t flo_len; 
            uint32_t zip_len; //length before compress 
            //uint8_t codec_type; // 0: none; 1:h264; 2:hi7+lo9 
            int8_t temp1;
            int8_t temp2;
            uint16_t ts;
            // uint8_t temp3; 
        }pkt;
        struct { //for lossless packet 
            uint16_t w;
            uint16_t h;
            uint32_t src_len; //length before compress 
            int8_t t_sensor;
            int8_t t_ib;
            //uint8_t reserved[2]; 
            uint16_t ts;
        }pkt2;

    };

    //uint8_t reserved[8]; 

}dm_frinfo_t;
typedef struct {
    uint16_t sync_word;
    uint16_t idx; // increment stream hdr index
    uint32_t len; // stream lenghth in bytes

    dm_frinfo_t finfo; // frame info

    uint32_t reserved; //Data chksum
    union {
        uint16_t val;
        struct {
            uint16_t fr_bound : 1; // frame boundary flag
            uint16_t reserved : 7;

            int16_t en_data_cksum : 1;
            int16_t codec_type : 3;  //0:none; 1:h264; 2:h264-7+zip-9
            int16_t reserved2 : 4;

        }bits;
    } flag;   // stream flag

    uint16_t cksum; //paket header checksum
}dm_stream_hdr_t;
static uint16_t _stream_hdr_hash16(const void *buf, int len, uint16_t seed)
{
    uint16_t sum1 = seed & 0xff;
    uint16_t sum2 = seed >> 8;
    const uint8_t *data = (uint8_t *)buf;
    int index;

    for (index = 0; index < len; ++index) {
        sum1 = (sum1 + data[index]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }

    return (sum2 << 8) | sum1;
}

static uint16_t _stream_hdr_update_cksum(dm_stream_hdr_t *hdr)
{
    return _stream_hdr_hash16(hdr, offsetof(dm_stream_hdr_t, cksum), 0);
}

#define XFER_BUF_SIZE (640*480*2+128)

static bool read_stop = false;
static bool write_stop = false; 

static uint8_t read_buf[XFER_BUF_SIZE];
static uint8_t write_buf[XFER_BUF_SIZE];
int loop = 0;
static void* th_reading_entry(void *arg)
{
    uint16_t seed = 0;
    int rd_bytes, last_rd, total_rd, ret;
    struct timeval last_ts, cur_ts;
    libusb_device_handle *handle = arg;

    printf(" [th] enter reading thread\n");

    {
        /* clear statistics */
        last_rd = total_rd = 0;
        gettimeofday(&last_ts, NULL);
    }
    while (!read_stop) {
        memset(read_buf,0,sizeof(read_buf));
        ret = libusb_bulk_transfer(handle, EP_IN, read_buf,XFER_BUF_SIZE, &rd_bytes, 100);
        if (ret < 0) {
            if (ret != LIBUSB_ERROR_TIMEOUT) {
                perr(" bulk read on EP1 failed: %s\n", libusb_error_name(ret));
                break;
            }
            else {
                usleep(10000);
            }
        }
        else {
            for (int i = 0; i < 36; i++)
            {
                printf("%02x ", (int)read_buf[i]);
            }
            printf("\n");
            usleep(50000);
            dm_stream_hdr_t *p_hdr = (dm_stream_hdr_t*)read_buf;
            uint16_t seed = _stream_hdr_update_cksum(p_hdr);
            printf("rd_bytes=%d,seed=%d\n", rd_bytes,seed);
            total_rd += rd_bytes;
            gettimeofday(&cur_ts, NULL);

            if (cur_ts.tv_sec > last_ts.tv_sec + 2) {
                float elapse_us = (cur_ts.tv_sec - last_ts.tv_sec) * 1000000 + (cur_ts.tv_usec - last_ts.tv_usec);
                int bytes_total = total_rd - last_rd;

                printf("  USB rx speed: %.3f MBytes/s (%u bytes since last)\n", (float)bytes_total / elapse_us, bytes_total);

                last_rd = total_rd = 0;
                memcpy(&last_ts, &cur_ts, sizeof(cur_ts));
            }
        }
    }
    printf(" [th] exit reading thread\n");
    return NULL;
}

static void* th_writing_entry(void *arg)
{
    int wr_bytes, last_wr, total_wr, ret, i;
    struct timeval last_ts, cur_ts;
    libusb_device_handle *handle = arg;

    printf(" [th] enter writing thread\n");

    for (i = 0; i < sizeof(write_buf); i++) {
        write_buf[i] = i & 0xff;
    }

    /* clear statistics */
    last_wr = total_wr = 0;
    gettimeofday(&last_ts, NULL);

    while (!write_stop) {
        ret = libusb_bulk_transfer(handle, EP_OUT, (uint8_t *)write_buf, XFER_BUF_SIZE, &wr_bytes, 100);
        if (ret < 0) {
            if (ret != LIBUSB_ERROR_TIMEOUT) {
                perr(" bulk write on EP1 failed: %s\n", libusb_error_name(ret));
                break;
            }
            else {
                usleep(1000);
            }
        }
        else {
            total_wr += wr_bytes;
            gettimeofday(&cur_ts, NULL);

            if (cur_ts.tv_sec > last_ts.tv_sec + 2) {
                float elapse_us = (cur_ts.tv_sec - last_ts.tv_sec) * 1000000 + (cur_ts.tv_usec - last_ts.tv_usec);
                int bytes_total = total_wr - last_wr;

                printf("  USB tx speed: %.3f MBytes/s (%u bytes since last)\n", (float)bytes_total / elapse_us, bytes_total);

                last_wr = total_wr = 0;
                memcpy(&last_ts, &cur_ts, sizeof(cur_ts));
            }
        }
    }
    printf(" [th] exit writing thread\n");
    return NULL;
}

static void test_bulk_read(uint16_t vid, uint16_t pid)
{
    libusb_device_handle *handle;
    //	libusb_device *dev;
    int r;
    pthread_t th_reading;

    printf("Opening device %04X:%04X...\n", vid, pid);
    handle = libusb_open_device_with_vid_pid(NULL, vid, pid);

    if (handle == NULL) {
        perr("  Failed.\n");
        return;
    }
    r = libusb_set_configuration(handle, MY_CONFIG);
    if (r != LIBUSB_SUCCESS) {
        libusb_close(handle);
        perr("libusb_claim_interface failed: %s\n", libusb_error_name(r));
        return;
    }
    /* We need to claim the first interface */
    libusb_set_auto_detach_kernel_driver(handle, 1);
    r = libusb_claim_interface(handle, MY_INTF);
    if (r != LIBUSB_SUCCESS) {
        libusb_close(handle);
        perr("libusb_claim_interface failed: %s\n", libusb_error_name(r));
        return;
    }
    printf(" start sampling \n");
    {
        dev_capture_start(handle, 0);
    }
    // create thread to monitor the reading
    printf("create reading thread\n");
    if (pthread_create(&th_reading, NULL, th_reading_entry, handle) < 0) {
        perr(" create thread failed\n");
        return;
    }

    printf("wait 10 seconds... \n");
    sleep(25);

    printf(" stop sampling \n");
    {
        dev_capture_stop(handle);
    }

    // stop reading thread
    printf(" Stop read thread\n");
    read_stop = true;
    pthread_join(th_reading, NULL);

    printf("Closing device...\n");
    libusb_close(handle);
}


static void test_bulk_write(uint16_t vid, uint16_t pid)
{
    libusb_device_handle *handle;
    //	libusb_device *dev;
    int r;
    pthread_t th_write;

    printf("Opening device %04X:%04X...\n", vid, pid);
    handle = libusb_open_device_with_vid_pid(NULL, vid, pid);

    if (handle == NULL) {
        perr("  Failed.\n");
        return;
    }
    r = libusb_set_configuration(handle, MY_CONFIG);
    if (r != LIBUSB_SUCCESS) {
        libusb_close(handle);
        perr("libusb_claim_interface failed: %s\n", libusb_error_name(r));
        return;
    }
    /* We need to claim the first interface */
    libusb_set_auto_detach_kernel_driver(handle, 1);
    r = libusb_claim_interface(handle, MY_INTF);
    if (r != LIBUSB_SUCCESS) {
        libusb_close(handle);
        perr("libusb_claim_interface failed: %s\n", libusb_error_name(r));
        return;
    }
    // create thread to monitor the reading
    printf("create writing thread\n");
    if (pthread_create(&th_write, NULL, th_writing_entry, handle) < 0) {
        perr(" create thread failed\n");
        return;
    }

    printf("wait 10 seconds... \n");
    sleep(10);

    // stop reading thread
    printf(" Stop write thread\n");
    write_stop = true;
    pthread_join(th_write, NULL);

    printf("Closing device...\n");
    libusb_close(handle);
}
#define BUFFER_POOL_SIZE  (320 * 240 * 2 * 4)
struct pbuf {
    uint32_t len;
    uint32_t width;
    uint32_t height;
    uint8_t payload[BUFFER_POOL_SIZE];
};
void test_p(uint8_t* p)
{
    p[1] = 1;
}

int main(int argc, char **argv)
{
    //	bool show_help = false;
    //	bool debug_mode = false;
    const struct libusb_version *version;
    int r;

    version = libusb_get_version();
    printf("Using libusb v%d.%d.%d.%d\n\n", version->major, version->minor,
        version->micro, version->nano);
    r = libusb_init(NULL);
    if (r < 0)
        return r;
    printf("-> test cmd \n");
    test_cmd(VID, PID);
    struct pbuf p;
    
    test_bulk_read(VID, PID);
    printf("-> test bulk write \n");
    test_bulk_write(VID, PID);


    //test_pthread();
    libusb_exit(NULL);

    return 0;
}
