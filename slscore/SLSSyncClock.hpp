#ifndef _SLSSyncLock_INCLUDE_
#define _SLSSyncLock_INCLUDE_

#include <sys/time.h>
#include <sys/types.h>

/**
 * CSLSSyncClock
 */
class CSLSSyncClock
{
public :
	CSLSSyncClock();
    ~CSLSSyncClock();

    int  wait(int64_t rts_tm_ms);
    void set_jitter(int v);
private:
    int64_t   m_begin_ms_rts;
	int64_t   m_begin_ms_sys;
	int       m_jitter;

};



#endif
