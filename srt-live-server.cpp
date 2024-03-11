/** ___  ___  _____   _     _             ___
 * / __|| _ \|_   _| | |   (_)__ __ ___  / __| ___  _ _ __ __ ___  _ _
 * \__ \|   /  | |   | |__ | |\ V // -_) \__ \/ -_)| '_|\ V // -_)| '_|
 * |___/|_|_\  |_|   |____||_| \_/ \___| |___/\___||_|   \_/ \___||_|
 * 
 * 
 * SRT Live Server (SLS) is an open source live streaming server for low latency 
 * based on Secure Reliable Transport (SRT).
 * 
 * 
 * * * * * * * * * * * * * * * *  Original project information  * * * * * * * * * * * * * * *
 * 
 * @author [Edward Wu](https://github.com/Edward-Wu)
 * @link [srt-live-server](https://github.com/Edward-Wu/srt-live-server)
 * @copyright [MIT License](https://github.com/Edward-Wu/srt-live-server/blob/master/LICENSE)
 * 
 * 
 * * * * * * * * * * * * * * * *  Fork information  * * * * * * * * * * * * * * * * * * * * *
 * 
 * @author [Paul Saillant](https://github.com/PaulSaillant)
 * @link [SRT-Live-Server](https://github.com/Le-JDL-La-Roche/SRT-Live-Server)
 * @copyright [GNU GPLv3 License](https://github.com/Le-JDL-La-Roche/SRT-Live-Server/blob/main/LICENSE)
 * 
 * @version 1.4.10
 * - Removed SRT Live Client;
 * - Improved logs and error messages;
 * - Changed HLS behavior.
 */


#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/utsname.h>

using namespace std;

#include "SLSLog.hpp"
#include "SLSManager.hpp"
#include "HttpClient.hpp"

#define SLS_VERSION "1.4.10"

static bool exit_flag = false;
static bool reload_flag = false;

static void handle_sigint(int signal_number) {
  printf("\nCaught signal %d, exiting.\n", signal_number);
  exit_flag = true;
}

static void handle_sighup(int signal_number) {
  printf("\nCaught signal %d, reloading.\n", signal_number);
  reload_flag = true;
}

static void print_usage() {
  printf("SRT Live Server (SLS v%s) running on %s.\n", SLS_VERSION, get_os_name());
  printf("This software is under the GNU General Public License v3.0 (GPLv3).\n\n");
}

static sls_conf_cmd_t conf_cmd_opt[] = {
  SLS_SET_OPT(string, "c", conf_file_name, "<PATH>    Run SLS with with the configuration file", 1, 1023),
  SLS_SET_OPT(string, "r", c_cmd,          "          Reload configuration", 1, 1023),
  SLS_SET_OPT(string, "l", log_level,      "<LEVEL>   Set log level (possible values: fatal/error/warning/info/debug)", 1, 1023),
  SLS_SET_OPT(string, "f", log_file,       "<PATH>    Set log file", 1, 1023),
};

int main(int argc, char *argv[]) {
  struct sigaction    sigIntHandler;
  struct sigaction    sigHupHandler;
  sls_opt_t           sls_opt;

  CSLSManager *sls_manager = NULL;
std:list <CSLSManager *> reload_manager_list;
  CHttpClient *http_stat_client = new CHttpClient;

  int ret = SLS_OK;
  int l = sizeof(sockaddr_in);
  int64_t tm_begin_ms = 0;

  char stat_method[] = "POST";
  sls_conf_srt_t *conf_srt = NULL;

  print_usage();

  memset(&sls_opt, 0, sizeof(sls_opt));

  if (argc > 1) {
    int cmd_size = sizeof(conf_cmd_opt) / sizeof(sls_conf_cmd_t);
    ret = sls_parse_argv(argc, argv, &sls_opt, conf_cmd_opt, cmd_size);
    if (ret != SLS_OK) {
      CSLSLog::destroy_instance();
      return SLS_ERROR;
    }
  }

  if (strcmp(sls_opt.c_cmd, "") != 0) {
    return send_cmd(sls_opt.c_cmd);
  }

  if (strlen(sls_opt.log_level) > 0) {
    set_log_level(sls_opt.log_level);
  }

  if (strlen(sls_opt.log_file) > 0) {
    set_log_file(sls_opt.log_file);
  }

  // Ctrl + C to exit
  sigIntHandler.sa_handler = handle_sigint;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, 0);

  // SIGHUP to reload
  sigHupHandler.sa_handler = handle_sighup;
  sigemptyset(&sigHupHandler.sa_mask);
  sigHupHandler.sa_flags = 0;
  sigaction(SIGHUP, &sigHupHandler, 0);

  CSLSSrt::libsrt_init();

  if (strlen(sls_opt.conf_file_name) == 0) {
    sprintf(sls_opt.conf_file_name, "./sls.conf");
  }
  ret = sls_conf_open(sls_opt.conf_file_name);
  if (ret != SLS_OK) {
    log(LOG_FATL, "Unable to open configuration file, exit!");
    goto EXIT_PROC;
  }

  if (0 != write_pid(getpid())) {
    log(LOG_FATL, "Unable to write PID, exit!");
    goto EXIT_PROC;
  }

  log(LOG_DFLT, "SRT Live Server is running...\n");

  sls_manager = new CSLSManager;
  if (SLS_OK != sls_manager->start()) {
    log(LOG_FATL, "Unable to start SLS Manager, exit!");
    goto EXIT_PROC;
  }

  conf_srt = (sls_conf_srt_t *)sls_conf_get_root_conf();
  if (strlen(conf_srt->stat_post_url) > 0)
    http_stat_client->open(conf_srt->stat_post_url, stat_method, conf_srt->stat_post_interval);

  while (!exit_flag) {
    int64_t cur_tm_ms = get_time_in_milliseconds();
    ret = 0;
    if (sls_manager->is_single_thread()) {
      ret = sls_manager->single_thread_handler();
    }
    if (NULL != http_stat_client) {
      if (!http_stat_client->is_valid()) {
        if (SLS_OK == http_stat_client->check_repeat(cur_tm_ms)) {
          http_stat_client->reopen();
        }
      }
      ret = http_stat_client->handler();
      if (SLS_OK == http_stat_client->check_finished() ||
        SLS_OK == http_stat_client->check_timeout(cur_tm_ms)) {
        http_stat_client->close();
      }

    }

    msleep(10);

    int reload_managers = reload_manager_list.size();
    std::list<CSLSManager *>::iterator it;
    std::list<CSLSManager *>::iterator it_erase;
    for (it = reload_manager_list.begin(); it != reload_manager_list.end();) {
      CSLSManager *manager = *it;
      it_erase = it;
      it++;
      if (NULL == manager) {
        continue;
      }
      if (SLS_OK == manager->check_invalid()) {
        log(LOG_DFLT, "Check reloaded manager, manager %p deleted.", manager);
        manager->stop();
        delete manager;
        reload_manager_list.erase(it_erase);
      }
    }

    if (reload_flag) {
      reload_flag = false;
      log(LOG_DFLT, "Reloading SRT Live Server...");
      ret = sls_manager->reload();

      if (ret != SLS_OK) {
        log(LOG_ERRO, "Reload failed.");
        continue;
      }
      
      reload_manager_list.push_back(sls_manager);
      sls_manager = NULL;
      log(LOG_DFLT, "Reloading, old SLS Manager pushed to list.");

      sls_conf_close();
      ret = sls_conf_open(sls_opt.conf_file_name);
      if (ret != SLS_OK) {
        log(LOG_FATL, "Reload failed, read configuration file failed.");
        break;
      }
      log(LOG_DFLT, "Configuration file reloaded.");
      sls_manager = new CSLSManager;
      if (SLS_OK != sls_manager->start()) {
        log(LOG_FATL, "Reload failed.");
        break;
      }
      if (strlen(conf_srt->stat_post_url) > 0)
        http_stat_client->open(conf_srt->stat_post_url, stat_method, conf_srt->stat_post_interval);
      log(LOG_DFLT, "Reloaded successfully.");
    }
  }

EXIT_PROC:
  printf("\n");
  log(LOG_DFLT, "Stopping SRT Live Server...");

  if (NULL != sls_manager) {
    sls_manager->stop();
    delete sls_manager;
    sls_manager = NULL;
    log(LOG_DFLT, "SLS Manager deleted.");
  }

  std::list<CSLSManager *>::iterator it;
  for (it = reload_manager_list.begin(); it != reload_manager_list.end(); it++) {
    CSLSManager *manager = *it;
    if (NULL == manager) {
      continue;
    }
    manager->stop();
    delete manager;
  }
  log(LOG_DFLT, "Reload Manager list deleted.");
  reload_manager_list.clear();

  if (NULL != http_stat_client) {
    http_stat_client->close();
    delete http_stat_client;
    http_stat_client = NULL;
    log(LOG_DFLT, "HTTP Stat Client deleted.");
  }

  CSLSSrt::libsrt_uninit();
  log(LOG_DFLT, "Uninitialized SRT.");

  sls_conf_close();
  log(LOG_DFLT, "Closed configuration file.");

  remove_pid();

  printf("\n");
  log(LOG_DFLT, "Exit, bye bye!");

  CSLSLog::destroy_instance();

  return 0;
}
