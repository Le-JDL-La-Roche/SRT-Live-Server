#include <errno.h>
#include <string.h>


#include "SLSRoleList.hpp"
#include "SLSLog.hpp"
#include "SLSLock.hpp"

/**
 * CSLSRoleList class implementation
 */

CSLSRoleList::CSLSRoleList() { }
CSLSRoleList::~CSLSRoleList() { }

int CSLSRoleList::push(CSLSRole *role) {
  if (role) {
    CSLSLock lock(&m_mutex);
    m_list_role.push_back(role);
  }
  return 0;
}

CSLSRole *CSLSRoleList::pop() {
  CSLSLock lock(&m_mutex);
  CSLSRole *role = NULL;
  if (!m_list_role.empty()) {
    role = m_list_role.front();
    m_list_role.pop_front();
  }
  return role;
}

void CSLSRoleList::erase() {
  CSLSLock lock(&m_mutex);
  log(LOG_DBUG, "[%d]CSLSRoleList::erase count=%d.", this, m_list_role.size());
  std::list<CSLSRole * >::iterator it_erase;
  for (std::list<CSLSRole * >::iterator it = m_list_role.begin(); it != m_list_role.end();) {
    CSLSRole *role = *it;
    if (role) {
      role->uninit();
      delete role;
    }
    it++;
  }
  m_list_role.clear();
}

int CSLSRoleList::size() {
  CSLSLock lock(&m_mutex);
  return m_list_role.size();
}




