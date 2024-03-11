#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <strings.h>

#include "SLSLog.hpp"
#include "SLSLock.hpp"

CSLSLog *CSLSLog::m_pInstance = NULL;
bool CSLSLog::m_level_set = false;
bool CSLSLog::m_file_set = false;

CSLSLog::CSLSLog() {
  m_level = LOG_DFLT;
  m_log_file = NULL;
  sprintf(log_filename, "");
}
CSLSLog::~CSLSLog() { }

int CSLSLog::create_instance() {
  if (!m_pInstance) {
    m_pInstance = new CSLSLog();
    m_pInstance->m_level = LOG_DFLT;
    return 0;
  }
  return -1;
}

int CSLSLog::destroy_instance() {
  m_level_set = false;
  m_file_set = false;
  SAFE_DELETE(m_pInstance);
  return 0;
}

void CSLSLog::log_(int level, const char *fmt, ...) {
  if (!m_pInstance)
    m_pInstance = new CSLSLog();
  if (level > m_pInstance->m_level)
    return;

  va_list vl;
  va_start(vl, fmt);
  m_pInstance->print_log(level, fmt, vl);
  va_end(vl);
}


void CSLSLog::print_log(int level, const char *fmt, va_list vl) {
  CSLSLock lock(&m_mutex);
  char buf[4096] = { 0 };
  char buf_info[4096] = { 0 };
  char cur_time[STR_DATE_TIME_LEN] = { 0 };
  int64_t cur_time_sec = get_time_in_milliseconds() / 1000;
  get_time_formatted(cur_time, cur_time_sec, "%Y-%m-%d %H:%M:%S");
  vsnprintf(buf, 4095, fmt, vl);

  const char *color_code;
  const char *color_code_text;
  switch (level) {
  case LOG_FATL:
    color_code = "\033[1;31m"; // Red
    color_code_text = "\033[1;31m";
    break;
  case LOG_ERRO:
    color_code = "\033[1;31m"; // Red
    color_code_text = "\033[0m";
    break;
  case LOG_WARN:
    color_code = "\033[0;33m"; // Yellow
    color_code_text = "\033[0m";
    break;
  case LOG_DFLT:
    color_code = "\033[0m"; // Reset
    color_code_text = "\033[0m";
    break;
  case LOG_DBUG:
    color_code = "\033[1;34m"; // Blue
    color_code_text = "\033[0m";
    break;
  default:
    color_code = "\033[0m"; // Reset
    color_code_text = "\033[0m";
  }

  sprintf(buf_info, "%s %s%s %s%s\033[0m\n", cur_time, color_code, LOG_LEVEL_NAME[level], color_code_text, buf);
  printf(buf_info);

  if (m_log_file) {
    sprintf(buf_info, "%s %s %s\n", cur_time, LOG_LEVEL_NAME[level], buf);
    fwrite(buf_info, strlen(buf_info), 1, m_log_file);
  }
}

void CSLSLog::set_log_level_(char *level) {
  if (!m_pInstance) m_pInstance = new CSLSLog();
  if (m_level_set) return;
  m_level_set = true;

  level = trim(string_to_uppercase(level));
  int n = sizeof(LOG_LEVEL_NAME) / sizeof(char *);
  for (int i = 0; i < n; i++) {
    char *level_name = trim(strdup(LOG_LEVEL_NAME[i]));
    if (strcmp(level, level_name) == 0) {
      m_pInstance->m_level = i;
      log(LOG_DFLT, "Set log level at '%s'.", level_name);
      free(level_name);
      return;
    }
    free(level_name);
  }
  log(LOG_WARN, "Wrong log level '%s'! Set default '%s'.\n", level, LOG_LEVEL_NAME[m_pInstance->m_level]);
}

void CSLSLog::set_log_file_(char *file_name) {
  if (!m_pInstance) m_pInstance = new CSLSLog();
  if (m_file_set) return;
  m_file_set = true;

  if (strlen(m_pInstance->log_filename) == 0) {
    sprintf(m_pInstance->log_filename, "%s", file_name);
    m_pInstance->m_log_file = fopen(m_pInstance->log_filename, "a");
    log(LOG_DFLT, "Set log file at '%s'.\n", file_name);
  }
}

