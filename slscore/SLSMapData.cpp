#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "SLSMapData.hpp"
#include "SLSLog.hpp"


/**
 * CSLSMapData class implementation
 */

CSLSMapData::CSLSMapData() { }
CSLSMapData::~CSLSMapData() {
  clear();
}

int CSLSMapData::add(char *key) {
  int ret = SLS_OK;
  std::string strKey = std::string(key);

  CSLSLock lock(&m_rwclock, true);

  std::map<std::string, CSLSRecycleArray *>::iterator item;
  item = m_map_array.find(strKey);
  if (item != m_map_array.end()) {
    CSLSRecycleArray *array_data = item->second;
    if (array_data) {
      log(LOG_DFLT, "Add TS data failed: stream ID '%s' and data already exist.", key);
      return ret;
    }
    //m_map_array.erase(item);
  }

  CSLSRecycleArray *data_array = new CSLSRecycleArray;
  //m_map_array.insert(make_pair(strKey, data_array));
  m_map_array[strKey] = data_array;
  log(LOG_DFLT, "Added TS data (stream ID '%s').", key);
  return ret;
}

int CSLSMapData::remove(char *key) {
  int ret = SLS_ERROR;
  std::string strKey = std::string(key);

  CSLSLock lock(&m_rwclock, true);

  std::map<std::string, CSLSRecycleArray *>::iterator item;
  item = m_map_array.find(strKey);
  if (item != m_map_array.end()) {
    CSLSRecycleArray *array_data = item->second;
    log(LOG_DFLT, "Removed TS data (stream ID '%s').", key);
    if (array_data) {
      delete array_data;
    }
    m_map_array.erase(item);
    return SLS_OK;
  }
  return ret;
}

bool CSLSMapData::is_exist(char *key) {

  CSLSLock lock(&m_rwclock, true);
  std::string strKey = std::string(key);

  std::map<std::string, CSLSRecycleArray *>::iterator item;
  item = m_map_array.find(key);
  if (item != m_map_array.end()) {
    CSLSRecycleArray *array_data = item->second;
    if (array_data) {
      log(LOG_DBUG, "[%p]CSLSMapData::is_exist, key=%s, exist.", key);
      return true;
    } else {
      log(LOG_DBUG, "[%p]CSLSMapData::is_exist, is_exist, key=%s, data_array is null.", key);
    }
  } else {
    log(LOG_DBUG, "[%p]CSLSMapData::add, is_exist, key=%s, not exist.", key);
  }
  return false;
}


int CSLSMapData::put(char *key, char *data, int len, int64_t *last_read_time) {
  int ret = SLS_OK;

  CSLSLock lock(&m_rwclock, true);
  std::string strKey = std::string(key);

  std::map<std::string, CSLSRecycleArray *>::iterator item;
  item = m_map_array.find(strKey);
  if (item == m_map_array.end()) {
    log(LOG_ERRO, "Put TS data failed: stream ID '%s' not found.", key);
    return SLS_ERROR;
  }
  CSLSRecycleArray *array_data = item->second;
  if (NULL == array_data) {
    log(LOG_ERRO, "Put TS data failed, no data for stream ID '%s' .", key);
  }

  ret = array_data->put(data, len);
  if (ret != len) {
    log(LOG_ERRO, "Put TS data failed (stream ID '%s').", key, len, ret);
  }
  if (NULL != last_read_time) {
    *last_read_time = array_data->get_last_read_time();
  }

  //check sps and pps
  ts_info *ti = NULL;
  std::map<std::string, ts_info *>::iterator item_ti;
  item_ti = m_map_ts_info.find(strKey);
  if (item_ti == m_map_ts_info.end()) {
    ti = new ts_info;
    init_ts_info(ti);
    ti->need_spspps = true;
    m_map_ts_info[strKey] = ti;
  } else {
    ti = item_ti->second;
  }

  if (SLS_OK == check_ts_info(data, len, ti)) {
    log(LOG_DFLT, "Put TS data (stream ID '%s').", key);
  }

  return ret;
}

int CSLSMapData::get(char *key, char *data, int len, SLSRecycleArrayID *read_id, int aligned) {
  int ret = SLS_OK;

  CSLSLock lock(&m_rwclock, false);
  std::string strKey = std::string(key);

  std::map<std::string, CSLSRecycleArray *>::iterator item;
  item = m_map_array.find(strKey);
  if (item == m_map_array.end()) {
    log(LOG_DBUG, "Get TS data failed: no data for stream ID '%s'.", key);
    return SLS_ERROR;
  }
  CSLSRecycleArray *array_data = item->second;
  if (NULL == array_data) {
    log(LOG_WARN, "Get TS data failed: no data for stream ID '%s'", key);
    return SLS_ERROR;
  }

  bool b_first = read_id->bFirst;
  ret = array_data->get(data, len, read_id, aligned);
  if (b_first) {
    //get sps and pps
    ret = get_ts_info(key, data, len);
    log(LOG_DFLT, "Got TS data.",
      this, key, ret);
  }
  return ret;
}

int CSLSMapData::get_ts_info(char *key, char *data, int len) {
  int ret = 0;
  ts_info *ti = NULL;
  std::string strKey = std::string(key);
  std::map<std::string, ts_info *>::iterator item_ti;
  item_ti = m_map_ts_info.find(strKey);
  if (item_ti != m_map_ts_info.end()) {
    ti = item_ti->second;
    if (len >= TS_UDP_LEN) {
      memcpy(data, ti->ts_data, TS_UDP_LEN);
      ret = TS_UDP_LEN;
    }
  }
  return ret;
}

void CSLSMapData::clear() {
  CSLSLock lock(&m_rwclock, true);
  std::map<std::string, CSLSRecycleArray *>::iterator it;
  for (it = m_map_array.begin(); it != m_map_array.end(); ) {
    CSLSRecycleArray *array_data = it->second;
    if (array_data) {
      delete array_data;
    }
    it++;
  }
  m_map_array.clear();
}

int CSLSMapData::check_ts_info(char *data, int len, ts_info *ti) {
  //only get the first, suppose the sps and pps are not changed always.
  for (int i = 0; i < len;) {
    if (ti->sps_len > 0 && ti->pps_len > 0 && ti->pat_len > 0 && ti->pat_len > 0) {
      break;
    }
    parse_ts_info((const uint8_t *)data + i, ti);
    i += TS_PACK_LEN;
  }

  return SLS_ERROR;
}


