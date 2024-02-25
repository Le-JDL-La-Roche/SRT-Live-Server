#include <errno.h>
#include <string.h>


#include "SLSGroup.hpp"
#include "SLSLog.hpp"

#define POLLING_TIME 1 /// Time in milliseconds between interrupt check

/**
 * CSLSGroup class implementation
 */

CSLSGroup::CSLSGroup() {
  m_list_role = NULL;
  m_worker_connections = 100;
  m_worker_number = 0;
  m_reload = false;

  m_stat_post_last_tm_ms = get_time_in_milliseconds();
  m_stat_post_interval = 5;//5s default
}

CSLSGroup::~CSLSGroup() { }

int CSLSGroup::start() {
  return CSLSEpollThread::start();

}

int CSLSGroup::stop() {
  int ret = 0;
  ret = CSLSEpollThread::stop();

  std::list<CSLSRole * >::iterator it_erase;
  for (std::list<CSLSRole * >::iterator it = m_list_wait_http_role.begin(); it != m_list_wait_http_role.end();) {
    CSLSRole *role = *it;
    if (role) {
      role->uninit();
      delete role;
    }
    it++;
  }
  m_list_wait_http_role.clear();
  return ret;
}

void CSLSGroup::reload() {
  m_reload = true;
}

void CSLSGroup::check_new_role() {

  // first, check rolelist
  if (NULL == m_list_role)
    return;
  if (m_map_role.size() >= m_worker_connections)
    return;

  CSLSRole *role = m_list_role->pop();
  if (NULL == role)
    return;

  int fd = role->get_fd();
  if (fd == 0) {
    //invalid role
    delete role;
    return;
  }

  //add to epoll
  if (0 == role->add_to_epoll(m_eid)) {
    m_map_role[fd] = role;
    log(LOG_DFLT, "Checked role (role '%s').", role->get_role_name());
  } else {
    log(LOG_DFLT, "Check role failed (role '%s'): failed to add to epoll.", role->get_role_name());
    delete role;
  }
}

int CSLSGroup::handler() {
  int ret = 0;
  int i;
  int read_len = MAX_SOCK_COUNT;
  int write_len = MAX_SOCK_COUNT;

  int handler_count = 0;

  if (m_reload && (m_map_role.size() == 0)) {
    log(LOG_DFLT, "Stoped worker.");
    m_exit = true;
    return SLS_OK;
  }

  //check epoll event
  ret = srt_epoll_wait(m_eid, m_read_socks, &read_len, m_write_socks, &write_len, POLLING_TIME, 0, 0, 0, 0);
  if (ret < 0) {
    //log(LOG_DBUG, "[%p]CSLSGroup::handle, worker_number=%d, srt_epoll_wait, no epoll event, ret=%d.",
    //        this, m_worker_number, ret);
    ret = srt_getlasterror(NULL);
    if (ret == SRT_ETIMEOUT)//6003
      ret = SLSERROR(EAGAIN);
    else
      ret = CSLSSrt::libsrt_neterrno();

    idle_check();
    return handler_count;
  }

  log(LOG_DBUG, "[%p]CSLSGroup::handle, worker_number=%d, writable sock count=%d, readable sock count=%d.",
    this, m_worker_number, write_len, read_len);

  for (i = 0; i < write_len; i++) {
    std::map<int, CSLSRole *>::iterator it = m_map_role.find(m_write_socks[i]);
    if (it == m_map_role.end()) {
      log(LOG_WARN, "No role map writable (socket %d), why?", m_write_socks[i]);
      continue;
    }

    CSLSRole *role = it->second;
    if (!role) {
      log(LOG_WARN, "Role is null, writable socket %d, why?", m_write_socks[i]);
      continue;
    }

    ret = role->handler();
    if (ret < 0) {
      //handle exception
      log(LOG_DBUG, "[%p]CSLSGroup::handle, worker_number=%d, write sock=%d is invalid, %s=%p, write_len=%d, role_map.size=%d.",
        this, m_worker_number, m_write_socks[i], role->get_role_name(), role, write_len, m_map_role.size());
      role->invalid_srt();
    } else {
      handler_count += ret;
    }
  }

  for (i = 0; i < read_len; i++) {
    std::map<int, CSLSRole *>::iterator it = m_map_role.find(m_read_socks[i]);
    if (it == m_map_role.end()) {
      log(LOG_WARN, "No role map readable (socket %d), why?", m_read_socks[i]);
      continue;
    }

    CSLSRole *role = it->second;
    if (!role) {
      log(LOG_WARN, "Role is null, readable socket %d, why?", m_read_socks[i]);
      continue;
    }

    ret = role->handler();
    if (ret < 0) {
      //handle exception
      log(LOG_DBUG, "[%p]CSLSGroup::handle, worker_number=%d, readable sock=%d is invalid, %s=%p, readable len=%d, role_map.size=%d.",
        this, m_worker_number, m_read_socks[i], role->get_role_name(), role, read_len, m_map_role.size());
      role->invalid_srt();
    } else {
      handler_count += ret;
    }
  }

  idle_check();
  if (0 == handler_count) {
    //release cpu
    msleep(POLLING_TIME);
  }
  return handler_count;
}

void CSLSGroup::idle_check() {
  check_wait_http_role();
  check_reconnect_relay();
  check_invalid_sock();
  check_new_role();
}

void CSLSGroup::check_wait_http_role() {
  std::list<CSLSRole *>::iterator it;
  std::list<CSLSRole *>::iterator it_erase;
  for (it = m_list_wait_http_role.begin(); it != m_list_wait_http_role.end();) {
    CSLSRole *role = *it;
    it_erase = it;
    it++;
    if (!role) {
      m_list_wait_http_role.erase(it_erase);
      continue;
    }
    if (SLS_ERROR == role->check_http_client()) {
      log(LOG_DFLT, "Deleted role '%s' (ID %d).",
        role->get_role_name(), role, m_worker_number);
      role->uninit();
      delete role;
      m_list_wait_http_role.erase(it_erase);
    } else {
      role->handler();
    }
  }
}

void CSLSGroup::check_reconnect_relay() {
  int64_t cur_time_ms = get_time_in_milliseconds();//m_cur_time_microsec;

  CSLSRelayManager *relay_manager = NULL;
  std::list<CSLSRelayManager * >::iterator it_erase;
  std::list<CSLSRelayManager * >::iterator it;
  for (it = m_list_reconnect_relay_manager.begin(); it != m_list_reconnect_relay_manager.end();) {
    CSLSRelayManager *relay_manager = *it;
    if (NULL == relay_manager) {
      log(LOG_DFLT, "Removed invalid Relay Manager.");
      it_erase = it;
      it++;
      m_list_reconnect_relay_manager.erase(it_erase);
      continue;
    }
    int ret = relay_manager->reconnect(cur_time_ms);
    if (SLS_OK != ret) {
      it++;
      continue;
    }
    it_erase = it;
    it++;
    m_list_reconnect_relay_manager.erase(it_erase);
  }
}

void CSLSGroup::check_invalid_sock() {
  bool update_stat_info = false;
  int64_t cur_time_ms = get_time_in_milliseconds();
  int d = cur_time_ms - m_stat_post_last_tm_ms;
  if (d >= m_stat_post_interval * 1000) {
    update_stat_info = true;
    m_stat_info.clear();
    m_stat_post_last_tm_ms = cur_time_ms;
  }

  std::map<int, CSLSRole *>::iterator it;
  std::map<int, CSLSRole *>::iterator it_erase;
  for (it = m_map_role.begin(); it != m_map_role.end();) {
    CSLSRole *role = it->second;
    it_erase = it;
    it++;
    if (!role) {
      m_map_role.erase(it_erase);
      continue;
    }

    if (update_stat_info) {
      std::string stat_info = role->get_stat_info();
      CSLSLock lock(&m_mutex_stat);
      m_stat_info.append(stat_info);
    }

    int state = role->get_state(cur_time_ms);
    if (SLS_RS_INVALID == state || SLS_RS_UNINIT == state) {
      log(LOG_DBUG, "[%p]CSLSGroup::check_invalid_sock, worker_number=%d, %s=%p, invalid sock=%d, state=%d, role_map.size=%d.",
        this, m_worker_number, role->get_role_name(), role, role->get_fd(), state, m_map_role.size());
      //check relay
      if (role->is_reconnect()) {
        CSLSRelay *relay = (CSLSRelay *)role;
        CSLSRelayManager *relay_manager = (CSLSRelayManager *)relay->get_relay_manager();
        m_list_reconnect_relay_manager.push_back(relay_manager);
        log(LOG_DFLT, "Role '%s' (ID %p) needs to reconnect.", role->get_role_name(), role);
      }

      role->uninit();
      if (SLS_OK == role->check_http_client()) {
        m_list_wait_http_role.push_back(role);
        log(LOG_DBUG, "[%p]CSLSGroup::check_invalid_sock, worker_number=%d, %s=%p, put into m_list_wait_http_role.",
          this, m_worker_number, role->get_role_name(), role);
      } else {
        log(LOG_DBUG, "[%p]CSLSGroup::check_invalid_sock, worker_number=%d, %s=%p, delete.",
          this, m_worker_number, role->get_role_name(), role);
        delete role;
      }
      m_map_role.erase(it_erase);
      continue;
    }
  }
}

void CSLSGroup::clear() {
  log(LOG_DBUG, "[%p]CSLSGroup::clear, worker_number=%d, role_map.size=%d.",
    this, m_worker_number, m_map_role.size());
  std::map<int, CSLSRole *>::iterator it;
  for (it = m_map_role.begin(); it != m_map_role.end(); it++) {
    CSLSRole *role = it->second;
    if (role) {
      log(LOG_DFLT, "Deleted role '%s' (ID %p).",
        this, m_worker_number, role->get_role_name(), role);
      role->uninit();
      delete role;
    }
  }
  m_map_role.clear();
}

void CSLSGroup::set_role_list(CSLSRoleList *list_role) {
  m_list_role = list_role;
}

void CSLSGroup::set_worker_number(int n) {
  m_worker_number = n;
}

void CSLSGroup::set_worker_connections(int n) {
  m_worker_connections = n;
}

void CSLSGroup::set_stat_post_interval(int interval) {
  m_stat_post_interval = interval;
}

void CSLSGroup::get_stat_info(std::string &info) {
  CSLSLock lock(&m_mutex_stat);
  info.append(m_stat_info);
}



