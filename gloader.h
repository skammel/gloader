#ifndef __GLOADER_H
#define __GLOADER_H
#define DFL_DEV "/dev/ttyUSB0"
#define DFL_BAUD B115200        /* default baudrate */
#define MAX_DATA_LEN 128
#define MAX_STR_LEN 128
#define READ_RETRY 50           /* read-retry-limit */
#define COM_TIMEOUT 5           /* commuincation timeout (x * 100ms) */

#define ERR_TO -1               /* timeout error */
#define STATE_IDLE 0
#define STATE_ALARM 1           /* GRBL is in alarm state */
#define STATE_RUN 2

#define MAX_GCODE_LINE 100
#define LINE_EMPTY -1
#define LINE_COMMENT -2
#define LINE_NO_CMD -3
#define LINE_TO_LONG -4

typedef struct
{
  char dev[MAX_STR_LEN];
  char in_file[MAX_STR_LEN * 2];
  speed_t baud;
  int ign, hlp, alarm, nowait, rst, chk;
} cmd_params;

#define VERSION "gloader V1.0 - cmdline based GRBL gcode-loader"
#define AUTHOR "Skammel <git@skammel.de>"
#define COPY "Skammel Solutions - http://www.skammel.de - GPL"
#define USAGE "Usage: %s [options] <gcode-file> \n \
  -a, --alarm - unlock the machine if alarm is set \n \
  -b, --baud - optional baudrate (default 115200) \n \
  -c, --check - only check the given gcode-file and exit \n \
  -d, --device - GRBL device (default /dev/ttyUSB0) \n \
  -i, --ignore - ignore errors \n \
  -n, --no-wait - do not wait after the last gcode is sent to GRBL \n \
  -r, --reset - reset/stop the running job \n "

#endif
