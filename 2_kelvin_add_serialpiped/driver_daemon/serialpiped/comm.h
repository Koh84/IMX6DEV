
#ifndef _COMM_H
#define _COMM_H

#include <termios.h>

#define DEFAULT_PORT  "/dev/ttymxc2"
#define DEFAULT_BAUD  B115200

void comm_init(char *name, int baud);
void comm_clean();
int comm_open(char *name, int b);
int comm_close();
int comm_opened();
int comm_read(char *data, size_t len);
int comm_write(char *data, size_t len);
void comm_info();
int comm_blocking_wait(void);
int comm_get_handle(void);

static char *set_port_name(char *name);
static void ios_init(int b);

int comm_setrts(int rts);
int comm_setdtr(int dtr);

#endif /* _COMM_H */

