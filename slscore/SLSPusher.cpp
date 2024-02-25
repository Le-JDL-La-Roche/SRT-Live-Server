#include <errno.h>
#include <string.h>


#include "SLSPusher.hpp"
#include "SLSLog.hpp"

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
 * CSLSPusher class implementation
 */

CSLSPusher::CSLSPusher()
{
    m_is_write             = 1;
    sprintf(m_role_name, "pusher");

}

CSLSPusher::~CSLSPusher()
{
    //release
}

int CSLSPusher::handler()
{
	return handler_write_data();
}

int   CSLSPusher::get_stat_base(char *stat_base)
{
    strcpy(stat_base, SLS_RELAY_STAT_INFO_BASE);
    return SLS_OK;
}


