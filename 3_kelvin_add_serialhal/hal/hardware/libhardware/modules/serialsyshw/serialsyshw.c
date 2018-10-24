/*******************************************************************************
 *  Copyright (c) 2018 Kelvin Koh.
 *
 *
 *  FILE
 *      serialsyshw.c
 *
 *  DESCRIPTION
 *
 *
 *
 ***************************************************************************/

#include <fcntl.h>
#include <errno.h>
//#include <linux/kernel.h>

#define LOG_TAG "serialsyshw"

#include <cutils/log.h>
#include <cutils/native_handle.h>

#include <hardware/serialsyshw.h>


#ifndef container_of
#define container_of(ptr, type, member)  __extension__ ({    \
     typeof(((type *)0)->member) *_ptr = (ptr);  \
     (type *)(((char *)_ptr) - offsetof(type, member)); })
#endif

static const char SERIAL_QUEUE[] =   "/data/misc/serialpipe/serialpipeq";
static const char SERIAL_CONTROL[] = "/data/misc/serialpipe/serialpipecontrol";
//static const char RFID_QUEUE[] = "/data/local/tmp/rfidq";
//#static const char RFID_CONTROL[] = "/data/local/tmp/rfidcontrol";


int serialsyshw_read(serial_device_t* dev, char* buffer, int length)
{
    int retval;

    retval = TEMP_FAILURE_RETRY(read(dev->fd, buffer, length));
    if(retval < 0)
    {
      ALOGD("serialsyshw_read(): got error : %s", strerror(errno));
    }
    else
    {
      //ALOGD("serialsyshw_read(): got data (%d) : %s", retval, buffer);
    }


    if(retval < 0)
        retval = 0;
    return retval;
}

int serialsyshw_read_timed(serial_device_t* dev,  char* buffer, int length, int timeout)
{
    int retval = 0;
    fd_set fds;
    struct timeval tv;

    tv.tv_sec = timeout ;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(dev->fd,&fds);
    if(timeout > 0)
      select(dev->fd+1, &fds, NULL, NULL, &tv);
    else if ( timeout < 0)
      select(dev->fd+1, &fds, NULL, NULL, NULL);

    if(timeout == 0 || FD_ISSET(dev->fd,&fds))
    {
      retval = TEMP_FAILURE_RETRY(read(dev->fd, buffer, length));
      if(retval < 0)
      {
        ALOGD("serialsyshw_read_timed(): got error : %s", strerror(errno));
      }
      else
      {
        //ALOGD("serialsyshw_read_timed(): got data (%d) : %s", retval, buffer);
      }
    }
    else
    {
      //ALOGD("serialsyshw_read_timed(): timed out");
    }

    if(retval < 0)
        retval = 0;

    return retval;
}


int serialsyshw_write(serial_device_t* dev, char* buffer, int length)
{
  int retval = 0;
  int fd;

  ALOGD("serialsyshw_write() Write to %s : %s", SERIAL_CONTROL, buffer);
  fd = TEMP_FAILURE_RETRY(open(SERIAL_CONTROL, O_RDWR | O_NONBLOCK));
  if(fd < 0)
  {
    ALOGE("serialsyshw_write() Cannot open SERIAL Queue %s : %s", SERIAL_CONTROL, strerror(errno));
  }
  else
  {
    retval = TEMP_FAILURE_RETRY(write(fd, buffer, length));
    if(retval < 0)
    {
      ALOGE("serialsyshw_write() Error writing to SERIAL Queue %s : %s", SERIAL_CONTROL, strerror(errno));
      retval = 0;
    }
    close(fd);
  }

  return retval;
}

int serialsyshw_test(serial_device_t* dev, int value)
{
    (void)(dev);

    return value;
}

int serialsyshw_enable_serial(serial_device_t* dev)
{
    dev->enabled = 1;
    return dev->enabled;
}

int serialsyshw_disable_serial(serial_device_t* dev)
{
    dev->enabled = 0;
    return dev->enabled;
}

static int serial_dev_exists() {
    int fd;

    fd = TEMP_FAILURE_RETRY(open(SERIAL_QUEUE, O_RDWR));
    if(fd < 0) {
        return 0;
    }

    close(fd);
    return 1;
}

static int serialsyshw_close(hw_device_t *device)
{
    serial_device_t* dev;

    dev = container_of(device, struct serial_device, common);

    ALOGD("serialsyshw_close(): Closing fd %d", dev->fd);
    close(dev->fd);
    free(device);
    return 0;
}


static int open_serialsyshw(const struct hw_module_t* module, char const* name __unused,
        struct hw_device_t** device)
{
  int fd;

    if (!serial_dev_exists()) {
        ALOGE("open_serialsyshw(): Cannot open serial queue %s, err : %s", SERIAL_QUEUE, strerror(errno));
        return -ENODEV;
    }


    fd = TEMP_FAILURE_RETRY(open(SERIAL_QUEUE, O_RDWR | O_NONBLOCK));
    if(fd < 0)
    {
        ALOGE("open_serialsyshw(): Cannot open serial queue %s, err : %s", SERIAL_QUEUE, strerror(errno));
        return -errno;
    }


    serial_device_t* dev = (serial_device_t*)malloc(sizeof(*dev));

    /* initialize */
    memset(dev, 0, sizeof(*dev));


	if (!dev) {
        return -ENOMEM;
    }

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = HARDWARE_DEVICE_API_VERSION(1,0);
    dev->common.module = (hw_module_t*)module;
    dev->common.close = serialsyshw_close;

    dev->read_timed =  serialsyshw_read_timed;
    dev->read =  serialsyshw_read;
    dev->write = serialsyshw_write;
    dev->enable_serial = serialsyshw_enable_serial;
    dev->disable_serial = serialsyshw_disable_serial;
    dev->test =  serialsyshw_test;

    dev->enabled = 1;
    dev->fd = fd;

    *device = (hw_device_t*) dev;

    ALOGD("open_serialsyshw(): Open fd %d", dev->fd);

    return 0;
}

static struct hw_module_methods_t serialsyshw_module_methods = {
    .open = open_serialsyshw
};


serial_input_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        module_api_version : SERIAL_API_VERSION,
        hal_api_version: HARDWARE_HAL_API_VERSION,
        id: SERIALSYSHW_HARDWARE_MODULE_ID,
        name: "Kelvin SERIAL HW Module",
        author: "Kelvin Koh",
        methods: &serialsyshw_module_methods,
    }
};



