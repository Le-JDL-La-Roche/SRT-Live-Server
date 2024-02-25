#include <errno.h>
#include <string.h>


#include "SLSPuller.hpp"
#include "SLSLog.hpp"
#include "SLSMapRelay.hpp"

const char SLS_RELAY_STAT_INFO_BASE[] = "\
{\
\"port\": \"%d\",\
\"role\": \"%s\",\
\"pub_domain_app\": \"%s\",\
\"stream_name\": \"%s\",\
\"url\": \"%s\",\
\"remote_ip\": \"%s\",\
\"remote_port\": \"%d\",\
\"start_time\": \"%s\",\
\"kbitrate\":\
";

/**
 * CSLSPuller class implementation
 */

CSLSPuller::CSLSPuller()
{
    m_is_write             = 0;
    sprintf(m_role_name, "puller");

}

int CSLSPuller::uninit()
{
	int ret = SLS_ERROR;
	if (NULL != m_map_publisher) {
		ret = m_map_publisher->remove(this);
		log(LOG_DFLT, "[%p]CSLSPuller::uninit, removed relay from m_map_publisher, ret=%d.",
				this, ret);
	}
	if (m_map_data) {
        ret = m_map_data->remove(m_map_data_key);
		log(LOG_DFLT, "[%p]CSLSPuller::uninit, removed relay from m_map_data, ret=%d.",
				this, ret);
	}
	return CSLSRelay::uninit();

}

CSLSPuller::~CSLSPuller()
{
    //release
}

int CSLSPuller::handler()
{
	int64_t last_read_time = 0;
	int ret = handler_read_data(&last_read_time);
	if (ret >= 0) {
		//*check if there is any player?
		if (-1 == m_idle_streams_timeout) {
			return ret;
		}
		int64_t cur_time = get_time_in_milliseconds();
		if (cur_time - last_read_time >= (m_idle_streams_timeout*1000)) {
	        log(LOG_DFLT, "[%p]CSLSPuller::handler, no any reader for m_idle_streams_timeout=%ds, last_read_time=%lld, close puller.",
	        		this, m_idle_streams_timeout, last_read_time);
			m_state = SLS_RS_INVALID;
			invalid_srt();
	        return SLS_ERROR;
		}
		//*/
	}
	return ret;
}

int   CSLSPuller::get_stat_base(char *stat_base)
{
    strcpy(stat_base, SLS_RELAY_STAT_INFO_BASE);
    return SLS_OK;
}



