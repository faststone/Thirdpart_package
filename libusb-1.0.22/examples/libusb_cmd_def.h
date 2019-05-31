#ifndef _LIBUSB_CMD_DEF_H__
#define _LIBUSB_CMD_DEF_H__


/* ---- CMD type definition -----*/
#define IF_CMD_USB_GLOBAL 0xB0
#define IF_CMD_DEV_CTRL 0xC0
#define IF_CMD_SENSOR_CTRL 0xC2
#define IF_CMD_DFU 0xC8

/* ---- CMD values */

#define IF_CMD_USB_GLOBAL_LAST_STATUS 0x00

#define IF_CMD_DEV_CTRL_RESET 0x00
#define IF_CMD_DEV_CTRL_SET_MODE 0x01
#define IF_CMD_DEV_CTRL_GET_MODE 0x02
#define DMIF_CMD_DEV_PARAM_SET   0x03
#define DMIF_CMD_DEV_PARAM_GET   0x04

#define IF_CMD_SENSOR_CTRL_SET_REG 0x00
#define IF_CMD_SENSOR_CTRL_GET_REG 0x01
#define IF_CMD_SENSOR_CTRL_SET_MFREQ 0x02
#define IF_CMD_SENSOR_CTRL_GET_MFREQ 0x03
#define IF_CMD_SENSOR_CTRL_SAMPLING_START 0x04
#define IF_CMD_SENSOR_CTRL_SAMPLING_STOP 0x05


#pragma pack(push)
#pragma pack(1)
/* USB GLOBAL */
#define IF_CMD_STATUS_OK  0
#define IF_CMD_STATUS_FAIL -1


#define IF_CMD_DFU_GET_INFO 0x00
#define IF_CMD_DFU_SET_INFO 0x01
#define IF_CMD_DFU_PACK 0x05
#define IF_CMD_DFU_FIN  0x06

/* DEV CTRL  */
typedef enum {
    RESET_TARGET_SYSTEM = 0,
    RESET_TARGET_TFC,
    RESET_TARGET_IR,
} cmdarg_reset_target_e;

typedef struct {
    int8_t cmd_status;
    uint8_t cmd_req_type;
    uint8_t cmd_req_val;
    uint8_t msg[60];
} cmdarg_cmd_status_t;

typedef struct {
    uint8_t param_id;
    uint8_t param_val_len;
    uint8_t param_val[18];
} dmif_param_item_t;

/* SENSOR CTRL  */
#define REG_RW_TARGET_INVALID 0
#define REG_RW_TARGET_OPT9221 1
#define REG_RW_TARGET_IR  2
#define REG_RW_TARGET_MCU 3

typedef struct {
    uint8_t target;  /* register rw target module. refer REG_RW_TARGET_XXX macro */
    uint32_t baseaddr; /* base address of register block */
    uint16_t val_cnt;    /* byte count of vals field */
    uint8_t vals[];  /* register value to write (no use when read reg) */
}cmdarg_reg_rw_t;

typedef struct {
    uint32_t mode;
}cmdarg_dev_mode_t;

typedef struct {
    uint32_t cnt;
}cmdarg_frame_cnt_t;

#pragma pack(pop)

#define   APP_WORK_MODE_NORMAL 0UL
#define   APP_WORK_MODE_DFU    1UL
#define   APP_WORK_MODE_TEST_USB   0xf0UL

#endif
