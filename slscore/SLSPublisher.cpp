#include <errno.h>
#include <string.h>


#include "SLSPublisher.hpp"
#include "SLSPlayer.hpp"
#include "SLSLog.hpp"

/**
 * app conf
 */
SLS_CONF_DYNAMIC_IMPLEMENT(app)

/**
 * CSLSPublisher class implementation
 */

  CSLSPublisher::CSLSPublisher() {
  m_is_write = 0;
  m_map_publisher = NULL;

  sprintf(m_role_name, "publisher");

}

CSLSPublisher::~CSLSPublisher() { }

int CSLSPublisher::init() {
  int ret = CSLSRole::init();
  if (m_conf) {
    sls_conf_app_t *app_conf = ((sls_conf_app_t *)m_conf);
    //m_exit_delay = ((sls_conf_app_t *)m_conf)->publisher_exit_delay;
    strcpy(m_record_hls, app_conf->record_hls);
    m_record_hls_segment_duration = app_conf->record_hls_segment_duration;
  }

  return ret;
}

int CSLSPublisher::uninit() {
  int ret = SLS_OK;

  if (m_map_data)
    ret = m_map_data->remove(m_map_data_key);


  if (m_map_publisher)
    ret = m_map_publisher->remove(this);

  return CSLSRole::uninit();
}

void CSLSPublisher::set_map_publisher(CSLSMapPublisher *publisher) {
  m_map_publisher = publisher;
}

int CSLSPublisher::handler() {
  return handler_read_data();
}



