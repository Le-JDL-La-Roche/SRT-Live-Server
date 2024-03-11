#ifndef _SLSPublisher_INCLUDE_
#define _SLSPublisher_INCLUDE_

#include <list>

#include "SLSRole.hpp"
#include "SLSRoleList.hpp"
#include "SLSMapPublisher.hpp"

/**
 * sls_conf_app_t
 */
SLS_CONF_DYNAMIC_DECLARE_BEGIN(app)
char             app_player[STR_MAX_LEN];
char             app_publisher[STR_MAX_LEN];
int              publisher_exit_delay;
char             record_hls[SHORT_STR_MAX_LEN];
int              record_hls_segment_duration;
SLS_CONF_DYNAMIC_DECLARE_END

/**
 * app cmd declare
 */
SLS_CONF_CMD_DYNAMIC_DECLARE_BEGIN(app)
SLS_SET_CONF(app, string, app_player,           "live", 1, STR_MAX_LEN-1),
SLS_SET_CONF(app, string, app_publisher,        "uplive", 1, STR_MAX_LEN-1),
SLS_SET_CONF(app, int,    publisher_exit_delay, "delay exit time, unit second.", 1, 300),
SLS_SET_CONF(app, string, record_hls,           "record_hls switch", 1, SHORT_STR_MAX_LEN-1),
SLS_SET_CONF(app, int,    record_hls_segment_duration,        "record_hls_segment_duration", 1, 3600),
SLS_CONF_CMD_DYNAMIC_DECLARE_END


/**
 * CSLSPublisher
 */
class CSLSPublisher: public CSLSRole
{
public :
    CSLSPublisher();
    virtual ~CSLSPublisher();

    void set_map_publisher(CSLSMapPublisher * publisher);

    virtual int init();
    virtual int uninit();

    virtual int  handler();
private:
    CSLSMapPublisher  * m_map_publisher;

};


#endif
