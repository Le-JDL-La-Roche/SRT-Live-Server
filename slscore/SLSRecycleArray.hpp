#ifndef _SLSRecycleArray_INCLUDE_
#define _SLSRecycleArray_INCLUDE_

#include <list>
#include <string.h>

#include "common.hpp"
#include "SLSLock.hpp"

struct SLSRecycleArrayID
{
    int   nReadPos;
    int   nDataCount;
    bool  bFirst;
};


/**
 * CSLSRecycleArray
 */
class CSLSRecycleArray
{
public :
    CSLSRecycleArray();
    ~CSLSRecycleArray();

public :
    int  put(char *data, int len);
    int  get(char *dst, int size, SLSRecycleArrayID *read_id, int aligned=0);

    void setSize(int n);
    int  count();

    int64_t get_last_read_time();
private:
    char     *m_arrayData;
    int       m_nDataSize;
    int       m_nDataCount;
    int       m_nWritePos;
    int64_t   m_last_read_time;


    CSLSRWLock m_rwclock;


};



#endif
