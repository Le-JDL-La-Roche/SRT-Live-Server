#ifndef _SLSRrt_INCLUDE_
#define _SLSRrt_INCLUDE_

#include <srt/srt.h>

enum SRTMode {
  SRT_MODE_CALLER = 0,
  SRT_MODE_LISTENER = 1,
  SRT_MODE_RENDEZVOUS = 2
};

typedef struct SRTContext {
  int fd;
  int eid;
  int flag;
  int port;
  char hostname[1024];
  int reuse;
  int backlog;

  int64_t rw_timeout;
  int64_t listen_timeout;
  int recv_buffer_size;
  int send_buffer_size;

  int64_t maxbw;
  int pbkeylen;
  char *passphrase;
  int mss;
  int ffs;
  int ipttl;
  int iptos;
  int64_t inputbw;
  int oheadbw;
  int64_t latency;
  int tlpktdrop;
  int nakreport;
  int64_t connect_timeout;
  int payload_size;
  int64_t rcvlatency;
  int64_t peerlatency;
  enum SRTMode mode;
  int sndbuf;
  int rcvbuf;
  int lossmaxttl;
  int minversion;
  char *streamid;
  char *smoother;
  int messageapi;
  SRT_TRANSTYPE transtype;
} SRTContext;

/**
 * CSLSSrt ,functions of srt
 */
class CSLSSrt {
public:
  CSLSSrt();
  ~CSLSSrt();

  static int  libsrt_init();
  static int  libsrt_uninit();
  static int  libsrt_epoll_create();
  static void libsrt_epoll_release(int eid);

  void libsrt_set_context(SRTContext *sc);

  int  libsrt_setup(int port);
  int  libsrt_close();

  int  libsrt_listen(int backlog);
  int  libsrt_accept();

  int  libsrt_get_fd();
  int  libsrt_set_fd(int fd);

  int  libsrt_set_eid(int eid);

  int  libsrt_read(char *buf, int size);
  int  libsrt_write(const char *buf, int size);

  int  libsrt_socket_nonblock(int enable);

  int  libsrt_getsockopt(SRT_SOCKOPT optname, const char *optnamestr, void *optval, int *optlen);
  int  libsrt_setsockopt(SRT_SOCKOPT optname, const char *optnamestr, const void *optval, int optlen);

  int  libsrt_split_sid(char *sid, char *host, char *app, char *name);

  int  libsrt_add_to_epoll(int eid, bool write);
  int  libsrt_remove_from_epoll();

  int  libsrt_getsockstate();
  int  libsrt_getpeeraddr(char *peer_name, int &port);

  void libsrt_set_latency(int latency);


  static int  libsrt_neterrno();
  static void libsrt_print_error_info();


protected:
  SRTContext m_sc;
  char m_peer_name[256];//peer ip addr, such as 172.12.22.14
  int  m_peer_port;

private:
  static bool m_inited;
};


#endif
