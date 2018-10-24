
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
//#include <errno.h>
//#include <syslog.h>


#include "comm.h"
//#include "ini.h"

#include "defs.h"

//#define LOG_TAG "rfidmanagerd"
//#include <cutils/log.h>


#define ACTIVITY_NONE     0
#define ACTIVITY_COMMS    1
#define ACTIVITY_PIPE     2

int verbose = 1;
int freq = DEFAULT_FREQ;
int baud = DEFAULT_BAUD;
char *port = DEFAULT_PORT;

u_int8_t hardwareFlags = 0;
u_int8_t formatFlags = 0;
u_int8_t customFlags = 0;

char firmwareFile[100];

static int running = 1;

static const char SERIAL_QUEUE[] = "/data/misc/serialpipe/serialpipeq";
static const char SERIAL_CONTROL[] = "/data/misc/serialpipe/serialpipecontrol";

static int config_handler(void* user, const char* section, const char* name, const char* value);
static int SendCommsData(char *buffer, int length);
static int Deamonize(void);
static void handle_signal(int sig);
static int wait_for_activity(int h1, int h2);
static void ProcessCommand(int handle);

int main(int argc, char *argv[])
{
	int retval = 0;
	int fd;
  int err = 0;
  char ibuf[MAX_IO_BUF];
  int activity;
  int hCom;
  int hPipe;

  printf("%s Ver %s running, port %s\n",PROJECT, PROJECT_VERSION, port);
  ALOGI("%s Ver %s Starting up, using port %s\n",PROJECT, PROJECT_VERSION, port);

	comm_init(port, baud);
	comm_clean();

/* attempt to open the queue */
  if (mkfifo (SERIAL_QUEUE, 0666) == -1)
  {
    ALOGE("Cannot create SERIAL Queue %s, err : %s\n", SERIAL_QUEUE, strerror(errno));
  }
  if (mkfifo (SERIAL_CONTROL, 0666) == -1)
  {
    ALOGE("Cannot create SERIAL Control %s, err : %s\n", SERIAL_CONTROL, strerror(errno));
  }

  /* setup signal handler */
  signal(SIGINT, handle_signal);
  signal(SIGHUP, handle_signal);

  signal(SIGPIPE, handle_signal);                    /* ignore pipe errors */

  Deamonize();

  comm_init(port, baud);

  ALOGD( "init hCom and hPipe...\n");

  hCom = comm_get_handle();
  hPipe = TEMP_FAILURE_RETRY(open(SERIAL_CONTROL, O_RDWR | O_NONBLOCK));
  if(hPipe <= 0)
  {
    ALOGW("Cannot open SERIAL control pipe  %s, err : %s\n", SERIAL_CONTROL, strerror(errno));
  }

  while (running)
  {
    activity = wait_for_activity(hCom, hPipe);

    if(running)
    {
      if(activity == ACTIVITY_COMMS)
      {
        if (ucgetline(ibuf, sizeof(ibuf), 1000000) > 0)
        {
          //ALOGD("Got data : %s", ibuf);
          strcat(ibuf, "\n");
          if(SendCommsData(ibuf, strlen(ibuf)) <= 0)
          {
            ALOGW("Failed to queue data : %s", ibuf);
          }
        }
      }
      else if(activity == ACTIVITY_PIPE)
      {
        ProcessCommand(hPipe);
      }
      else
      {
        // timeout
      }
    }
    //usleep(10000);
  }

  close(hPipe);

  comm_clean();

	return retval;
}
/*****************************************************************************
*
*   Function:   wait_for_activity
*
*   Comments:
*
*   Parameters: NONE
*
*   Returns:    NONE
******************************************************************************/
int wait_for_activity(int hc, int hp)
{
  fd_set rfds;
  struct timeval tv;
  int retval = ACTIVITY_NONE;
  int max = hc > hp ? hc : hp;

  FD_ZERO(&rfds);
  FD_SET(hc, &rfds);
  FD_SET(hp, &rfds);

   /* Wait up to 1 seconds. */
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  select(max+1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

  if (FD_ISSET(hc, &rfds))
    retval = ACTIVITY_COMMS;
  else if (FD_ISSET(hp, &rfds))
    retval = ACTIVITY_PIPE;

  return(retval);
}

/*****************************************************************************
*
*   Function:   ProcessCommand
*
*   Comments:
*
*   Parameters: NONE
*
*   Returns:    NONE
******************************************************************************/
void ProcessCommand(int handle)
{
  //int oldCustomFlags = customFlags;
  //int oldFormatFlags = formatFlags;
  //int oldHardwareFlags = hardwareFlags;
  //FILE* file;
  //int err;

  char cmd[100];

  //ALOGD( "ProcessCommand(): Event on SERIAL command queue\n");

  memset(cmd, 0, sizeof(cmd));
  if(TEMP_FAILURE_RETRY(read(handle, cmd, sizeof(cmd)) > 0))
  {
    ALOGD( "Processing SERIAL command %s", cmd);
    comm_write(cmd, strlen(cmd));

  }
  else
  {
    ALOGE( "Cannot read SERIAL command\n");
  }
}

/*****************************************************************************
*
*   Function:   SendCommsData
*
*   Comments:
*
*   Parameters: NONE
*
*   Returns:    NONE
******************************************************************************/
int SendCommsData(char *buffer, int length)
{
  int retval;
  int fd;

  ALOGD("Write to SERIAL Queue %s : %-*.*s", SERIAL_QUEUE, length-1, length-1, buffer);
  fd = TEMP_FAILURE_RETRY(open(SERIAL_QUEUE, O_RDWR | O_NONBLOCK));
  if(fd < 0)
  {
    ALOGE("Cannot open SERIAL Queue %s : %s\n", SERIAL_QUEUE, strerror(errno));
    return -errno;
  }

  retval = TEMP_FAILURE_RETRY(write(fd, buffer, length));
  if(retval < 0)
  {
    ALOGE("Error writing to SERIAL Queue %s : %s\n", SERIAL_QUEUE, strerror(errno));
  }
  else
  {
    //ALOGD("Write complete ok");
  }

  close(fd);

  return retval;
}

/*****************************************************************************
*
*   Function:   Deamonize
*
*   Comments:
*
*   Parameters: NONE
*
*   Returns:    NONE
******************************************************************************/
int Deamonize(void)
{
  /* Our process ID and Session ID */
  pid_t pid = 0, sid;
  running = 1;
  int fd;


  ALOGI( "Deamonize...\n");

  /* Fork off the parent process */
  pid = fork();


  /* An error occurred */
  if(pid < 0)
  {
    ALOGE( "Error in fork 1\n");
    exit(EXIT_FAILURE);
  }


  /* Success: Let the parent terminate */
  if(pid > 0)
  {
    ALOGD( "Parent terminate 1\n");
    exit(EXIT_SUCCESS);
  }


  /* On success: The child process becomes session leader */
  if(setsid() < 0)
  {
    ALOGE( "Error in setsid()\n");
    exit(EXIT_FAILURE);
  }


  /* Ignore signal sent from child to parent process */
  signal(SIGCHLD, SIG_IGN);

  /* Set new file permissions */
  umask(0);

  /* Change the working directory to the root directory */
  /* or another appropriated directory */
  chdir("/");

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  ALOGD( "Running as deamon\n");

  return(0);
}

void handle_signal(int sig)
{
  if(sig == SIGINT)
  {
    ALOGI( "Stopping daemon ...\n");
    running = 0;
    signal(SIGINT, SIG_DFL);
  }
  else if(sig == SIGHUP)
  {
    ALOGD( "Received SIGHUP signal\n");
  }
  else if(sig == SIGCHLD)
  {
    ALOGD( "Received SIGCHLD signal\n");
  }
  else if(sig == SIGPIPE)
  {
    ALOGD( "Received SIGPIPE signal\n");
  }
  else
  {
    ALOGD( "Received unknown signal: %d\n", sig);
  }
}
/* End of file */

