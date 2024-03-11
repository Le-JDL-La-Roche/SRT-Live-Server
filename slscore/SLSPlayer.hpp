#ifndef _SLSPlayer_INCLUDE_
#define _SLSPlayer_INCLUDE_

#include "SLSRole.hpp"

/**
 * CSLSPlayer
 */
class CSLSPlayer: public CSLSRole
{
public :
	CSLSPlayer();
	~CSLSPlayer();

    virtual int handler();

private:


};


#endif
