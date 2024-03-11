#include <errno.h>
#include <string.h>

#include "SLSEpollThread.hpp"
#include "SLSLog.hpp"
#include "common.hpp"
#include "SLSRole.hpp"

#include <srt/srt.h>


CSLSEpollThread::CSLSEpollThread() { }

CSLSEpollThread::~CSLSEpollThread() { }

int CSLSEpollThread::init_epoll() {
  int ret = 0;

  m_eid = CSLSSrt::libsrt_epoll_create();
  if (m_eid < 0) {
    log(LOG_DFLT, "Create SRT Epoll failed.");
    return CSLSSrt::libsrt_neterrno();
  }
  //compatible with srt v1.4.0 when container is empty.
  srt_epoll_set(m_eid, SRT_EPOLL_ENABLE_EMPTY);
  return ret;
}

int CSLSEpollThread::uninit_epoll() {
  int ret = 0;
  if (m_eid >= 0) {
    CSLSSrt::libsrt_epoll_release(m_eid);
  }
  return ret;
}

int CSLSEpollThread::work() {
  int ret = 0;
  log(LOG_DFLT, "Epoll thread begin...");
  //epoll loop
  while (!m_exit) {
    handler();
  }

  clear();
  log(LOG_DFLT, "Epoll thread ended.");
  return ret;
}

int CSLSEpollThread::handler() {
  int ret = 0;
  return ret;
}

