#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "SLSRole.hpp"
#include "SLSLog.hpp"


/**
 * CSLSRole class implementation
 */

CSLSRole::CSLSRole() {
  m_srt = NULL;
  m_is_write = true;//listener: 0, publisher: 0, player: 1
  m_invalid_begin_tm = get_time_in_milliseconds();//
  m_stat_bitrate_last_tm = m_invalid_begin_tm;//
  m_stat_bitrate_interval = 1000;//ms
  m_stat_bitrate_datacount = 0;
  m_kbitrate = 0;//kb
  m_idle_streams_timeout = 10;//unit: s, -1: unlimited
  m_latency = 20;//ms

  m_state = SLS_RS_UNINIT;
  m_back_log = 1024;//maximum number of connections at the same time
  m_port = 0;
  memset(m_peer_ip, 0, IP_MAX_LEN);
  m_peer_port = 0;
  memset(m_role_name, 0, STR_MAX_LEN);
  memset(m_streamid, 0, URL_MAX_LEN);
  memset(m_http_url, 0, URL_MAX_LEN);
  m_http_passed = true;

  m_conf = NULL;
  m_map_data = NULL;
  memset(m_map_data_key, 0, URL_MAX_LEN);
  memset(&m_map_data_id, 0, sizeof(SLSRecycleArrayID));

  memset(m_data, 0, DATA_BUFF_SIZE);
  m_data_len = 0;
  m_data_pos = 0;
  m_need_reconnect = false;
  m_http_client = NULL;

  m_i_ts_file = 0;
  sprintf(m_record_hls, "off");//default off
  m_record_hls_ts_fd = 0;
  memset(m_record_hls_ts_filename, 0, URL_MAX_LEN);
  m_record_hls_index_fd = 0;
  memset(m_record_hls_index_filename, 0, URL_MAX_LEN);
  sprintf(m_record_hls_path, "./index");//default current path
  m_record_hls_begin_tm_ms = 0;
  m_record_hls_segment_duration = 10;//default 10s
  m_record_hls_target_duration = m_record_hls_segment_duration;

  sprintf(m_role_name, "role");
}

CSLSRole::~CSLSRole() {
  uninit();
}

int CSLSRole::init() {
  int ret = 0;
  m_state = SLS_RS_INITED;

  m_map_data_id.bFirst = true;
  m_map_data_id.nDataCount = 0;
  m_map_data_id.nReadPos = 0;

  return ret;
}

int CSLSRole::uninit() {
  int ret = 0;
  if (NULL != m_http_client) {
    m_http_client->close();
    delete m_http_client;
    m_http_client = NULL;
  }

  if (SLS_RS_UNINIT != m_state) {
    m_state = SLS_RS_UNINIT;
    remove_from_epoll();
    invalid_srt();
  }
  close_hls_file();

  return ret;
}

int CSLSRole::invalid_srt() {
  if (m_srt) {
    log(LOG_DFLT, "Invalided SRT, closed socket %d.", get_fd());
    m_srt->libsrt_close();
    delete m_srt;
    m_srt = NULL;

    on_close();
  }
  return SLS_OK;
}

int CSLSRole::get_state(int64_t cur_time_ms) {
  if (SLS_RS_INVALID == m_state)
    return m_state;

  if (check_idle_streams_duration(cur_time_ms)) {
    log(LOG_DBUG, "[%p]CSLSRole::get_state, check_idle_streams_duration is true, cur m_state=%d, m_idle_streams_timeout=%ds, call invalid_srt.",
      this, m_state, m_idle_streams_timeout);
    m_state = SLS_RS_INVALID;
    invalid_srt();
    return m_state;
  }

  int ret = get_sock_state();
  if (SLS_ERROR == ret || SRTS_BROKEN == ret || SRTS_CLOSED == ret || SRTS_NONEXIST == ret) {
    log(LOG_DBUG, "[%p]CSLSRole::get_state, get_sock_state, ret=%d, call invalid_srt.",
      this, ret);
    if (SRTS_BROKEN == ret || SRTS_CLOSED == ret || SRTS_NONEXIST == ret) {
      CSLSSrt::libsrt_neterrno();
    }
    m_state = SLS_RS_INVALID;
    invalid_srt();
    return m_state;
  }
  return m_state;
}

int CSLSRole::handler() {
  int ret = 0;
  return ret;
}

int CSLSRole::get_fd() {
  if (m_srt)
    return m_srt->libsrt_get_fd();
  return 0;
}

int CSLSRole::set_eid(int eid) {
  if (m_srt)
    return m_srt->libsrt_set_eid(eid);
  return 0;
}

int CSLSRole::set_srt(CSLSSrt *srt) {
  if (m_srt) {
    log(LOG_ERRO, "Set SRT failed: SRT is null.");
    return SLS_ERROR;
  }
  m_srt = srt;
  return 0;
}

int CSLSRole::write(const char *buf, int size) {
  if (m_srt)
    return m_srt->libsrt_write(buf, size);
  return SLS_ERROR;
}

int CSLSRole::add_to_epoll(int eid) {
  int ret = SLS_ERROR;
  if (m_srt) {
    m_srt->libsrt_set_eid(eid);
    ret = m_srt->libsrt_add_to_epoll(eid, m_is_write);
    log(LOG_DFLT, "Added to epoll (epoll ID %d).", eid);
  }
  return ret;
}

int CSLSRole::remove_from_epoll() {
  int ret = SLS_ERROR;
  if (m_srt) {
    ret = m_srt->libsrt_remove_from_epoll();
    log(LOG_DFLT, "Removed from epoll.");
  }
  return ret;
}

int CSLSRole::get_sock_state() {
  if (m_srt)
    return m_srt->libsrt_getsockstate();
  return SLS_ERROR;
}

char *CSLSRole::get_role_name() {
  return m_role_name;
}

char *CSLSRole::get_streamid() {
  if (strlen(m_streamid) != 0) {
    return m_streamid;
  }
  char sid[URL_MAX_LEN] = { 0 };
  int  sid_size = sizeof(m_streamid);
  if (m_srt) {
    m_srt->libsrt_getsockopt(SRTO_STREAMID, "SRTO_STREAMID", m_streamid, &sid_size);
  }
  return m_streamid;
}

bool CSLSRole::is_reconnect() {
  return m_need_reconnect;
}

void CSLSRole::set_conf(sls_conf_base_t *conf) {
  m_conf = conf;
}

void CSLSRole::set_map_data(char *map_key, CSLSMapData *map_data) {
  if (NULL != map_key) {
    strcpy(m_map_data_key, map_key);
    m_map_data = map_data;
  } else {
    log(LOG_ERRO, "Set map data failed: map key is null.");
  }
}

void CSLSRole::set_idle_streams_timeout(int timeout) {
  m_idle_streams_timeout = timeout;
}

bool CSLSRole::check_idle_streams_duration(int64_t cur_time_ms) {
  if (-1 == m_idle_streams_timeout) {
    return false;
  }
  if (0 == cur_time_ms) {
    cur_time_ms = get_time_in_milliseconds();
  }
  int duration = cur_time_ms - m_invalid_begin_tm;
  if (duration >= m_idle_streams_timeout * 1000) {
    return true;
  }
  return false;
}

void CSLSRole::set_record_hls_path(const char *hls_path) {
  if (hls_path && strlen(hls_path) > 0) {
    strcpy(m_record_hls_path, hls_path);
  }
}

int CSLSRole::check_http_client() {
  if (NULL == m_http_client) {
    return SLS_ERROR;
  }
  return SLS_OK;
}

void CSLSRole::close_hls_file() {
  ::close(m_record_hls_index_fd);
  m_record_hls_index_fd = 0;
}

void CSLSRole::check_hls_file() {
  char m3u8_info[URL_MAX_LEN] = { 0 };
  int64_t cur_tm_ms = get_time_in_milliseconds();
  if (m_record_hls_begin_tm_ms == 0) {
    m_record_hls_begin_tm_ms = cur_tm_ms;
  }
  float d = cur_tm_ms - m_record_hls_begin_tm_ms;
  d /= 1000;

  if (d < m_record_hls_segment_duration) {
    return;
  }

  m_record_hls_begin_tm_ms = cur_tm_ms;

  if (create_directory(m_record_hls_path) != -1) {
    log(LOG_DBUG, "[%p]CSLSRole::check_hls_file, mkdir '%s' ok.\n", this, m_record_hls_path);
  } else {
    if (errno != EEXIST) {
      log(LOG_ERRO, "Create HLS directory '%s' failed.\n", this, m_record_hls_path);
      return;
    }
    log(LOG_DBUG, "[%p]CSLSRole::check_hls_file, '%s' exist.\n", this, m_record_hls_path);
  }

  //* Close previous TS file
  if (m_record_hls_ts_fd) {
    ::close(m_record_hls_ts_fd);
    m_record_hls_ts_fd = 0;
  }

  //* Create TS file
  char full_ts_name[URL_MAX_LEN] = { 0 };
  sprintf(m_record_hls_ts_filename, "%lld.ts", m_i_ts_file);
  sprintf(full_ts_name, "%s/%s", m_record_hls_path, m_record_hls_ts_filename);
  m_record_hls_ts_fd = ::open(full_ts_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IXOTH);
  log(LOG_DFLT, "Created TS file '%s'.", full_ts_name);
  if (m_record_hls_ts_fd) {
    if (m_map_data) {
      char ts_info[TS_UDP_LEN] = { 0 };
      int re = m_map_data->get_ts_info(m_map_data_key, ts_info, TS_UDP_LEN);
      if (re > 0)
        ::write(m_record_hls_ts_fd, ts_info, re);
    }
  }
  log(LOG_DBUG, "[%p]CSLSRole::check_hls_file, close ts file='%s', fd=%d.", this, m_record_hls_ts_filename, m_record_hls_ts_fd);

  //* Open temp index file if not already open
  if (m_i_ts_file == 0) {
    sprintf(m_record_hls_index_filename, "%s/index.m3u8", m_record_hls_path);
    struct stat stat_file;
    if (0 == stat(m_record_hls_index_filename, &stat_file))
      m_record_hls_index_fd = ::open(m_record_hls_index_filename, O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IXOTH);
    else
      m_record_hls_index_fd = ::open(m_record_hls_index_filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IXOTH);
    sprintf(m3u8_info, "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:%d\n#EXT-X-ENDLIST", (int)(m_record_hls_segment_duration + 1));
    ::write(m_record_hls_index_fd, m3u8_info, strlen(m3u8_info));
    log(LOG_DFLT, "Created HLS index file '%s'.", m_record_hls_index_filename);
  }

  //* Write to temp index file
  char ts_item[URL_MAX_LEN] = { 0 };
  sprintf(ts_item, "#EXTINF:%0.3f,\n%s\n#EXT-X-ENDLIST", d, m_record_hls_ts_filename);
  if (0 != m_record_hls_index_fd) {
    lseek(m_record_hls_index_fd, -14, SEEK_END);
    ::write(m_record_hls_index_fd, ts_item, strlen(ts_item));
  }

  m_i_ts_file++;
}

void CSLSRole::record_data2hls(char *data, int len) {
  check_hls_file();

  if (0 != m_record_hls_ts_fd) {
    ::write(m_record_hls_ts_fd, data, len);
  }
}

int CSLSRole::handler_read_data(int64_t *last_read_time) {
  char szData[TS_UDP_LEN];

  if (SLS_OK != check_http_passed()) {
    return SLS_OK;
  }

  if (NULL == m_srt) {
    log(LOG_ERRO, "Read data failed: SRT is null.");
    return SLS_ERROR;
  }
  //read data
  int n = m_srt->libsrt_read(szData, TS_UDP_LEN);
  if (n <= 0) {
    return SLS_ERROR;
  }

  m_stat_bitrate_datacount += n;
  //update invalid begin time
  m_invalid_begin_tm = get_time_in_milliseconds();
  int d = m_invalid_begin_tm - m_stat_bitrate_last_tm;
  if (d >= m_stat_bitrate_interval) {
    m_kbitrate = m_stat_bitrate_datacount * 8 / d;
    m_stat_bitrate_datacount = 0;
    m_stat_bitrate_last_tm = m_invalid_begin_tm;
  }

  if (n != TS_UDP_LEN) {
    log(LOG_DBUG, "[%p]CSLSRole::handler_read_data, libsrt_read n=%d, expect %d.", this, n, TS_UDP_LEN);
    //return SLS_ERROR;
  }

  if (NULL == m_map_data) {
    log(LOG_ERRO, "Read SRT Data failed: no data handled.");
    return SLS_ERROR;
  }

  log(LOG_DBUG, "[%p]CSLSRole::handler_read_data, ok, libsrt_read n=%d.", this, n);
  int ret = m_map_data->put(m_map_data_key, szData, n, last_read_time);


  //record data
  if (strcmp(m_record_hls, "on") == 0) {
    record_data2hls(szData, n);
  }

  return ret;
}

int CSLSRole::handler_write_data() {
  int ret = 0;
  int write_size = 0;

  if (check_http_passed()) {
    return SLS_OK;
  }

  //read data from publisher's data array
  if (NULL == m_map_data) {
    log(LOG_DFLT, "Write data failed: no SRT data.");
    return SLS_ERROR;
  }
  if (strlen(m_map_data_key) == 0) {
    log(LOG_DFLT, "Write data failed: no SRT data.");
    return SLS_ERROR;
  }

  if (m_data_len < TS_UDP_LEN) {
    ret = m_map_data->get(m_map_data_key, m_data, DATA_BUFF_SIZE, &m_map_data_id, TS_UDP_LEN);
    if (ret < 0) {
      //maybe no publisher, wait for timeout.
      return SLS_OK;
    }
    m_data_pos = 0;
    m_data_len = ret;
  }

  m_stat_bitrate_datacount += ret;
  //update invalid begin time
  m_invalid_begin_tm = get_time_in_milliseconds();
  int d = m_invalid_begin_tm - m_stat_bitrate_last_tm;
  if (d >= m_stat_bitrate_interval) {
    m_kbitrate = m_stat_bitrate_datacount * 8 / d;
    m_stat_bitrate_datacount = 0;
    m_stat_bitrate_last_tm = m_invalid_begin_tm;
  }

  int len = m_data_len - m_data_pos;
  int remainer = m_data_len - m_data_pos;
  while (remainer >= TS_UDP_LEN) {
    ret = write(m_data + m_data_pos, TS_UDP_LEN);
    if (ret < TS_UDP_LEN) {
      log(LOG_DFLT, "Write SRT data failed.");
      break;
    }
    m_data_pos += TS_UDP_LEN;
    write_size += TS_UDP_LEN;
    remainer = m_data_len - m_data_pos;
  }

  if (m_data_pos < m_data_len) {
    log(LOG_DBUG, "[%p]CSLSRole::handler_write_data, write data, len=%d, remainder=%d.", this, len, m_data_len - m_data_pos);
    return SLS_OK;
  }
  if (m_data_pos > m_data_len) {
    log(LOG_WARN, "Written data, data error: data position > data length (%d > %d).", m_data_pos, m_data_len);
  } else {
    //log(LOG_DBUG, "[%p]CSLSRole::handler_write_data, write data, m_data_len=%d.", this, m_data_len);
  }
  m_data_pos = m_data_len = 0;

  return write_size;
}

void CSLSRole::set_stat_info_base(std::string &v) {
  m_stat_info_base = v;
}

std::string CSLSRole::get_stat_info() {
  char tmp[STR_MAX_LEN] = { 0 };
  sprintf(tmp, "\"%d\"}", m_kbitrate);
  return m_stat_info_base + std::string(tmp);
}

int CSLSRole::get_peer_info(char *peer_name, int &peer_port) {
  int ret = SLS_ERROR;
  if (m_srt) {
    ret = m_srt->libsrt_getpeeraddr(peer_name, peer_port);
  }
  return ret;
}

void  CSLSRole::set_http_url(const char *http_url) {
  if (NULL == http_url || strlen(http_url) == 0) {
    return;
  }
  strcpy(m_http_url, http_url);
  if (NULL == m_http_client) {
    m_http_client = new CHttpClient;
    m_http_passed = false;
  }
}

int  CSLSRole::on_connect() {
  if (strlen(m_http_url) == 0) {
    return SLS_ERROR;
  }
  if (NULL == m_http_client) {
    m_http_client = new CHttpClient;
  }

  char on_event_url[URL_MAX_LEN] = { 0 };
  if (strlen(m_peer_ip) == 0) {
    get_peer_info(m_peer_ip, m_peer_port);
  }
  sprintf(on_event_url, "%s?on_event=on_connect&role_name=%s&srt_url=%s&remote_ip=%s&remote_port=%d",
    m_http_url, m_role_name, get_streamid(), m_peer_ip, m_peer_port);

  return m_http_client->open(on_event_url);
}

int  CSLSRole::on_close() {
  if (!m_http_passed) {
    return SLS_OK;
  }
  if (strlen(m_http_url) == 0) {
    return SLS_OK;
  }
  if (NULL == m_http_client) {
    m_http_client = new CHttpClient;
  }

  char on_event_url[URL_MAX_LEN] = { 0 };
  if (strlen(m_peer_ip) == 0) {
    get_peer_info(m_peer_ip, m_peer_port);
  }
  sprintf(on_event_url, "%s?on_event=on_close&role_name=%s&srt_url=%s&remote_ip=%s&remote_port=%d",
    m_http_url, m_role_name, get_streamid(), m_peer_ip, m_peer_port);

  int ret = m_http_client->open(on_event_url);
  return ret;
}

int CSLSRole::check_http_passed() {
  if (m_http_passed)
    return SLS_OK;

  if (!m_http_client) {
    return SLS_OK;
  }

  m_http_client->handler();

  if (SLS_OK != m_http_client->check_finished() && SLS_OK != m_http_client->check_timeout()) {
    return SLS_ERROR;
  }

  HTTP_RESPONSE_INFO *re = m_http_client->get_response_info();
  if (NULL == re) {
    return SLS_ERROR;
  }

  int ret = strcmp(re->m_response_code.c_str(), HTTP_RESPONSE_CODE_200);
  if (0 == ret) {
    log(LOG_DBUG, "[%p]CSLSRole::check_http_client_response, http finished, %s, http_url='%s', response_code=%s, response='%s'.",
      this, m_role_name, m_http_url, re->m_response_code.c_str(), re->m_response_content.c_str());
    m_http_client->close();
    delete m_http_client;
    m_http_client = NULL;
    m_http_passed = true;
    return SLS_OK;
  } else {
    log(LOG_ERRO, "HTTP error: invalid %s url '%s', respond with code %s and message '%s'.",
      this, m_role_name, m_http_url, re->m_response_code.c_str(), re->m_response_content.c_str());
    m_http_client->close();
    delete m_http_client;
    m_http_client = NULL;
    invalid_srt();
    return SLS_ERROR;
  }
}
