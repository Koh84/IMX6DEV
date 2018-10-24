/*******************************************************************************
 *  Copyright (c) 2018 Kelvin Koh.
 *
 *
 *  FILE
 *      serialsyshw.h
 *
 *  DESCRIPTION
 *
 *
 *
 ***************************************************************************/

#ifndef ANDROID_SERIALSYSHW_INTERFACE_H
#define ANDROID_SERIALSYSHW_INTERFACE_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>

__BEGIN_DECLS
#define SERIAL_API_VERSION HARDWARE_MODULE_API_VERSION(1,0)

#define SERIALSYSHW_HARDWARE_MODULE_ID "serialsyshw"

#define SERIAL_DEVICE_ID_MAIN "serial"


typedef struct serial_input_module {
    struct hw_module_t common;
} serial_input_module_t;


typedef struct serial_device serial_device_t;

struct serial_device {
    struct hw_device_t common;

    int (*enable_serial) (serial_device_t* dev);
    int (*disable_serial) (serial_device_t* dev);
    int (*read_timed) (serial_device_t* dev,char* buffer, int length, int timeout);
    int (*read) (serial_device_t* dev,char* buffer, int length);
    int (*write)(serial_device_t* dev,char* buffer, int length);
    int (*test) (serial_device_t* dev,int value);

    int fd;
    int enabled;

};


__END_DECLS

#endif // ANDROID_SERIALSYSHW_INTERFACE_H

