#ifndef _SLSPusher_INCLUDE_
#define _SLSPusher_INCLUDE_

#include "SLSRelay.hpp"

/**
 * CSLSPuller
 */
class CSLSPusher: public CSLSRelay
{
public :
	CSLSPusher();
    virtual ~CSLSPusher();

    virtual int  handler();
    virtual int  get_stat_base(char *stat_base);
protected:

};


#endif
