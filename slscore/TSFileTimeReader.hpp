#ifndef _TSFileTimeReader_INCLUDE_
#define _TSFileTimeReader_INCLUDE_

#include <string.h>

#include "common.hpp"
#include "SLSArray.hpp"
#include "SLSSyncClock.hpp"

/**
 * CTSFileTimeReader
 */
class CTSFileTimeReader {
public:
  CTSFileTimeReader();
  ~CTSFileTimeReader();

public:
  int  open(const char *ts_file_name, bool loop);
  int  close();
  int  get(uint8_t *data, int size, int64_t &tm_ms, bool &jitter);

  int64_t  generate_rts_file(const char *ts_file_name);

private:
  char            m_file_name[URL_MAX_LEN];
  int             m_rts_fd;
  int             m_dts_pid;
  int64_t         m_dts;
  int64_t         m_pts;
  bool            m_loop;
  CSLSArray       m_array_data;
  int64_t         m_udp_duration;
  int64_t         m_readed_count;

};



#endif
