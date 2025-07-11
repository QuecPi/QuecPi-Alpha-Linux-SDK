/*
 *  Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *  Not a Contribution.
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef HCI_UART_H
#define HCI_UART_H

#include <asm-generic/ioctls.h>
#include <termios.h>

#ifndef TIOCPMGET
#ifdef TIOCPMGET_544D
#define TIOCPMGET 0x544d /* PM get */
#define TIOCPMPUT 0x544e /* PM put */
#define TIOCPMACT 0x544f /* PM is active */
#else
#define TIOCPMGET 0x5441 /* PM get */
#define TIOCPMPUT 0x5442 /* PM put */
#define TIOCPMACT 0x5443 /* PM is active */
#endif
#endif
/* Variables to identify the platform */
/*BT HS UART TTY DEVICE */
#define BT_HS_UART_DEVICE "/dev/ttyHS1"

/**** baud rates ****/
#define USERIAL_BAUD_300 0
#define USERIAL_BAUD_600 1
#define USERIAL_BAUD_1200 2
#define USERIAL_BAUD_2400 3
#define USERIAL_BAUD_9600 4
#define USERIAL_BAUD_19200 5
#define USERIAL_BAUD_57600 6
#define USERIAL_BAUD_115200 7
#define USERIAL_BAUD_230400 8
#define USERIAL_BAUD_460800 9
#define USERIAL_BAUD_921600 10
#define USERIAL_BAUD_1M 11
#define USERIAL_BAUD_1_5M 12
#define USERIAL_BAUD_2M 13
#define USERIAL_BAUD_3M 14
#define USERIAL_BAUD_4M 15
#define USERIAL_BAUD_AUTO 16

/**** Data Format ****/
/* Stop Bits */
#define USERIAL_STOPBITS_1 1
#define USERIAL_STOPBITS_1_5 (1 << 1)
#define USERIAL_STOPBITS_2 (1 << 2)

/* Parity Bits */
#define USERIAL_PARITY_NONE (1 << 3)
#define USERIAL_PARITY_EVEN (1 << 4)
#define USERIAL_PARITY_ODD (1 << 5)

/* Data Bits */
#define USERIAL_DATABITS_5 (1 << 6)
#define USERIAL_DATABITS_6 (1 << 7)
#define USERIAL_DATABITS_7 (1 << 8)
#define USERIAL_DATABITS_8 (1 << 9)

/* HCI Packet types */
#define HCI_COMMAND_PKT 0x01
#define HCI_ACLDATA_PKT 0x02
#define HCI_SCODATA_PKT 0x03
#define HCI_EVENT_PKT 0x04
#define HCI_VENDOR_PKT 0xff

/* HCI Command/Event Opcode */
#define HCI_RESET 0x0C03
#define EVT_CMD_COMPLETE 0x0E

#define HCI_READ_BD_ADDR 0x1009
#define HCIC_PARAM_SIZE_READ_CMD 0

#define BT_DEBUG printf
#define LOG__LEVEL_V 0
#define LOG__LEVEL_D 1
#define LOG__LEVEL_I 2
#define LOG__LEVEL_W 3
#define LOG__LEVEL_E 4

#define LOG__LEVEL 5

#define LOG_V(fmt, ...)             \
  {                                 \
    if (LOG__LEVEL >= LOG__LEVEL_V) \
      BT_DEBUG(fmt, ##__VA_ARGS__); \
    BT_DEBUG("\n");                 \
  }

#define LOG_D(fmt, ...)             \
  {                                 \
    if (LOG__LEVEL >= LOG__LEVEL_D) \
      BT_DEBUG(fmt, ##__VA_ARGS__); \
    BT_DEBUG("\n");                 \
  }

#define LOG_I(fmt, ...)             \
  {                                 \
    if (LOG__LEVEL >= LOG__LEVEL_I) \
      BT_DEBUG(fmt, ##__VA_ARGS__); \
    BT_DEBUG("\n");                 \
  }

#define LOG_W(fmt, ...)             \
  {                                 \
    if (LOG__LEVEL >= LOG__LEVEL_W) \
      BT_DEBUG(fmt, ##__VA_ARGS__); \
    BT_DEBUG("\n");                 \
  }

#define LOG_E(fmt, ...)             \
  {                                 \
    if (LOG__LEVEL >= LOG__LEVEL_E) \
      BT_DEBUG(fmt, ##__VA_ARGS__); \
    BT_DEBUG("\n");                 \
  }

#define MAX_COL 16
#define SHOW_LINE_SIZE 16

#define BT_HEX_DUMP(data, len)                           \
  {                                                      \
    if (BTAPP_ENABLE_DUMP)                               \
    {                                                    \
      do                                                 \
      {                                                  \
        uint32_t line;                                   \
        uint32_t curline = 0;                            \
        uint32_t curcol = 0;                             \
        uint8_t showline[SHOW_LINE_SIZE];                \
        uint32_t data_pos = 0;                           \
        if (len % MAX_COL)                               \
        {                                                \
          line = len / MAX_COL + 1;                      \
        }                                                \
        else                                             \
        {                                                \
          line = len / MAX_COL;                          \
        }                                                \
                                                         \
        for (curline = 0; curline < line; curline++)     \
        {                                                \
          sprintf(showline, "%08xh:", curline *MAX_COL); \
          printf("%s", showline);                        \
          for (curcol = 0; curcol < MAX_COL; curcol++)   \
          {                                              \
            if (data_pos < len)                          \
            {                                            \
              printf("%02x ", data[data_pos]);           \
              data_pos++;                                \
              continue;                                  \
            }                                            \
            else                                         \
            {                                            \
              break;                                     \
            }                                            \
          }                                              \
          printf("\n");                                  \
        }                                                \
      } while (0);                                       \
    }                                                    \
  }

/* Command opcode pack/unpack */
#define cmd_opcode_pack(ogf, ocf) (uint16_t)((ocf & 0x03ff) | (ogf << 10))

#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)
/* These are the ioctl values used for bt_wake ioctl via UART driver. you may
 * need to redefine them on you platform!
 * Logically they need to be unique and not colide with existing uart ioctl's.
 */
#ifndef USERIAL_IOCTL_BT_WAKE_ASSERT
#define USERIAL_IOCTL_BT_WAKE_ASSERT 0x8003
#endif
#ifndef USERIAL_IOCTL_BT_WAKE_DEASSERT
#define USERIAL_IOCTL_BT_WAKE_DEASSERT 0x8004
#endif
#ifndef USERIAL_IOCTL_BT_WAKE_GET_ST
#define USERIAL_IOCTL_BT_WAKE_GET_ST 0x8005
#endif
#endif // (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)

/* UART CLOCK IOCTLS*/
/* UART CLOCK IOCTLS*/
#define USERIAL_OP_CLK_ON TIOCPMGET    /* PM get */
#define USERIAL_OP_CLK_OFF TIOCPMPUT   /* PM put */
#define USERIAL_OP_CLK_STATE TIOCPMACT /* PM is active */

/******************************************************************************
**  Type definitions
******************************************************************************/

/* Structure used to configure serial port during open */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
// typedef unsigned char bool;
typedef unsigned int uint32_t;

#define TRUE 1
#define FALSE 0

typedef struct
{
  uint16_t fmt; /* Data format */
  uint8_t baud; /* Baud rate */
} tUSERIAL_CFG;

typedef enum
{
#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)
  USERIAL_OP_ASSERT_BT_WAKE,
  USERIAL_OP_DEASSERT_BT_WAKE,
  USERIAL_OP_GET_BT_WAKE_STATE,
#endif
  USERIAL_OP_FLOW_ON,
  USERIAL_OP_FLOW_OFF,
  USERIAL_OP_NOP,
} userial_vendor_ioctl_op_t;

/* UPIO signals */
enum
{
  UPIO_BT_WAKE = 0,
  UPIO_HOST_WAKE,
  UPIO_LPM_MODE,
  UPIO_MAX_COUNT
};

/* UPIO assertion/deassertion */
enum
{
  UPIO_UNKNOWN = 0,
  UPIO_DEASSERT,
  UPIO_ASSERT
};

#define VND_PORT_NAME_MAXLEN 256

/* vendor serial control block */
typedef struct
{
  int fd;                 /* fd to Bluetooth device */
  struct termios termios; /* serial terminal of BT port */
  char port_name[VND_PORT_NAME_MAXLEN];
} vnd_userial_cb_t;

typedef struct
{
  uint8_t ncmd;
  uint16_t opcode;
} __attribute__((packed)) evt_cmd_complete;

typedef struct
{
  uint8_t status;
  uint8_t ncmd;
  uint16_t opcode;
} __attribute__((packed)) evt_cmd_status;

typedef struct
{
  uint16_t opcode;
  uint8_t plen;
} __attribute__((packed)) hci_command_hdr;

typedef struct
{
  uint8_t evt;
  uint8_t plen;
} __attribute__((packed)) hci_event_hdr;

/******************************************************************************
**  Extern
******************************************************************************/
extern vnd_userial_cb_t vnd_userial;

/*******************************************************************************
**
** Function        userial_vendor_init
**
** Description     Initialize userial vendor-specific control block
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_init(char *uart_device);

/*******************************************************************************
**
** Function        userial_vendor_open
**
** Description     Open the serial port with the given configuration
**
** Returns         device fd
**
*******************************************************************************/
int userial_vendor_open(char *uart_device);

/*******************************************************************************
**
** Function        userial_vendor_close
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_close(void);

/*******************************************************************************
**
** Function        userial_vendor_set_baud
**
** Description     Set new baud rate
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_set_baud(uint8_t userial_baud);

/*******************************************************************************
**
** Function        userial_vendor_ioctl
**
** Description     ioctl inteface
**
** Returns         int error
**
*******************************************************************************/
int userial_vendor_ioctl(userial_vendor_ioctl_op_t op, int *p_data);

/*******************************************************************************
**
** Function        userial_to_tcio_baud
**
** Description     helper function converts USERIAL baud rates into TCIO
**                  conforming baud rates
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t userial_to_tcio_baud(uint8_t cfg_baud, uint32_t *baud);

/*******************************************************************************
**
** Function        userial_to_baud_tcio
**
** Description     helper function converts TCIO baud rate into integer
**
** Returns         uint32_t
**
*******************************************************************************/
int userial_tcio_baud_to_int(uint32_t baud);

/*******************************************************************************
**
** Function        read_hci_event
**
** Description     Read HCI event during vendor initialization
**
** Returns         int: size to read
**
*******************************************************************************/
int read_hci_event(int fd, unsigned char *buf, int size);

int read_new_hci_event(int fd, unsigned char *buf, int size);
int read_cmd_compl_event(int fd, unsigned char *buf, int size);

#endif /* HCI_UART_H */
