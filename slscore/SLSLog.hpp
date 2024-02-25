#ifndef _SLSLOG_INCLUDE_
#define _SLSLOG_INCLUDE_

#include <cstdarg>
#include <stdio.h>

#include "common.hpp"
#include "SLSLock.hpp"

#define LOG_FATL   0
#define LOG_ERRO   1
#define LOG_WARN   2
#define LOG_DFLT   3
#define LOG_DBUG   4
static char const *LOG_LEVEL_NAME[] = {
  "FATAL  ",
  "ERROR  ",
  "WARNING",
  "INFO   ",
  "DEBUG  "
};


static const char APP_NAME[] = "SLS";

#define log             CSLSLog::log_
#define set_log_level   CSLSLog::set_log_level_
#define set_log_file    CSLSLog::set_log_file_
/**
 * CSLSLog
 */
class CSLSLog {
private:
  CSLSLog();
  ~CSLSLog();

public:
  static int  create_instance();
  static int  destroy_instance();
  static void log_(int level, const char *fmt, ...);
  static void set_log_level_(char *level);
  static void set_log_file_(char *file_name);
  static bool m_level_set;
  static bool m_file_set;

private:
  static CSLSLog *m_pInstance;
  int             m_level;
  CSLSMutex       m_mutex;
  FILE *m_log_file;
  char            log_filename[1024];

  void print_log(int level, const char *fmt, va_list vl);
};

#endif
