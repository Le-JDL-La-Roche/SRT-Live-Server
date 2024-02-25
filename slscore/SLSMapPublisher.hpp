#ifndef _SLSMapPublisher_INCLUDE_
#define _SLSMapPublisher_INCLUDE_

#include <map>
#include <string>

#include "conf.hpp"
#include "SLSLock.hpp"
#include "SLSRole.hpp"

class CSLSMapPublisher
{
public:
	CSLSMapPublisher();
    virtual ~CSLSMapPublisher();

    void set_conf(std::string key, sls_conf_base_t * ca);
    void set_live_2_uplive(std::string strLive, std::string strUplive);
    int  set_push_2_pushlisher(std::string app_streamname, CSLSRole * role);
    int  remove(CSLSRole *role);
    void clear();

    std::string      get_uplive(std::string key_app);
    sls_conf_base_t *get_ca(std::string key_app);

    CSLSRole *       get_publisher(std::string strAppStreamName);


private:
    std::map<std::string, std::string>           m_map_live_2_uplive;   // 'hostname/live':'hostname/uplive'
    std::map<std::string, sls_conf_base_t*>      m_map_uplive_2_conf;   // 'hostname/uplive':sls_app_conf_t
    std::map<std::string, CSLSRole *>            m_map_push_2_pushlisher;    // 'hostname/uplive/steam_name':publisher'

    CSLSRWLock          m_rwclock;


};


#endif
