#ifndef _SLSRoleList_INCLUDE_
#define _SLSRoleList_INCLUDE_

#include <list>

#include "SLSRole.hpp"
#include "SLSLock.hpp"

/**
 * CSLSRoleList
 */
class CSLSRoleList
{
public :
	CSLSRoleList();
    ~CSLSRoleList();

    int       push(CSLSRole *role);
    CSLSRole *pop();
    void      erase();
    int       size();

protected:

private:
    std::list<CSLSRole * > m_list_role;

    CSLSMutex m_mutex;
};


#endif
