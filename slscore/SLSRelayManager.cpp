#include <errno.h>
#include <string.h>

#include "common.hpp"
#include "SLSRelayManager.hpp"
#include "SLSLog.hpp"


/**
 * CSLSRelayManager class implementation
 */
CSLSRelayManager::CSLSRelayManager() {
	m_reconnect_begin_tm = 0;
	m_map_publisher = NULL;
	m_map_data = NULL;
	m_role_list = NULL;
	m_sri = NULL;
	m_listen_port = 0;

	memset(m_app_uplive, 0, sizeof(m_app_uplive));
	memset(m_stream_name, 0, sizeof(m_stream_name));

}

CSLSRelayManager::~CSLSRelayManager() { }

void CSLSRelayManager::set_map_publisher(CSLSMapPublisher *map_publisher) {
	m_map_publisher = map_publisher;
}

void CSLSRelayManager::set_map_data(CSLSMapData *map_data) {
	m_map_data = map_data;
}

void CSLSRelayManager::set_role_list(CSLSRoleList *role_list) {
	m_role_list = role_list;
}

void CSLSRelayManager::set_relay_conf(SLS_RELAY_INFO *sri) {
	m_sri = sri;
}

void CSLSRelayManager::set_relay_info(const char *app_uplive, const char *stream_name) {
	strcpy(m_app_uplive, app_uplive);
	strcpy(m_stream_name, stream_name);
}

void CSLSRelayManager::set_listen_port(int port) {
	m_listen_port = port;
}

int CSLSRelayManager::connect(const char *url) {
	int ret = SLS_ERROR;
	if (url == NULL || strlen(url) == 0) {
		log(LOG_DFLT, "[%p]CSLSManager::connect, failed, url=%s.", url ? url : "null");
		return ret;
	}

	CSLSRelay *cur_relay = create_relay();//new relay;
	cur_relay->init();
	ret = cur_relay->open(url);
	if (SLS_OK == ret) {
		cur_relay->set_idle_streams_timeout(m_sri->m_idle_streams_timeout);

		//set stat info
		char tmp[URL_MAX_LEN] = { 0 };
		char stat_base[URL_MAX_LEN] = { 0 };
		char cur_time[STR_DATE_TIME_LEN] = { 0 };
		get_time_as_string(cur_time);
		char relay_peer_name[IP_MAX_LEN] = { 0 };
		int  relay_peer_port = 0;
		cur_relay->get_peer_info(relay_peer_name, relay_peer_port);
		cur_relay->get_stat_base(stat_base);
		sprintf(tmp, stat_base,
			m_listen_port, cur_relay->get_role_name(), m_app_uplive, m_stream_name, url, relay_peer_name, relay_peer_port, cur_time);
		std::string stat_info = std::string(tmp);
		cur_relay->set_stat_info_base(stat_info);

		ret = set_relay_param(cur_relay);
		if (SLS_OK != ret) {
			cur_relay->uninit();
			delete cur_relay;
			cur_relay = NULL;
		}
		return ret;
	} else {
		cur_relay->uninit();
		delete cur_relay;
		cur_relay = NULL;
	}
	return ret;
}

int CSLSRelayManager::connect_hash() {
	//make hash to hostnames by stream_name
	std::string url = get_hash_url();
	char szURL[1024] = { 0 };
	const char *szTmp = url.c_str();
	sprintf(szURL, "srt://%s/%s", szTmp, m_stream_name);
	int ret = connect(szURL);
	if (SLS_OK != ret) {
		log(LOG_DFLT, "[%p]CSLSRelayManager::connect_hash, faild, connect szURL=%s, m_stream_name=%s.",
			this, szURL, m_stream_name);
	} else {
		log(LOG_DFLT, "[%p]CSLSRelayManager::connect_hash, ok, connect szURL=%s, m_stream_name=%s.",
			this, szURL, m_stream_name);
	}
	return ret;
}


std::string CSLSRelayManager::get_hash_url() {
	if (NULL == m_sri) {
		return "";
	}
	uint32_t key = hash_key(m_stream_name, strlen(m_stream_name));
	uint32_t index = key % m_sri->m_upstreams.size();
	return m_sri->m_upstreams[index];

}
