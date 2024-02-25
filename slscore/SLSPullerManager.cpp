#include <errno.h>
#include <string.h>

#include "common.hpp"
#include "SLSPullerManager.hpp"
#include "SLSLog.hpp"
#include "SLSPuller.hpp"

/**
 * CSLSPullerManager class implementation
 */

CSLSPullerManager::CSLSPullerManager()
{
	m_cur_loop_index = -1;
}

CSLSPullerManager::~CSLSPullerManager()
{
}


//start to connect from next of cur index per time.
int CSLSPullerManager::connect_loop()
{
	int ret = SLS_ERROR;

	if (m_sri == NULL || m_sri->m_upstreams.size() == 0) {
	    log(LOG_DFLT, "[%p]CSLSPullerManager::connect_loop, failed, m_upstreams.size()=0, m_app_uplive=%s, m_stream_name=%s.",
	    		this, m_app_uplive, m_stream_name);
		return ret;
	}

	CSLSRelay *puller = NULL;
	if (-1 == m_cur_loop_index) {
		m_cur_loop_index = m_sri->m_upstreams.size() - 1;
	}
	int index = m_cur_loop_index ;
	index ++;

	char szURL[1024] = {0};
	while (true) {
		if (index >= m_sri->m_upstreams.size())
			index = 0;

		const char *szTmp = m_sri->m_upstreams[index].c_str();
		sprintf(szURL, "srt://%s/%s", szTmp, m_stream_name);
		ret = connect(szURL);
		if (SLS_OK == ret) {
			break;
		}
		if (index == m_cur_loop_index) {
		    log(LOG_DFLT, "[%p]CSLSPullerManager::connect_loop, failed, no available pullers, m_app_uplive=%s, m_stream_name=%s.",
		    		this, m_app_uplive, m_stream_name);
			break;
		}
	    log(LOG_DFLT, "[%p]CSLSPullerManager::connect_loop, failed, index=%d, m_app_uplive=%s, m_stream_name=%s, szURL=‘%s’.",
	    		this, m_app_uplive, m_stream_name, szURL);
		index ++;
	}
	m_cur_loop_index = index;
    return ret;
}

int CSLSPullerManager::start()
{
	int ret = SLS_ERROR;

	if (NULL == m_sri) {
	    log(LOG_DFLT, "[%p]CSLSPullerManager::start, failed, m_upstreams.size()=0, m_app_uplive=%s, m_stream_name=%s.",
	    		this, m_app_uplive, m_stream_name);
		return ret;
	}

	//check publisher
	char key_stream_name[1024] = {0};
	sprintf(key_stream_name, "%s/%s", m_app_uplive, m_stream_name);
	if (NULL != m_map_publisher) {
	    CSLSRole * publisher = m_map_publisher->get_publisher(key_stream_name);
	    if (NULL != publisher) {
	        log(LOG_DFLT, "[%p]CSLSPullerManager::start, failed, key_stream_name=%s, publisher=%p exist.",
	    		this, key_stream_name, publisher);
	        return ret;
	    }
	}


	if (SLS_PM_LOOP == m_sri->m_mode) {
		ret = connect_loop();
	} else if (SLS_PM_HASH == m_sri->m_mode) {
		ret = connect_hash();
	} else {
	    log(LOG_DFLT, "[%p]CSLSPullerManager::start, failed, wrong m_sri->m_mode=%d, m_app_uplive=%s, m_stream_name=%s.",
	    		this, m_sri->m_mode, m_app_uplive, m_stream_name);
	}
	return ret;
}

CSLSRelay *CSLSPullerManager::create_relay()
{
     CSLSRelay * relay = new CSLSPuller;
     return relay;
}

int CSLSPullerManager::check_relay_param()
{
	if (NULL == m_role_list) {
		log(LOG_WARN, "[%p]CSLSRelayManager::check_relay_param, failed, m_role_list is null, stream=%s.",
					this, m_stream_name);
		return SLS_ERROR;
	}
	if (NULL == m_map_publisher) {
		log(LOG_WARN, "[%p]CSLSRelayManager::check_relay_param, failed, m_map_publisher is null, stream=%s.",
					this, m_stream_name);
		return SLS_ERROR;
	}
	if (NULL == m_map_data) {
		log(LOG_WARN, "[%p]CSLSRelayManager::check_relay_param, failed, m_map_data is null, stream=%s.",
					this, m_stream_name);
		return SLS_ERROR;
	}
    return SLS_OK;

}

int CSLSPullerManager::set_relay_param(CSLSRelay *relay)
{
	char key_stream_name[1024] = {0};
	sprintf(key_stream_name, "%s/%s", m_app_uplive, m_stream_name);

	if (SLS_OK != check_relay_param()){
		log(LOG_WARN, "[%p]CSLSRelayManager::set_relay_param, check_relay_param failed, stream=%s.",
					this, key_stream_name);
		return SLS_ERROR;
	}

	if (SLS_OK != m_map_publisher->set_push_2_pushlisher(key_stream_name, relay)) {
		log(LOG_WARN, "[%p]CSLSRelayManager::set_relay_param, m_map_publisher->set_push_2_pushlisher, stream=%s.",
					this, key_stream_name);
		return SLS_ERROR;
	}

	if (SLS_OK != m_map_data->add(key_stream_name)) {
		log(LOG_WARN, "[%p]CSLSRelayManager::set_relay_param, m_map_data->add failed, stream=%s, remove from relay=%p, m_map_publisher.",
					this, key_stream_name, relay);
		m_map_publisher->remove(relay);
		return SLS_ERROR;
	}

	relay->set_map_data(key_stream_name, m_map_data);
	relay->set_map_publisher(m_map_publisher);
	relay->set_relay_manager(this);
	m_role_list->push(relay);

	return SLS_OK;

}

int  CSLSPullerManager::add_reconnect_stream(char* relay_url)
{
	m_reconnect_begin_tm = get_time_in_milliseconds();
}

int CSLSPullerManager::reconnect(int64_t cur_tm_ms)
{
	int ret = SLS_ERROR;
	if (cur_tm_ms - m_reconnect_begin_tm < (m_sri->m_reconnect_interval * 1000)) {
		return ret;
	}
	m_reconnect_begin_tm = cur_tm_ms;

    if (SLS_OK != check_relay_param()) {
		log(LOG_WARN, "[%p]CSLSPullerManager::reconnect, check_relay_param failed, stream=%s.",
					this, m_stream_name);
		return SLS_ERROR;
    }

	char key_stream_name[1024] = {0};
	sprintf(key_stream_name, "%s/%s", m_app_uplive, m_stream_name);

	ret = start();
	if (SLS_OK != ret) {
		log(LOG_DFLT, "[%p]CSLSPullerManager::reconnect, start failed, key_stream_name=%s.",
					this, key_stream_name);
	} else {
		log(LOG_DFLT, "[%p]CSLSPullerManager::reconnect, start ok, key_stream_name=%s.",
					this, key_stream_name);
	}
	return ret;
}

