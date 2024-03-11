#include "HttpRoleList.hpp"

#include <errno.h>
#include <string.h>


#include "SLSLog.hpp"
#include "SLSLock.hpp"

/**
 * CHttpRoleList class implementation
 */

CHttpRoleList::CHttpRoleList() { }
CHttpRoleList::~CHttpRoleList() { }

int CHttpRoleList::push(CHttpClient *role) {
  if (role) {
    CSLSLock lock(&m_mutex);
    m_list_role.push_back(role);
  }
  return 0;
}

CHttpClient *CHttpRoleList::pop() {
  CSLSLock lock(&m_mutex);
  CHttpClient *role = NULL;
  if (!m_list_role.empty()) {
    role = m_list_role.front();
    m_list_role.pop_front();
  }
  return role;
}

int CHttpRoleList::size() {
  CSLSLock lock(&m_mutex);
  return m_list_role.size();
}

void CHttpRoleList::erase() {
  CSLSLock lock(&m_mutex);
  log(LOG_DBUG, "Erasing HTTP Role list (count %d).", this, m_list_role.size());
  for (std::list<CHttpClient * >::iterator it = m_list_role.begin(); it != m_list_role.end();) {
    CHttpClient *role = *it;
    if (role) {
      role->close();
      delete role;
    }
    it++;
  }
  m_list_role.clear();
}

