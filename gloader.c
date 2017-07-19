/* gloader - cmdline based GRBL gcode-loader
 * Author: Skammel <git@skammel.de> - Skammel Solutions
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <termio.h>
#include <unistd.h>
#include <getopt.h>
#include "gloader.h"

int parse_params (int argc, char **argv, cmd_params * params);
void sel_baudrate (speed_t * baud, uint32_t b);
int read_data (int fd, char *data);
int check_state (int fd, int alarm);
void check_gcode (FILE * f);
int check_gcode_line (char *line);
void grbl_stop (int fd);
int dev_init (char *dev, speed_t baud);
void close_all (int fd, FILE * f);

int main (int argc, char **argv)
{
  cmd_params params;
  int fd = -1;
  FILE *f = NULL;
  char str[MAX_STR_LEN] = "";
  char state[MAX_STR_LEN] = "";
  int cnt = 0;

  memset (&params, 0, sizeof (params));
  strcpy (params.dev, DFL_DEV);
  params.baud = DFL_BAUD;
  if (parse_params (argc, argv, &params))
    return 1;

  if (params.hlp)
    return 0;

  if (!(f = fopen (params.in_file, "r")))
   {
     fprintf (stderr, "cannot open file %s\n", params.in_file);
     return 1;
   }

  if ((fd = dev_init (params.dev, params.baud)) < 0 && !params.chk)
   {
     fprintf (stderr, "cannot open %s\n", params.dev);
     fclose (f);
     return 1;
   }

  if (params.chk)
   {
     printf ("check only - no transfer to machine!\n\n");
     check_gcode (f);
     fclose (f);
     printf ("\n\n check done \n\n");
     return 0;
   }

  if (params.rst)
    grbl_stop (fd);
  switch (check_state (fd, params.alarm))
   {
   case ERR_TO:
     fprintf (stderr,
              "communication error - check your device and baudrate settings\n");
     fprintf (stderr, "device = %s\n\n", params.dev);
     close_all (fd, f);
     return 2;

   case STATE_RUN:
     printf
       ("GRBL is busy - try again later or use -r to stop/reset the machine\n");
     close_all (fd, f);
     return 0;

   case STATE_ALARM:
     if (params.alarm)
       break;
     printf ("GRBL is in alarm state - use -a to unlock the machine\n");
     close_all (fd, f);
     return 3;
   }

  while (!feof (f))
   {
     memset (str, 0, sizeof (str));
     fgets (str, MAX_GCODE_LINE + 10, f);
     cnt++;

     switch (check_gcode_line (str))
      {
      case LINE_EMPTY:
        printf ("%4d: skipping - line is empty\n", cnt);
        break;
      case LINE_NO_CMD:
        printf ("%4d: skipping - not a valid gcode line\n", cnt);
        break;
      case LINE_TO_LONG:
        printf ("%4d: skipping - line has more than %d chars\n", cnt,
                MAX_GCODE_LINE);
        printf ("stop sending commands to GRBL\n");
        close_all (fd, f);
        return 1;
        break;
      case LINE_COMMENT:
        printf ("%4d: skipping - comment\n", cnt);
        break;
      default:

        printf ("%4d: %4zu - ", cnt, write (fd, str, strlen (str)));
        memset (state, 0, sizeof (state));
        if (read_data (fd, state) == ERR_TO)
         {
           fprintf (stderr, "error reading data from GRBL\n");
           close_all (fd, f);
           return 2;
         }
        printf ("%s", state);

        if (!strncmp (state, "error:", 6) && !params.ign)
         {
           printf ("gcode sending stopped - use -i to ignore errors\n\n");
           close_all (fd, f);
           return 4;
         }
      }

   }

  if (!params.nowait)
   {
     printf ("\nmachine is still working - please wait...");
     fflush (stdout);
     while (check_state (fd, 0) == STATE_RUN)
       usleep (500 * 1000);

     printf (" done\n");

   }

  close_all (fd, f);
  printf ("\n");

  return 0;
}

int dev_init (char *dev, speed_t baud)
{
  int fd = -1;
  struct termios tiodata;

  fd = open (dev, O_RDWR | O_NOCTTY);
  if (fd < 0)
    return fd;

  tiodata.c_cflag = (baud | CLOCAL | CREAD | CS8);
  tiodata.c_iflag = IGNPAR;
  tiodata.c_oflag = 0;
  tiodata.c_lflag = 0;
  tiodata.c_cc[VMIN] = 0;
  tiodata.c_cc[VTIME] = COM_TIMEOUT;    /* read timeout 100ms */

  tcflush (fd, TCIFLUSH);       /* clean line */
/* activate new settings */
  if (tcsetattr (fd, TCSANOW, &tiodata))
   {
     close (fd);
     return -1;
   }

  return fd;
}

int parse_params (int argc, char **argv, cmd_params * params)
{
  char c = 0;

/* options */
  struct option long_options[] = {
    {"alarm", 0, 0, 'a'},
    {"baud", 0, 0, 'b'},
    {"check", 0, 0, 'c'},
    {"device", 0, 0, 'd'},
    {"help", 0, 0, 'h'},
    {"ignore", 0, 0, 'i'},
    {"no-wait", 0, 0, 'n'},
    {"reset", 0, 0, 'r'},
    {0, 0, 0, 0}
  };

  while (1)
   {
     int option_index = 0;

     c = getopt_long (argc, argv, "ab:cd:hinr", long_options, &option_index);
     if (c == -1)
       break;

     switch (c)
      {
      case 0:
        fprintf (stderr, " option %s", long_options[option_index].name);
        if (optarg)
          fprintf (stderr, " with arg %s", optarg);
        fprintf (stderr, "\n");
        break;

      case 'a':
        params->alarm = 1;
        break;
      case 'i':
        params->ign = 1;
        break;
      case 'c':
        params->chk = 1;
        break;
      case 'r':
        params->rst = 1;
        break;
      case 'h':
        params->hlp = 1;
        printf ("%s\n", VERSION);
        printf ("%s\n", AUTHOR);
        printf ("%s\n", COPY);
        printf (USAGE, argv[0]);
        break;

      case 'd':
        strcpy (params->dev, optarg);
        break;
      case 'n':
        params->nowait = 1;
        break;
      case 'b':
        sel_baudrate (&params->baud, atoi (optarg));
        break;
      case '?':
        return 2;
        break;

      }
   }

  if (optind + 1 == argc)
   {
     strcpy (params->in_file, argv[optind]);
     return 0;
   }

  if (optind < argc)
   {
     fprintf (stderr, "unknown option: %s\n", argv[optind]);
     return 2;
     while (optind < argc)
       fprintf (stderr, "%s", argv[optind++]);
     fprintf (stderr, "\n");

     return 2;
   }

  return 0;

}

void close_all (int fd, FILE * f)
{
  if (f)
    fclose (f);
  f = NULL;
  close (fd);
}

void sel_baudrate (speed_t * baud, uint32_t b)
{

  switch (b)
   {
   case 9600:
     *baud = B9600;
     break;
   case 19200:
     *baud = B19200;
     break;
   case 38400:
     *baud = B38400;
     break;
   case 57600:
     *baud = B57600;
     break;
   case 115200:
     *baud = B115200;
     break;

   default:
     fprintf (stderr, "baudrate %d not supported\n", b);
   }

}

int check_state (int fd, int alarm)
{
  char cmd[32] = "?";
  char str[MAX_DATA_LEN] = "";

  write (fd, cmd, strlen (cmd));
  memset (str, 0, sizeof (str));
  if (read_data (fd, str) == ERR_TO)
    return ERR_TO;

  if (!strncmp (str, "<Run,", 5))
    return STATE_RUN;
  if (!strncmp (str, "<Alarm,", 7))
    if (!strncmp (str, "<Alarm,", 7))
     {
       if (alarm == 1)
        {
          strcpy (cmd, "$X\n");
          memset (str, 0, sizeof (str));
          write (fd, cmd, strlen (cmd));
          if (read_data (fd, str) == ERR_TO)
            return ERR_TO;
        }
       else
         return STATE_ALARM;

     }

//  printf ("%s", str);
  return 0;
}

int read_data (int fd, char *data)
{
  int retry = 0;
  int cnt = 0;

  while ((cnt = read (fd, data, MAX_DATA_LEN)) <= 0 && retry < READ_RETRY)
    retry++;

  if (retry >= READ_RETRY)
    return ERR_TO;

  return cnt;
}

int check_gcode_line (char *line)
{
  int i = 0, is_cmd = 0;
  if (line[0] == 0 || line[0] == '\n' || line[0] == '\r')
    return LINE_EMPTY;
  if (line[0] == '(')
    return LINE_COMMENT;

  for (i = 0; i < (int) strlen (line); i++)
   {
     if (line[i] == '(' && !is_cmd)
       return LINE_COMMENT;
     if (is_cmd && line[i] == ')')
       return LINE_NO_CMD;
     if (line[i] != '(' && line[i] != '\t' && line[i] != '\r'
         && line[i] != '\n' && line[i] != ' ')
       is_cmd = 1;

     if (line[i] == '\n' || line[i] == '\r' || line[i] == '(')
      {
        line[i] = 0;
        break;
      }
   }

  strcat (line, "\n\0");
  if (strlen (line) > MAX_GCODE_LINE)
    return LINE_TO_LONG;
  if (!is_cmd)
    return LINE_NO_CMD;

  return 0;

}

void grbl_stop (int fd)
{
  char cmd[16] = { 0x18, 0 };   /* ctrl-x */
  char ret[MAX_DATA_LEN] = "";

  memset (ret, 0, sizeof (ret));
  write (fd, cmd, strlen (cmd));
  sleep (1);                    /* give GRBL time to reset */
  if (read_data (fd, ret) == ERR_TO)
    return;

  memset (ret, 0, sizeof (ret));
  strcpy (cmd, "$X\n");
  write (fd, cmd, strlen (cmd));
  read_data (fd, ret);
}

void check_gcode (FILE * f)
{
  char str[MAX_GCODE_LINE + 20] = "";
  int cnt = 0;
  if (f == NULL)
    fprintf (stderr, "nothing to check\n\n");

  while (!feof (f))
   {
     memset (str, 0, sizeof (str));
     fgets (str, MAX_GCODE_LINE + 10, f);
     cnt++;

     switch (check_gcode_line (str))
      {
      case LINE_EMPTY:
        printf ("%4d: skipping - line is empty\n", cnt);
        break;
      case LINE_NO_CMD:
        printf ("%4d: skipping - not a valid gcode line\n", cnt);
        break;
      case LINE_TO_LONG:
        printf ("%4d: skipping - line has more than %d chars\n", cnt,
                MAX_GCODE_LINE);
        printf ("stop sending commands to GRBL\n");
        break;
      case LINE_COMMENT:
        printf ("%4d: skipping - comment\n", cnt);
        break;
      default:
        printf ("%4d: %4zu - ok \n", cnt, strlen (str));
      }

   }
}
