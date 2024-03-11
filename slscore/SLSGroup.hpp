#ifndef _SLSGroup_INCLUDE_
#define _SLSGroup_INCLUDE_

#include <map>

#include "SLSEpollThread.hpp"
#include "SLSRoleList.hpp"
#include "SLSRole.hpp"
#include "SLSMapRelay.hpp"
#include "HttpClient.hpp"

/**
 * CSLSGroup , group of players, publishers and listener
 */
class CSLSGroup : public CSLSEpollThread
{
public :
	CSLSGroup();
    ~CSLSGroup();

    int  start();
    int  stop();
    void reload();

    void set_role_list(CSLSRoleList *list_role);
    void set_worker_connections(int n);
    void set_worker_number(int n);

    virtual int     handler();

    void set_stat_post_interval(int interval);
    void set_http_stat_post(CHttpClient *p);

    void get_stat_info(std::string &info);

protected:
    virtual void    clear();

private:
    CSLSRoleList                  *m_list_role;
    std::list<CSLSRole *>          m_list_wait_http_role;
    std::map<int, CSLSRole *>      m_map_role;
    std::list<CSLSRelayManager *>  m_list_reconnect_relay_manager;


    void  idle_check();
    void  check_reconnect_relay();
    void  check_invalid_sock();
    void  check_new_role();
    void  check_wait_http_role();

    int           m_worker_connections;
    int           m_worker_number;
    int64_t       m_cur_time_microsec;
    bool          m_reload;

    int64_t       m_stat_post_last_tm_ms;
    int           m_stat_post_interval;
    CSLSMutex     m_mutex_stat;
    std::string   m_stat_info;

};


#endif
