#ifndef _SLSArray_INCLUDE_
#define _SLSArray_INCLUDE_

#include <list>
#include <string.h>

#include "common.hpp"
#include "SLSLock.hpp"

/**
 * CSLSArray
 */
class CSLSArray {
public:
  CSLSArray();
  ~CSLSArray();

public:
  int  put(const uint8_t *data, int len);
  int  get(uint8_t *data, int size);

  void setSize(int n);
  int  count();
  void clear();
private:
  uint8_t *m_arrayData;
  int       m_nDataSize;
  int       m_nDataCount;
  int       m_nWritePos;
  int       m_nReadPos;

  CSLSMutex m_mutex;

  int       get_inline(uint8_t *data, int size);

};



#endif
