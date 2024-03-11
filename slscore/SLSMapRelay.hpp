#ifndef _SLSMapRelay_INCLUDE_
#define _SLSMapRelay_INCLUDE_

#include <map>
#include <string>

#include "SLSRelayManager.hpp"
#include "SLSLock.hpp"

class CSLSMapRelay
{
public:
	CSLSMapRelay();
    virtual ~CSLSMapRelay();

    CSLSRelayManager *add_relay_manager(const char *app_uplive, const char *stream_name);
    void clear();

    int add_relay_conf(std::string app_uplive, sls_conf_relay_t * cr);
    SLS_RELAY_INFO *get_relay_conf(std::string app_uplive);

private:
    CSLSRWLock          m_rwclock;
    std::map<std::string, CSLSRelayManager *>   m_map_relay_manager;        //stream_name: relay_manager

    std::map<std::string, SLS_RELAY_INFO *>     m_map_relay_info;        //uplive: relay_conf_info
};


#endif
