#ifndef _SECFB_IOCTL_H_
#define _SECFB_IOCTL_H_

#include <linux/ioctl.h>

struct secfb_io {
        int fd;
        size_t size;
};

#define SECFB_IOCTL_GET_SECFB_FD        _IOW('r', 1, struct secfb_io)

#endif /* _SECFB_IOCTL_H_ */
