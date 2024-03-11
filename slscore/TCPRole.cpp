#include <errno.h>
#include <string.h>

#include "TCPRole.hpp"
#include "SLSLog.hpp"

#define MAX_TCP_SOCK_COUNT 1

/**
 * CTCPRole class implementation
 */

CTCPRole::CTCPRole()
{
    m_fd          = 0;
    m_port        = 0;
    m_remote_port = 0;
    m_valid       = false;

    strcpy(m_remote_host, "");
    sprintf(m_role_name, "tcp_role");
}
CTCPRole::~CTCPRole()
{
    close();
}

int CTCPRole::handler(DATA_PARAM *p)
{
	int ret = 0;
    //log(LOG_DFLT, "CTCPRole::handler()");
	return ret;
}

int CTCPRole::write(const char * buf, int size)
{
	int len = 0;
	len = send(m_fd, buf, size, 0);
	if (0 >= len) {
        log(LOG_DFLT, "[%p]CTCPRole::read, len=%d, errno=%d, err='%s'",
        		this, len, errno, strerror(errno));

	}
    return len;
}

int CTCPRole::read(char * buf, int size)
{
	int len = 0;
	len = recv(m_fd, buf, size, 0);
	if (len <= 0){
        log(LOG_DBUG, "[%p]CTCPRole::read, len=%d, errno=%d, err='%s'.",
        		this, len, errno, strerror(errno));
		if (errno != EAGAIN ) {
	        log(LOG_DFLT, "[%p]CTCPRole::read, invalid tcp.",
	        		this);
			m_valid = false;
		}
	}
    return len;
}

int CTCPRole::open(char *host, int port)
{
	if (SLS_OK != setup())
	{
        log(LOG_ERRO, "[%p]CTCPRole::open setup failure, host='%s', port==%d.", this, host, port);
        return SLS_ERROR;
	}
	if (SLS_OK != connect(host, port))
	{
        log(LOG_ERRO, "[%p]CTCPRole::open setup connect failure, host='%s', port==%d.", this, host, port);
        return SLS_ERROR;
	}
    m_valid = true;
	return SLS_OK;
}

int CTCPRole::connect(char *host, int port)
{
    if (m_fd <= 0) {
        log(LOG_ERRO, "[%p]CTCPRole::connect, m_fd=%d, cant't setup, host='%s', port==%d.", this, m_fd,  host, port);
        return SLS_ERROR;
    }
    int ret = SLS_ERROR;

    //must be nonblock, otherwise, if the host is wrong, connect will be blocked.
    ret = set_nonblock();
    if (ret == SLS_ERROR) {
        log(LOG_ERRO, "[%p]CTCPRole::connect, set_nonblock failure, m_fd=%d.", this, m_fd);
        return SLS_ERROR;
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    //inet_pton(AF_INET, host, &servaddr.sin_addr);
    servaddr.sin_addr.s_addr = inet_addr(host);

    ret = ::connect(m_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if ( ret != 0 )
	{
    	if (errno != EINPROGRESS)
    	{
            log(LOG_ERRO, "[%p]CTCPRole::connect, failure, m_fd=%d, host=%s, port==%d, errno=%d.", this, m_fd, host, port, errno);
            ::close(m_fd);
            m_fd = 0;
            return SLS_ERROR;
    	}
	}

    log(LOG_DFLT, "[%p]CTCPRole::connect, ok, m_fd=%d, host=%s, port==%d.", this, m_fd, host, port);
    strcpy(m_remote_host, host);
    m_remote_port = port;
	return SLS_OK;
}

int CTCPRole::open(int port, int backlog)
{
	if (SLS_OK != setup())
	{
        log(LOG_ERRO, "[%p]CTCPRole::open setup failure, port==%d.", this, port);
        return SLS_ERROR;
	}
	if (SLS_OK != listen(port, backlog))
	{
        log(LOG_ERRO, "[%p]CTCPRole::open listen failure, port==%d.", this, port);
        return SLS_ERROR;
	}
    m_valid = true;
	return SLS_OK;

}

int CTCPRole::setup()
{
    if (m_fd > 0) {
        log(LOG_ERRO, "[%p]CTCPRole::setup, m_fd=%d, cant't setup.", this, m_fd);
        return SLS_ERROR;
    }

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd == 0) {
        log(LOG_ERRO, "[%p]CTCPRole::setup, create sock failure.", this);
        return SLS_ERROR;
    }
    log(LOG_DFLT, "[%p]CTCPRole::setup, create sock ok, m_fd=%d.", this, m_fd);

    int yes = 1;
    int ret = setsockopt(m_fd,
               SOL_SOCKET, SO_REUSEADDR,
               (void *)&yes, sizeof(yes));
    if (ret != 0) {
        log(LOG_ERRO, "[%p]CTCPRole::setup, setsockopt reused failure, m_fd=%d.", this, m_fd);
        return SLS_ERROR;
    }
    log(LOG_DFLT, "[%p]CTCPRole::setup, setsockopt reused ok, m_fd=%d.", this, m_fd);

    return SLS_OK;
}

int CTCPRole::set_nonblock()
{
    if (m_fd <= 0) {
        log(LOG_ERRO, "[%p]CTCPRole::set_nonblock, m_fd=%d, cant't setup.", this, m_fd);
        return SLS_ERROR;
    }
    int opts;
    opts=fcntl(m_fd, F_GETFL);
    if(opts<0)
    {
        log(LOG_ERRO, "[%p]CTCPRole::set_nonblock, fcntl failure, m_fd=%d.", this, m_fd);
        return SLS_ERROR;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(m_fd, F_SETFL, opts)<0)
    {
        log(LOG_ERRO, "[%p]CTCPRole::set_nonblock, fcntl set O_NONBLOCK failure, m_fd=%d.", this, m_fd);
        return SLS_ERROR;
    }
    log(LOG_DFLT, "[%p]CTCPRole::set_nonblock, set O_NONBLOCK ok, m_fd=%d.", this, m_fd);
    return SLS_OK;
}

int CTCPRole::listen(int port, int backlog)
{
    if (m_fd <= 0) {
        log(LOG_ERRO, "[%p]CTCPRole::listen, m_fd=%d, cant't setup.", this, m_fd);
        return SLS_ERROR;
    }
    struct sockaddr_in serverAdd;
    struct sockaddr_in clientAdd;

    bzero(&serverAdd, sizeof(serverAdd));
    serverAdd.sin_family = AF_INET;
    serverAdd.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAdd.sin_port = htons(port);

    socklen_t clientAddrLen;
    int ret = bind(m_fd, (struct sockaddr *)&serverAdd, sizeof(serverAdd));
    if (ret < 0) {
        log(LOG_ERRO, "[%p]CTCPRole::listen, bind failure, m_fd=%d, port=%d.", this, m_fd, port);
        close();
        return SLS_ERROR;
    }
    log(LOG_DFLT, "[%p]CTCPRole::listen, bind ok, m_fd=%d, port=%d.", this, m_fd, port);
    m_port = port;

    ret = ::listen(m_fd, backlog);
    if (ret < 0) {
        log(LOG_ERRO, "[%p]CTCPRole::listen, listen failure, m_fd=%d, port=%d.", this, m_fd, port);
        close();
        return SLS_ERROR;
    }
    log(LOG_DFLT, "[%p]CTCPRole::listen, listen ok, m_fd=%d, port=%d.", this, m_fd, port);
    return SLS_OK;
}

int CTCPRole::close() {
    if (m_fd <= 0) {
        return SLS_ERROR;
    }
    log(LOG_DFLT, "[%p]CTCPRole::close ok, m_fd=%d.", this, m_fd);
    ::close(m_fd);
    m_fd = 0;
    m_valid = false;
    return SLS_OK;
}


char * CTCPRole::get_role_name()
{
    return m_role_name;
}

bool   CTCPRole::is_valid()
{
    return m_valid;
}

int    CTCPRole::get_fd()
{
	return m_fd;
}






