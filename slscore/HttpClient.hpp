#ifndef _HttpClient_INCLUDE_
#define _HttpClient_INCLUDE_

#include <list>

#include "TCPRole.hpp"
#include "SLSArray.hpp"
#include "common.hpp"

enum HTTP_CALLBACK_TYPE {
  HCT_OPEN = 0,
  HCT_CLOSE,
  HCT_RESPONSE_END,
  HCT_REQUEST_CONTENT,
};

#define HTTP_DATA_SIZE             4096
#define SLS_HTTP_INVALID_CLIENT_ID 0
#define HTTP_RESPONSE_CODE_200 "200"

typedef int (*HTTP_CALLBACK)(void *p, HTTP_CALLBACK_TYPE type, void *v, void *context);
struct HTTP_RESPONSE_INFO {
  std::vector<std::string> m_response_header;
  std::string              m_response_code;
  std::string              m_response_content;
  int                      m_response_content_length;
};
/**
 * CHttpClient
 */
class CHttpClient : public CTCPRole {
public:
  CHttpClient();
  virtual ~CHttpClient();

  int         open(const char *url, const char *method = NULL, int interval = 0);
  int         close();
  int         reopen();

  int         send();
  int         recv();
  virtual int handler(DATA_PARAM *p);
  virtual int handler();

  void        set_id(uint32_t client_id);
  void        set_timeout(int v);
  void        set_stage_callback(HTTP_CALLBACK callback, void *context);

  int         check_timeout(int64_t cur_tm_ms = 0);
  int         check_repeat(int64_t cur_tm_ms = 0);
  int         check_finished();

  uint32_t            get_id();
  HTTP_RESPONSE_INFO *get_response_info();
protected:
  uint32_t    m_id;
  char        m_url[URL_MAX_LEN];
  char        m_uri[URL_MAX_LEN];
  char        m_remote_host[URL_MAX_LEN];
  char        m_http_method[32];
  int         m_remote_port;
  int64_t     m_begin_tm_ms;
  int64_t     m_end_tm_ms;
  int         m_timeout;//s
  int         m_interval;//s

  CSLSArray   m_out_array;
  uint8_t     m_out_data[HTTP_DATA_SIZE];
  int         m_out_pos;
  int         m_out_data_len;

  std::string m_request_data;

  HTTP_RESPONSE_INFO       m_response_info;
  /*
std::vector<std::string> m_response_header;
std::string              m_response_code;
std::string              m_response_content;
int                      m_response_content_length;
  */
  HTTP_CALLBACK            m_callback;
  void *m_callback_context;

  int         parse_url();
  int         write_http_header(int data_len);
  int         write_string(std::string *str);
  int         parse_http_response(std::string &response);
  virtual int generate_http_request();
  void        get_request_data();

private:

};

#endif