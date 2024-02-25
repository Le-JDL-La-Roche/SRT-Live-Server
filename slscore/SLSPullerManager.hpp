#ifndef _SLSPulllerManager_INCLUDE_
#define _SLSPulllerManager_INCLUDE_

#include <vector>
#include <string>

#include "SLSRelayManager.hpp"
#include "conf.hpp"

/**
 * CSLSPullerManager
 */
class CSLSPullerManager: public CSLSRelayManager
{
public :
	CSLSPullerManager();
    virtual ~CSLSPullerManager();

    virtual int start();
    virtual int add_reconnect_stream(char* relay_url);
    virtual int reconnect(int64_t cur_tm_ms);

protected:

    int connect_loop();
    virtual CSLSRelay *create_relay();
    virtual int set_relay_param(CSLSRelay *relay);
    int check_relay_param();

    int   m_cur_loop_index;

};


#endif
