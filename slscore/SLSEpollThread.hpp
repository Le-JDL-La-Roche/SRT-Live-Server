#ifndef _SLSEpollThread_INCLUDE_
#define _SLSEpollThread_INCLUDE_

#include <srt/srt.h>
#include "SLSThread.hpp"

#define MAX_SOCK_COUNT 1024

/**
 * CSLSEpollThread , the base thread class
 */
class CSLSEpollThread: public CSLSThread
{
public :
	CSLSEpollThread();
    ~CSLSEpollThread();

    virtual int     work();

    int init_epoll();
    int uninit_epoll();

protected:
    virtual int     handler();

    int         add_to_epoll(int fd, bool write) ;

    int        m_eid;
    SRTSOCKET  m_read_socks[MAX_SOCK_COUNT];
    SRTSOCKET  m_write_socks[MAX_SOCK_COUNT];

};


#endif
