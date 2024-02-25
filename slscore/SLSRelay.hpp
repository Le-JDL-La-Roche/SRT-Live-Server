#ifndef _SLSRelay_INCLUDE_
#define _SLSRelay_INCLUDE_

#include <list>

#include "SLSRole.hpp"
#include "SLSMapPublisher.hpp"

/**
 * sls_conf_relay_t
 */

SLS_CONF_DYNAMIC_DECLARE_BEGIN(relay)
char             type[32];
char             mode[32];
char             upstreams[1024];
int              reconnect_interval;
int              idle_streams_timeout;
SLS_CONF_DYNAMIC_DECLARE_END


/**
 * relay cmd declare
 */

SLS_CONF_CMD_DYNAMIC_DECLARE_BEGIN(relay)
SLS_SET_CONF(relay, string, type,                 "pull, push",  1, 31),
SLS_SET_CONF(relay, string, mode,                 "relay mode.", 1, 31),
SLS_SET_CONF(relay, string, upstreams,            "upstreams",   1, 1023),
SLS_SET_CONF(relay, int,    reconnect_interval,   "reconnect interval, unit s", 1, 3600),
SLS_SET_CONF(relay, int,    idle_streams_timeout, "idle streams timeout, unit s", -1, 3600),

SLS_CONF_CMD_DYNAMIC_DECLARE_END


enum SLS_PULL_MODE {
	SLS_PM_LOOP  = 0,
	SLS_PM_HASH  = 1,
	SLS_PM_ALL   = 2,
};

/**
 * CSLSRelay
 */
class CSLSRelay: public CSLSRole
{
public :
	CSLSRelay();
    virtual ~CSLSRelay();

    virtual int uninit();

    void  set_map_publisher(CSLSMapPublisher *publisher);
    void  set_relay_manager(void *relay_manager);
    void *get_relay_manager();
    char *get_url();

    int           open(const char *url);
    virtual int   close();
    virtual int   get_peer_info(char *peer_name, int &peer_port);
    virtual int   get_stat_base(char *stat_base) ;
protected:
    char         m_url[URL_MAX_LEN];
    char         m_upstream[URL_MAX_LEN];
    char         m_server_ip[IP_MAX_LEN];
    int          m_server_port;

    CSLSMapPublisher  *m_map_publisher;
    void              *m_relay_manager;

    int          parse_url(char* url, char * ip, int& port, char * streamid);

};


#endif
