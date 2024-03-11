#ifndef _SLSPuller_INCLUDE_
#define _SLSPuller_INCLUDE_

#include "SLSRelay.hpp"

/**
 * CSLSPuller
 */
class CSLSPuller: public CSLSRelay
{
public :
	CSLSPuller();
    virtual ~CSLSPuller();

    virtual int  uninit();
    virtual int  handler();
    virtual int  get_stat_base(char *stat_base);
protected:

};


#endif
