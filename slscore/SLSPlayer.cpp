#include <errno.h>
#include <string.h>


#include "SLSPlayer.hpp"
#include "SLSLog.hpp"

/**
 * CSLSPlayer class implementation
 */

CSLSPlayer::CSLSPlayer()
{
    m_is_write = 1;

    sprintf(m_role_name, "player");
}

CSLSPlayer::~CSLSPlayer()
{
}



int CSLSPlayer::handler()
{
    return handler_write_data() ;
}




