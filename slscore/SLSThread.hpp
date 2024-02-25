#ifndef _SLSThread_INCLUDE_
#define _SLSThread_INCLUDE_


#include <pthread.h>



/**
 * CSLSThread , the base thread class
 */
class CSLSThread
{
public :
	CSLSThread();
    ~CSLSThread();

    int start();
    int stop();

    bool  is_exit();


    virtual int work();
protected:
	bool m_exit;
	pthread_t m_th_id;

    virtual void    clear();

private:
	static void *thread_func(void *);

};


#endif
