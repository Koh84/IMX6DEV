
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


#include "defs.h"
#include "comm.h"

extern int verbose;

static int port = -1;
static int baud = DEFAULT_BAUD;
static char *port_name = NULL;
static int oldmc = 0;
static int wait_flag = 1;

static struct termios ios, old_ios;

static char *set_port_name(char *name)
{
	if (port_name)
		free(port_name);
	if (name) {
    //VERBOSE(3, "Set new port name\n");
		port_name = strdup(name);
	}
	else
		port_name = strdup(DEFAULT_PORT);
	return port_name;
}

void signal_handler_IO (int status)
{
  //ALOGV("received SIGIO signal %d", status);
  wait_flag = 0;
  status++;
}

int comm_open(char *name, int b)
{
	int mc;
	int retval;
  struct sigaction saio;           /* definition of signal action */


	if (port > 0)
		comm_close();
	if (name)
		set_port_name(name);
	if (b) ios_init(b);

  ALOGD( "Opening port %s...", port_name);
	port = open(port_name, O_RDWR | O_NONBLOCK | O_NOCTTY);
	if (port < 0) {
    ALOGE("Can not open port");
		return -1;
	}

  /* install the signal handler before making the device asynchronous */
  saio.sa_handler = signal_handler_IO;
  saio.sa_mask = 0;
  saio.sa_flags = 0;
  saio.sa_restorer = NULL;
  sigaction(SIGIO,&saio,NULL);

  /* allow the process to receive SIGIO */
  fcntl(port, F_SETOWN, getpid());
  /* Make the file descriptor asynchronous (the manual page says only
     O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
  fcntl(port, F_SETFL, FASYNC);

  //VERBOSE(3, "Retrive old port attributes...\n");
	if (tcgetattr(port, &old_ios) < 0) {
    ALOGE("Can not get port parameters");
		comm_close();
		return -1;
	}
  //VERBOSE(3, "Apply new port attributes...\n");
	if (tcsetattr(port, TCSANOW, &ios) < 0) {
    ALOGE("Can not set port parameters");
		comm_close();
		return -1;
	}

  //VERBOSE(3, "Getting modem control lines state\n");
	retval = ioctl(port, TIOCMGET, &mc);
	if (retval < 0) {
    ALOGE("Can not get modem control lines state");
		return retval;
	}
	oldmc = mc;
	mc &= (~(TIOCM_RTS | TIOCM_DTR));
  //VERBOSE(3, "Setting RTS & DTR lines to OFF\n");
	retval = ioctl(port, TIOCMSET, &mc);
	if (retval < 0) {
    ALOGE("Can not set RTS & DTR lines");
		return retval;
	}

  ALOGD( "Opened.");
	return 0;
}

// void comm_close()
int comm_close()
{
	int retval;

  ALOGD("Closing port...");
	if (port < 0)
		return 0;
  //VERBOSE(3, "Apply old modem control lines state...\n");
	retval = ioctl(port, TIOCMSET, &oldmc);
	if (retval < 0) {
    ALOGE("Can not set RTS & DTR lines");
		return retval;
	}
  //VERBOSE(3, "Apply old port attributes...\n");
	tcsetattr(port, TCSANOW, &old_ios);
	close(port);
	port = -1;
	return 0;
}

int comm_opened()
{
	return port >= 0;
}

static void ios_init(int b)
{
  ALOGD( "Initializing termios structure.");
	cfmakeraw(&ios);

	/* Input flags */
	ios.c_iflag |= IGNBRK | IGNPAR;
	/* Control flags */
	ios.c_cflag |= CREAD | CLOCAL;
	ios.c_cflag &= ~CRTSCTS;

/* canonical input */
  //ios.c_lflag = ICANON;

	/* Control char's */
	ios.c_cc[VINTR] = 0;
	ios.c_cc[VQUIT] = 0;
	ios.c_cc[VERASE] = 0;
	ios.c_cc[VKILL] = 0;
	ios.c_cc[VEOF] = 0;
	ios.c_cc[VMIN] = 0;
	ios.c_cc[VEOL] = 0;
	ios.c_cc[VTIME] = 0;
	ios.c_cc[VEOL2] = 0;
	ios.c_cc[VSTART] = 0;
	ios.c_cc[VSTOP] = 0;
	ios.c_cc[VSUSP] = 0;
	ios.c_cc[VLNEXT] = 0;
	ios.c_cc[VWERASE] = 0;
	ios.c_cc[VREPRINT] = 0;
	ios.c_cc[VDISCARD] = 0;

	baud = DEFAULT_BAUD;
	switch (b) {
		case 9600:
			baud = 9600;
			break;
		case 19200:
			baud = 19200;
			break;
		case 38400:
			baud = 38400;
			break;
		case 57600:
			baud = B57600;
			break;
		case 115200:
			baud = B115200;
			break;
		case 230400:
			baud = B230400;
			break;
	}
	cfsetospeed(&ios, baud);
	cfsetispeed(&ios, baud);
}

void comm_init(char *name, int baud)
{
	set_port_name(name);
	ios_init(baud);
}

void comm_clean()
{
	comm_close();
	FREE(port_name);
}

void comm_info()
{
  long b;
  ALOGI("Port:\t\t%s %s", port_name, comm_opened() ? "opened." : "closed.");
	switch (baud) {
		case B9600:
      b = 9600;
			break;
		case B19200:
      b = 19200;
			break;
		case B38400:
      b = 38400;
			break;
		case B115200:
      b = 115200;
			break;
		case B230400:
      b = 230400;
			break;
		case B57600:
		default:
      b = 57600;
			break;
	}
  ALOGI("Baud:\t\t%ld.",b);
}

int comm_blocking_wait(void)
{
  fd_set rfds;
  struct timeval tv;
  int retval = 0;

   /* Watch stdin (fd 0) to see when it has input. */
  FD_ZERO(&rfds);
  FD_SET(port, &rfds);

   /* Wait up to 1 seconds. */
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  select(port+1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

  if (FD_ISSET(port, &rfds))                        /* input from source 1 available */
    retval = 1;

  return(retval);
}


int comm_get_handle(void)
{
  return(port);
}

int comm_read(char *data, size_t size)
{
	int retval = 0;

	if (port < 0)
		retval = comm_open(NULL, 0);
	if (retval < 0)
		return retval;
  //ALOGD( "Reading from device...");
	return read(port, data, size);
}

int comm_write(char *data, size_t len)
{
	int retval = 0;
	int count;

	if (port < 0)
		retval = comm_open(NULL, 0);
	if (retval < 0)
		return retval;

	count = len;
	while (count > 0) {
    //VERBOSE(5, "Writing to device... ");
		retval = write(port, data, count);
		if (retval < 0) {
			if (errno == EAGAIN)
				continue;
			else
				return retval;
		}
    //VERBOSE(5, "%d bytes.\n", retval);
		count -= retval;
		data += retval;
	}

	return len;
}

int comm_setrts(int rts)
{
	int retval = 0;
	int mc;

	if (port < 0)
		retval = comm_open(NULL, 0);
	if (retval < 0)
		return retval;

  //VERBOSE(3, "Getting modem control lines state\n");
	retval = ioctl(port, TIOCMGET, &mc);
	if (retval < 0) {
    ALOGE("Can not get modem control lines state");
		return retval;
	}
  //VERBOSE(4, "mc = 0x%08X TIOCM_RTS = 0x%08X\n", mc, TIOCM_RTS);
	if (rts)
		mc |= TIOCM_RTS;
	else
		mc &= (~TIOCM_RTS);
  //VERBOSE(4, "mc = 0x%08X TIOCM_RTS = 0x%08X\n", mc, TIOCM_RTS);
  //VERBOSE(3, "Setting RTS line to %s\n", rts ? "ON" : "OFF");
	retval = ioctl(port, TIOCMSET, &mc);
	if (retval < 0) {
    ALOGE("Can not set RTS line");
		return retval;
	}
	return 0;
}

int comm_setdtr(int dtr)
{
	int retval = 0;
	int mc;

	if (port < 0)
		retval = comm_open(NULL, 0);
	if (retval < 0)
		return retval;

  //VERBOSE(3, "Getting modem control lines state\n");
	retval = ioctl(port, TIOCMGET, &mc);
	if (retval < 0) {
    ALOGE("Can not get modem control lines state");
		return retval;
	}
  //VERBOSE(4, "mc = 0x%08X TIOCM_DTR = 0x%08X\n", mc, TIOCM_DTR);
	if (dtr)
		mc |= TIOCM_DTR;
	else
		mc &= (~TIOCM_DTR);
  //VERBOSE(4, "mc = 0x%08X TIOCM_DTR = 0x%08X\n", mc, TIOCM_DTR);
  //VERBOSE(3, "Setting DTR line to %s\n", dtr ? "ON" : "OFF");
	retval = ioctl(port, TIOCMSET, &mc);
	if (retval < 0) {
    ALOGE("Can not set DTR line");
		return retval;
	}
	return 0;
}

/* End of file */

