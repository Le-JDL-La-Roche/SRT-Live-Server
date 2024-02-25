#ifndef _TCPRole_INCLUDE_
#define _TCPRole_INCLUDE_

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <fcntl.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "conf.hpp"

struct DATA_PARAM {
	bool readable;
	bool writable;
};
/**
 * CTCPRole , the base of tcp client
 */
class CTCPRole
{
public :
    CTCPRole();
	virtual ~CTCPRole();

    int         open(int port, int backlog);
    int         open(char *host, int port);
    virtual int close();

    virtual int handler(DATA_PARAM *p);

    int         write(const char *buf, int size);
    int         read(char *buf, int size);

    char       *get_role_name();
    int         set_nonblock();
    int         get_fd();

    bool        is_valid();
protected:
    int         m_fd;
    int         m_port;
    char        m_role_name[256];
    char        m_remote_host[256];
    int         m_remote_port;
    bool        m_valid;

    int         setup();
    int         listen(int port, int backlog);
    int         connect(char *host, int port);

};


#endif
