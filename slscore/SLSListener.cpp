#include <errno.h>
#include <string.h>
#include <vector>

#include "SLSListener.hpp"
#include "SLSLog.hpp"
#include "SLSPublisher.hpp"
#include "SLSPlayer.hpp"
#include "SLSPullerManager.hpp"
#include "SLSPusherManager.hpp"

const char SLS_SERVER_STAT_INFO_BASE[] = "\
{\
\"port\": \"%d\",\
\"role\": \"%s\",\
\"pub_domain_app\": \"\",\
\"stream_name\": \"\",\
\"url\": \"\",\
\"remote_ip\": \"\",\
\"remote_port\": \"\",\
\"start_time\": \"%s\",\
\"kbitrate\": \"0\"\
}";

const char SLS_PUBLISHER_STAT_INFO_BASE[] = "\
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

const char SLS_PLAYER_STAT_INFO_BASE[] = "\
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
 * server conf
 */
SLS_CONF_DYNAMIC_IMPLEMENT(server)

/**
 * CSLSListener class implementation
 */

  CSLSListener::CSLSListener() {
  m_conf = NULL;
  m_back_log = 1024;
  m_is_write = 0;
  m_port = 0;

  m_list_role = NULL;
  m_map_publisher = NULL;
  m_map_puller = NULL;
  m_idle_streams_timeout = UNLIMITED_TIMEOUT;
  m_idle_streams_timeout_role = 0;
  m_stat_info = std::string("");
  memset(m_http_url_role, 0, URL_MAX_LEN);
  memset(m_record_hls_path_prefix, 0, URL_MAX_LEN);

  sprintf(m_role_name, "listener");
}

CSLSListener::~CSLSListener() { }

int CSLSListener::init() {
  int ret = 0;
  return CSLSRole::init();
}

int CSLSListener::uninit() {
  CSLSLock lock(&m_mutex);
  stop();
  return CSLSRole::uninit();
}

void CSLSListener::set_role_list(CSLSRoleList *list_role) {
  m_list_role = list_role;
}

void CSLSListener::set_map_publisher(CSLSMapPublisher *publisher) {
  m_map_publisher = publisher;
}

void CSLSListener::set_map_puller(CSLSMapRelay *map_puller) {
  m_map_puller = map_puller;
}

void CSLSListener::set_map_pusher(CSLSMapRelay *map_pusher) {
  m_map_pusher = map_pusher;
}

void CSLSListener::set_record_hls_path_prefix(char *path) {
  if (path != NULL && strlen(path) > 0) {
    strcpy(m_record_hls_path_prefix, path);
  }
}

int CSLSListener::init_conf_app() {
  string strLive;
  string strUplive;
  string strLiveDomain;
  string strUpliveDomain;
  string strTemp;
  vector<string> domain_players;
  sls_conf_server_t *conf_server;

  if (NULL == m_map_puller) {
    log(LOG_ERRO, "Init configuration app failed, m_map_puller is null.");
    return SLS_ERROR;
  }

  if (NULL == m_map_pusher) {
    log(LOG_ERRO, "Init configuration app failed, m_map_pusher is null.");
    return SLS_ERROR;
  }

  if (!m_conf) {
    log(LOG_ERRO, "Init configuration app failed, m_conf is null.");
    return SLS_ERROR;
  }
  conf_server = (sls_conf_server_t *)m_conf;

  m_back_log = conf_server->backlog;
  m_idle_streams_timeout_role = conf_server->idle_streams_timeout;
  strcpy(m_http_url_role, conf_server->on_event_url);

  //domain
  domain_players = sls_conf_string_split(string(conf_server->domain_player), string(" "));
  if (domain_players.size() == 0) {
    log(LOG_ERRO, "Init configuration app failed, wrong domain player '%s'.", conf_server->domain_player);
    return SLS_ERROR;
  }
  strUpliveDomain = conf_server->domain_publisher;
  if (strUpliveDomain.length() == 0) {
    log(LOG_ERRO, "Init configuration app failed, wrong domain publisher '%s'.", conf_server->domain_publisher);
    return SLS_ERROR;
  }
  sls_conf_app_t *conf_app = (sls_conf_app_t *)conf_server->child;
  if (!conf_app) {
    log(LOG_ERRO, "Init configuration app failed, no app configuration info.");
    return SLS_ERROR;
  }

  int app_count = sls_conf_get_conf_count(conf_app);
  sls_conf_app_t *ca = conf_app;
  for (int i = 0; i < app_count; i++) {
    strUplive = ca->app_publisher;
    if (strUplive.length() == 0) {
      log(LOG_ERRO, "Init configuration app failed, wrong app publisher '%s' and domain publisher '%s'.",
        strUplive.c_str(), strUpliveDomain.c_str());
      return SLS_ERROR;
    }
    strUplive = strUpliveDomain + "/" + strUplive;
    m_map_publisher->set_conf(strUplive, ca);

    strLive = ca->app_player;
    if (strLive.length() == 0) {
      log(LOG_ERRO, "Init configuration app failed, wrong app player '%s' and domain publisher '%s'.",
        strLive.c_str(), strUpliveDomain.c_str());
      return SLS_ERROR;
    }

    for (int j = 0; j < domain_players.size(); j++) {
      strLiveDomain = domain_players[j];
      strTemp = strLiveDomain + "/" + strLive;
      if (strUplive == strTemp) {
        log(LOG_ERRO, "Init configuration app failed, domain publisher and domain player must not be equal.");
        return SLS_ERROR;
      }
      //m_map_live_2_uplive[strTemp]  = strUplive;
      m_map_publisher->set_live_2_uplive(strTemp, strUplive);

      log(LOG_DFLT, "Added app player '%s' and app publisher '%s'.",
        strTemp.c_str(), strUplive.c_str());
    }

    if (NULL != ca->child) {
      sls_conf_relay_t *cr = (sls_conf_relay_t *)ca->child;
      while (cr) {
        if (strcmp(cr->type, "pull") == 0) {
          if (SLS_OK != m_map_puller->add_relay_conf(strUplive.c_str(), cr)) {
            log(LOG_WARN, "[%p]CSLSListener::init_conf_app, m_map_puller.add_app_conf failed. relay type='%s', app push='%s'.",
              this, cr->type, strUplive.c_str());
          }
        } else if (strcmp(cr->type, "push") == 0) {
          if (SLS_OK != m_map_pusher->add_relay_conf(strUplive.c_str(), cr)) {
            log(LOG_WARN, "[%p]CSLSListener::init_conf_app, m_map_pusher.add_app_conf failed. relay type='%s', app push='%s'.",
              this, cr->type, strUplive.c_str());
          }
        } else {
          log(LOG_ERRO, "[%p]CSLSListener::init_conf_app, wrong relay type='%s', app push='%s'.",
            this, cr->type, strUplive.c_str());
          return SLS_ERROR;
        }
        cr = (sls_conf_relay_t *)cr->sibling;
      }
    }

    ca = (sls_conf_app_t *)ca->sibling;
  }
  return SLS_OK;

}

int CSLSListener::start() {
  int ret = 0;
  std::string strLive;
  std::string strUplive;
  std::string strLiveDomain;
  std::string strUpliveDomain;


  if (NULL == m_conf) {
    log(LOG_ERRO, "Start SLS Listener failed, configuration is null.", this);
    return SLS_ERROR;
  }

  ret = init_conf_app();
  if (SLS_OK != ret) {
    log(LOG_ERRO, "Start SLS Listener failed, unable to init configuration app.", this);
    return SLS_ERROR;
  }

  //init listener
  if (NULL == m_srt)
    m_srt = new CSLSSrt();

  int latency = ((sls_conf_server_t *)m_conf)->latency;
  if (latency > 0) {
    m_srt->libsrt_set_latency(latency);
  }

  m_port = ((sls_conf_server_t *)m_conf)->listen;
  ret = m_srt->libsrt_setup(m_port);
  if (SLS_OK != ret) {
    log(LOG_ERRO, "Start SLS Listener failed, unable to setup LibSRT.", this);
    return ret;
  }
  log(LOG_DBUG, "[%p]CSLSListener::start, libsrt_setup ok.", this);


  ret = m_srt->libsrt_listen(m_back_log);
  if (SLS_OK != ret) {
    log(LOG_ERRO, "Start SLS Listener failed, unable to listen LibSRT.", this);
    return ret;
  }

  log(LOG_DBUG, "[%p]CSLSListener::start, m_list_role=%p.", this, m_list_role);
  if (NULL == m_list_role) {
    log(LOG_DBUG, "[%p]CSLSListener::start, m_roleList is null.", this);
    return ret;
  }

  log(LOG_DBUG, "[%p]CSLSListener::start, push to m_list_role=%p.", this, m_list_role);
  m_list_role->push(this);

  return ret;
}

int CSLSListener::stop() {
  int ret = SLS_OK;
  log(LOG_DFLT, "Stopped SLS Listener.");
  return ret;
}


int CSLSListener::handler() {
  int ret = SLS_OK;
  int fd_client = 0;
  CSLSSrt *srt = NULL;
  char sid[1024] = { 0 };
  int  sid_size = sizeof(sid);
  char host_name[URL_MAX_LEN] = { 0 };
  char app_name[URL_MAX_LEN] = { 0 };
  char stream_name[URL_MAX_LEN] = { 0 };
  char key_app[URL_MAX_LEN] = { 0 };
  char publisher_stream_id[URL_MAX_LEN] = { 0 };
  char player_stream_id[URL_MAX_LEN] = { 0 };
  char tmp[URL_MAX_LEN] = { 0 };
  char peer_name[IP_MAX_LEN] = { 0 };
  int  peer_port = 0;
  int  client_count = 0;

  //1: accept
  fd_client = m_srt->libsrt_accept();
  if (ret < 0) {
    log(LOG_ERRO, "SRT accept failed.");
    CSLSSrt::libsrt_neterrno();
    return client_count;
  }
  client_count = 1;

  //2.check streamid, split it
  srt = new CSLSSrt;
  srt->libsrt_set_fd(fd_client);
  ret = srt->libsrt_getpeeraddr(peer_name, peer_port);
  if (ret != 0) {
    log(LOG_ERRO, "SRT get peer address failed.");
    srt->libsrt_close();
    delete srt;
    return client_count;
  }

  log(LOG_DFLT, "New client (IP %s:%d).", peer_name, peer_port);

  if (0 != srt->libsrt_getsockopt(SRTO_STREAMID, "SRTO_STREAMID", &sid, &sid_size)) {
    log(LOG_ERRO, "\033[1;30m[%s:%d]\033[0m Get stream info failed.", this, peer_name, peer_port, srt->libsrt_get_fd());
    srt->libsrt_close();
    delete srt;
    return client_count;
  }

  if (0 != srt->libsrt_split_sid(sid, host_name, app_name, stream_name)) {
    log(LOG_ERRO, "\033[1;30m[%s:%d]\033[0m Parse stream ID '%s' failed.", peer_name, peer_port, sid);
    srt->libsrt_close();
    delete srt;
    return client_count;
  }
  log(LOG_DFLT, "\033[1;30m[%s:%d]\033[0m Client on stream ID '%s/%s/%s'.",
    peer_name, peer_port, host_name, app_name, stream_name);

  // app exist?
  sprintf(key_app, "%s/%s", host_name, app_name);

  std::string app_uplive = m_map_publisher->get_uplive(key_app);
  sls_conf_app_t *ca = NULL;

  char cur_time[STR_DATE_TIME_LEN] = { 0 };
  get_time_as_string(cur_time);

  //! IS PLAYER?
  if (app_uplive.length() > 0) {
    sprintf(player_stream_id, "%s/%s/%s", host_name, app_name, stream_name);
    sprintf(publisher_stream_id, "%s/%s", app_uplive.c_str(), stream_name);
    CSLSRole *pub = m_map_publisher->get_publisher(publisher_stream_id);
    if (NULL == pub) {
      if (NULL == m_map_puller) {
        log(LOG_DFLT, "\033[1;30m[↓ %s:%d@%s]\033[0m Client refused: no publisher.",
          peer_name, peer_port, player_stream_id);
        srt->libsrt_close();
        delete srt;
        return client_count;
      }
      CSLSRelayManager *puller_manager = m_map_puller->add_relay_manager(app_uplive.c_str(), stream_name);
      if (NULL == puller_manager) {
        log(LOG_DFLT, "\033[1;30m[↓ %s:%d@%s]\033[0m Add Relay Manager failed: no publisher, no player manager.",
          peer_name, peer_port, player_stream_id);
        srt->libsrt_close();
        delete srt;
        return client_count;
      }

      puller_manager->set_map_data(m_map_data);
      puller_manager->set_map_publisher(m_map_publisher);
      puller_manager->set_role_list(m_list_role);
      puller_manager->set_listen_port(m_port);

      if (SLS_OK != puller_manager->start()) {
        log(LOG_DFLT, "\033[1;30m[↓ %s:%d@%s]\033[0m Start player manager failed.",
          peer_name, peer_port, player_stream_id);
        srt->libsrt_close();
        delete srt;
        return client_count;
      }
      log(LOG_DFLT, "\033[1;30m[↓ %s:%d@%s]\033[0m Started player manager.",
        peer_name, peer_port, player_stream_id);

      pub = m_map_publisher->get_publisher(publisher_stream_id);
      if (NULL == pub) {
        log(LOG_DFLT, "\033[1;30m[↓ %s:%d@%s]\033[0m Get publisher failed.",
          peer_name, peer_port, player_stream_id);
        srt->libsrt_close();
        delete srt;
        return client_count;
      }
    }

    //3.2 handle new play
    if (!m_map_data->is_exist(publisher_stream_id)) {
      log(LOG_ERRO, "\033[1;30m[↓ %s:%d@%s]\033[0m Client refused: publisher data doesn't exist.",
        peer_name, peer_port, player_stream_id);
      srt->libsrt_close();
      delete srt;
      return client_count;
    }

    //new player
    if (srt->libsrt_socket_nonblock(0) < 0)
      log(LOG_WARN, "\033[1;30m[↓ %s:%d@%s]\033[0m Set socket options failed.",
        peer_name, peer_port, player_stream_id);

    CSLSPlayer *player = new CSLSPlayer;
    player->init();
    player->set_idle_streams_timeout(m_idle_streams_timeout_role);
    player->set_srt(srt);
    player->set_map_data(publisher_stream_id, m_map_data);
    //stat info
    sprintf(tmp, SLS_PLAYER_STAT_INFO_BASE,
      m_port, player->get_role_name(), app_uplive.c_str(), stream_name, sid, peer_name, peer_port, cur_time);
    std::string stat_info = std::string(tmp);
    player->set_stat_info_base(stat_info);
    player->set_http_url(m_http_url_role);
    player->on_connect();

    m_list_role->push(player);
    log(LOG_DFLT, "\033[1;30m[↓ %s:%d@%s]\033[0m Created player.", peer_name, peer_port, player_stream_id);
    return client_count;
  }

  //! IS PUBLISHER?
  app_uplive = key_app;
  sprintf(publisher_stream_id, "%s/%s", app_uplive.c_str(), stream_name);
  ca = (sls_conf_app_t *)m_map_publisher->get_ca(app_uplive);
  if (NULL == ca) {
    log(LOG_ERRO, "\033[1;30m[↑ %s:%d@%s]\033[0m Client refused: no SLS configuration app info.",
      peer_name, peer_port, publisher_stream_id);
    srt->libsrt_close();
    delete srt;
    return client_count;
  }

  CSLSRole *publisher = m_map_publisher->get_publisher(publisher_stream_id);
  if (NULL != publisher) {
    log(LOG_ERRO, "\033[1;30m[↑ %s:%d@%s]\033[0m Client refused: publisher is not null.",
      peer_name, peer_port, publisher_stream_id);
    srt->libsrt_close();
    delete srt;
    return client_count;
  }
  //create new publisher
  CSLSPublisher *pub = new CSLSPublisher;
  pub->set_srt(srt);
  pub->set_conf(ca);
  pub->init();
  pub->set_idle_streams_timeout(m_idle_streams_timeout_role);
  //stat info
  sprintf(tmp, SLS_PUBLISHER_STAT_INFO_BASE,
    m_port, pub->get_role_name(), app_uplive.c_str(), stream_name, sid, peer_name, peer_port, cur_time);
  std::string stat_info = std::string(tmp);
  pub->set_stat_info_base(stat_info);
  pub->set_http_url(m_http_url_role);
  //set hls record path
  sprintf(tmp, "%s/%s",
    m_record_hls_path_prefix, publisher_stream_id);
  pub->set_record_hls_path(tmp);

  log(LOG_DFLT, "\033[1;30m[↑ %s:%d@%s]\033[0m Publisher created.",
    peer_name, peer_port, publisher_stream_id);

  //init data array
  if (SLS_OK != m_map_data->add(publisher_stream_id)) {
    log(LOG_WARN, "\033[1;30m[↑ %s:%d@%s]\033[0m Add map data failed.",
      peer_name, peer_port, publisher_stream_id);
    pub->uninit();
    delete pub;
    pub = NULL;
    return client_count;
  }

  if (SLS_OK != m_map_publisher->set_push_2_pushlisher(publisher_stream_id, pub)) {
    log(LOG_WARN, "\033[1;30m[↑ %s:%d@%s]\033[0m Push to publisher failed.",
      peer_name, peer_port, publisher_stream_id);
    pub->uninit();
    delete pub;
    pub = NULL;
    return client_count;
  }
  pub->set_map_publisher(m_map_publisher);
  pub->set_map_data(publisher_stream_id, m_map_data);
  pub->on_connect();
  m_list_role->push(pub);

  //5. check pusher
  if (NULL == m_map_pusher) {
    return client_count;
  }
  CSLSRelayManager *pusher_manager = m_map_pusher->add_relay_manager(app_uplive.c_str(), stream_name);
  if (NULL == pusher_manager) {
    log(LOG_DFLT, "\033[1;30m[↑ %s:%d@%s]\033[0m Add Relay Manager failed.",
      peer_name, peer_port, publisher_stream_id);
    return client_count;
  }
  pusher_manager->set_map_data(m_map_data);
  pusher_manager->set_map_publisher(m_map_publisher);
  pusher_manager->set_role_list(m_list_role);
  pusher_manager->set_listen_port(m_port);

  if (SLS_OK != pusher_manager->start()) {
    log(LOG_DFLT, "\033[1;30m[↑ %s:%d@%s]\033[0m Start publisher manager failed.",
      peer_name, peer_port, publisher_stream_id);
  }
  return client_count;
}

std::string   CSLSListener::get_stat_info() {
  if (m_stat_info.length() == 0) {
    char tmp[STR_MAX_LEN] = { 0 };
    char cur_time[STR_DATE_TIME_LEN] = { 0 };
    get_time_as_string(cur_time);
    sprintf(tmp, SLS_SERVER_STAT_INFO_BASE, m_port, m_role_name, cur_time);
    m_stat_info = std::string(tmp);
  }
  return m_stat_info;
}



