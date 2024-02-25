#ifndef _SLSManager_INCLUDE_
#define _SLSManager_INCLUDE_

#include <list>

#include "SLSRole.hpp"
#include "SLSRoleList.hpp"
#include "SLSGroup.hpp"
#include "SLSListener.hpp"
#include "conf.hpp"
#include "SLSMapData.hpp"
#include "SLSMapRelay.hpp"

/**
 * srt conf declare
 */
SLS_CONF_DYNAMIC_DECLARE_BEGIN(srt)
char             log_file[URL_MAX_LEN];
char             log_level[URL_MAX_LEN];
int              worker_threads;
int              worker_connections;
char             stat_post_url[URL_MAX_LEN];
int              stat_post_interval;
char             record_hls_path_prefix[URL_MAX_LEN];
SLS_CONF_DYNAMIC_DECLARE_END


/**
 * srt cmd declare
 */
  SLS_CONF_CMD_DYNAMIC_DECLARE_BEGIN(srt)
  SLS_SET_CONF(srt, string, log_file, "save log file name.", 1, URL_MAX_LEN - 1),
  SLS_SET_CONF(srt, string, log_level, "log level", 1, URL_MAX_LEN - 1),
  SLS_SET_CONF(srt, int, worker_threads, "count of worker thread, if 0, only main thread.", 0, 100),
  SLS_SET_CONF(srt, int, worker_connections, "", 1, 1024),
  SLS_SET_CONF(srt, string, stat_post_url, "statistic info post url", 1, URL_MAX_LEN - 1),
  SLS_SET_CONF(srt, int, stat_post_interval, "interval of statistic info post.", 1, 60),
  SLS_SET_CONF(srt, string, record_hls_path_prefix, "hls path prefix", 1, URL_MAX_LEN - 1),
  SLS_CONF_CMD_DYNAMIC_DECLARE_END


  /**
   * CSLSManager , manage players, publishers and listener
   */
  class CSLSManager {
  public:
    CSLSManager();
    virtual ~CSLSManager();


    int  start();
    int  stop();
    int  reload();
    int  single_thread_handler();
    int  check_invalid();
    bool is_single_thread();

    void       get_stat_info(std::string &info);
    static int stat_client_callback(void *p, HTTP_CALLBACK_TYPE type, void *v, void *context);

  private:
    std::list<CSLSListener *>     m_servers;
    int                           m_server_count;
    CSLSMapData *m_map_data;
    CSLSMapPublisher *m_map_publisher;
    CSLSMapRelay *m_map_puller;
    CSLSMapRelay *m_map_pusher;

    std::list<CSLSGroup *>     m_workers;
    int                           m_worker_threads;

    CSLSRoleList *m_list_role;
    CSLSGroup *m_single_group;
};

#endif
