
#ifndef _HttpRoleList_INCLUDE_
#define _HttpRoleList_INCLUDE_

#include <list>

#include "HttpClient.hpp"
#include "SLSLock.hpp"

/**
 * CHttpRoleList
 */
class CHttpRoleList {
public:
  CHttpRoleList();
  ~CHttpRoleList();

  int          push(CHttpClient *role);
  CHttpClient *pop();
  void         erase();
  int          size();

protected:

private:
  std::list<CHttpClient * > m_list_role;
  CSLSMutex                 m_mutex;
};


#endif
