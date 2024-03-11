#ifndef _SLSPusherManager_INCLUDE_
#define _SLSPusherManager_INCLUDE_

#include <vector>
#include <string>

#include "SLSRelayManager.hpp"
#include "conf.hpp"

/**
 * CSLSPusherManager
 */
class CSLSPusherManager: public CSLSRelayManager
{
public :
	CSLSPusherManager();
    virtual ~CSLSPusherManager();

    virtual int start();
    virtual int add_reconnect_stream(char* relay_url);
    virtual int reconnect(int64_t cur_tm_ms);


private:
    int connect_all();
    virtual CSLSRelay *create_relay();
    virtual int set_relay_param(CSLSRelay *relay);
    int check_relay_param();
    int reconnect_all(int64_t cur_tm_ms, bool no_publisher);

    CSLSRWLock          m_rwclock;
    std::map<std::string, int64_t> m_map_reconnect_relay;//relay:timeout

};


#endif
