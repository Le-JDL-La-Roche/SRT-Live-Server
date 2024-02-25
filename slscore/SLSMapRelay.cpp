#include "SLSMapRelay.hpp"

#include <errno.h>
#include <string.h>

#include "SLSPullerManager.hpp"
#include "SLSPusherManager.hpp"
#include "SLSLog.hpp"


/**
 * CSLSMapRelay class implementation
 */

CSLSMapRelay::CSLSMapRelay() { }

CSLSMapRelay::~CSLSMapRelay() {
  clear();
}

CSLSRelayManager *CSLSMapRelay::add_relay_manager(const char *app_uplive, const char *stream_name) {
  //find conf info
  SLS_RELAY_INFO *sri = get_relay_conf(std::string(app_uplive));
  if (NULL == sri) {
    log(LOG_DFLT, "No relay configuration info (app publish '%s', stream name '%s').", app_uplive, stream_name);
    return NULL;
  }

  std::string key_stream_name = std::string(app_uplive) + std::string("/") + std::string(stream_name);
  CSLSLock lock(&m_rwclock, true);
  CSLSRelayManager *cur_mananger = NULL;
  std::map<std::string, CSLSRelayManager *>::iterator item;
  item = m_map_relay_manager.find(key_stream_name);
  if (item != m_map_relay_manager.end()) {
    cur_mananger = item->second;
    if (NULL != cur_mananger) {
      log(LOG_DFLT, "Relay Manager already exists (app publish '%s', stream name '%s').",
        app_uplive, stream_name);
      return cur_mananger;
    }
  }

  if (strcmp(sri->m_type, "pull") == 0)
    cur_mananger = new CSLSPullerManager;
  else if (strcmp(sri->m_type, "push") == 0)
    cur_mananger = new CSLSPusherManager;
  else {
    log(LOG_DFLT, "Add Relay Manager failed (app publish '%s', stream name '%s').",
      app_uplive, stream_name);
    return NULL;
  }
  cur_mananger->set_relay_conf(sri);
  cur_mananger->set_relay_info(app_uplive, stream_name);

  m_map_relay_manager[key_stream_name] = cur_mananger;
  log(LOG_DFLT, "Added Relay Manager (app publish '%s', stream name '%s').",
    app_uplive, stream_name);
  return cur_mananger;
}

void CSLSMapRelay::clear() {
  CSLSLock lock(&m_rwclock, true);
  log(LOG_DFLT, "Cleared Map Relay Manager.",
    this);

  std::map<std::string, CSLSRelayManager *>::iterator it;
  for (it = m_map_relay_manager.begin(); it != m_map_relay_manager.end(); ) {
    CSLSRelayManager *relay_manager = it->second;
    if (NULL != relay_manager) {
      delete relay_manager;
    }
    it++;
  }
  m_map_relay_manager.clear();

  std::map<std::string, SLS_RELAY_INFO *>::iterator it_sri;
  for (it_sri = m_map_relay_info.begin(); it_sri != m_map_relay_info.end(); ) {
    SLS_RELAY_INFO *sri = it_sri->second;
    if (NULL != sri) {
      delete sri;
    }
    it_sri++;
  }
  m_map_relay_info.clear();
}

int CSLSMapRelay::add_relay_conf(std::string app_uplive, sls_conf_relay_t *cr) {
  if (NULL == cr) {
    log(LOG_DFLT, "Add app configuration failed: relay configuration is null.");
    return SLS_ERROR;
  }

  SLS_RELAY_INFO *sri = get_relay_conf(app_uplive);
  if (NULL != sri) {
    log(LOG_DFLT, "Add app configuration failed.",
      this, app_uplive.c_str());
    return SLS_ERROR;
  }
  sri = new SLS_RELAY_INFO;
  strcpy(sri->m_type, cr->type);
  sri->m_reconnect_interval = cr->reconnect_interval;
  sri->m_idle_streams_timeout = cr->idle_streams_timeout;

  if (strcmp(cr->mode, "loop") == 0) {
    sri->m_mode = SLS_PM_LOOP;
  } else if (strcmp(cr->mode, "all") == 0) {
    sri->m_mode = SLS_PM_ALL;
  } else if (strcmp(cr->mode, "hash") == 0) {
    sri->m_mode = SLS_PM_HASH;
  } else {
    sri->m_mode = SLS_PM_HASH;
    log(LOG_WARN, "Wrong relay configuration mode ('%s'), use default 'SLS_PM_LOOP'.",
      this, cr->mode);
  }

  //parse upstreams
  sri->m_upstreams = sls_conf_string_split(string(cr->upstreams), string(" "));
  if (sri->m_upstreams.size() == 0) {
    log(LOG_DBUG, "[%p]CSLSMapRelay::add_app_conf, wrong upstreams='%s'.", this, cr->upstreams);
  }
  m_map_relay_info[app_uplive] = sri;
  return SLS_OK;
}

SLS_RELAY_INFO *CSLSMapRelay::get_relay_conf(std::string app_uplive) {
  SLS_RELAY_INFO *sri = NULL;
  std::map<std::string, SLS_RELAY_INFO *>::iterator item;
  item = m_map_relay_info.find(app_uplive);
  if (item != m_map_relay_info.end()) {
    sri = item->second;
  }
  return sri;
}
