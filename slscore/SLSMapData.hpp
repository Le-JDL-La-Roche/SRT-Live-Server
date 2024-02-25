#ifndef _SLSMapData_INCLUDE_
#define _SLSMapData_INCLUDE_

#include <map>
#include <string>

#include "SLSRecycleArray.hpp"
#include "SLSLock.hpp"

class CSLSMapData
{
public:
    CSLSMapData();
    virtual ~CSLSMapData();

    int  add(char *key);
    int  remove(char *key);
    void clear();

    int  put(char *key, char *data, int len, int64_t *last_read_time=NULL);
    int  get(char *key, char *data, int len, SLSRecycleArrayID *read_id, int aligned=0);

    bool is_exist(char *key);

    int  get_ts_info(char *key, char *data, int len);
private:
    std::map<std::string, CSLSRecycleArray *>    m_map_array;        //uplive_key_stream:data'
    std::map<std::string, ts_info *>             m_map_ts_info;      //uplive_key_stream:ts_info'
    CSLSRWLock          m_rwclock;

    int check_ts_info(char *data, int len, ts_info *ti);
};


#endif
