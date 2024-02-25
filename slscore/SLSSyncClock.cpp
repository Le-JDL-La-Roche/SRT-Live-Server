#include <stdio.h>

#include "SLSSyncClock.hpp"
#include "common.hpp"

#define TM_JITTER 1000//ms

CSLSSyncClock::CSLSSyncClock()
{
    m_jitter = TM_JITTER;
	m_begin_ms_rts = -1;
	m_begin_ms_sys = -1;
}

CSLSSyncClock::~CSLSSyncClock()
{
}

int  CSLSSyncClock::wait(int64_t rts_tm_ms)
{
	if (-1 == m_begin_ms_rts) {
		m_begin_ms_rts = rts_tm_ms;
		m_begin_ms_sys = get_time_in_milliseconds();
		return SLS_OK;
	}
	int64_t cur_sys_ms = get_time_in_milliseconds();
	int64_t sys_passed = cur_sys_ms - m_begin_ms_sys;
	int64_t rts_passed = rts_tm_ms - m_begin_ms_rts;
	int64_t d = rts_passed - sys_passed;
	if (d >= m_jitter || d <= (-1 * m_jitter)) {
		//jitter
		m_begin_ms_rts = rts_tm_ms;
		m_begin_ms_sys = cur_sys_ms;
		return SLS_OK;
	}
    if (d > 0) {
    	//printf("rts_passed=%lld, sys_passed=%lld, d=%lld\n", rts_passed, sys_passed, d);
    	msleep(d);
    }
    return SLS_OK;
}

void CSLSSyncClock::set_jitter(int v)
{
	m_jitter = v;
}

