#ifndef _COMMON_INCLUDE_
#define _COMMON_INCLUDE_

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <unistd.h>

using namespace std;

#if EDOM > 0
#define SLSERROR(e) (-(e))   // Returns a negative error code from a POSIX error code, to return from library functions.
#define SLSUNERROR(e) (-(e)) // Returns a POSIX error code from a library function error return value.
#else
#define SLSERROR(e) (e)
#define SLSUNERROR(e) (e)
#endif

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define SLSERRTAG(a, b, c, d) (-(int)MKTAG(a, b, c, d))

#define SLS_OK                       SLSERRTAG(0x0,0x0,0x0,0x0 ) // OK
#define SLS_ERROR                    SLSERRTAG(0x0,0x0,0x0,0x1 ) //
#define SLS_ERROR_BSF_NOT_FOUND      SLSERRTAG(0xF8,'B','S','F') // Bitstream filter not found
#define SLS_ERROR_BUG                SLSERRTAG( 'B','U','G','!') // Internal bug, also see SLS_ERROR_BUG2
#define SLS_ERROR_BUFFER_TOO_SMALL   SLSERRTAG( 'B','U','F','S') // Buffer too small
#define SLS_ERROR_EOF                SLSERRTAG( 'E','O','F',' ') // End of file
#define SLS_ERROR_EXIT               SLSERRTAG( 'E','X','I','T') // Immediate exit was requested; the called function should not be restarted
#define SLS_ERROR_EXTERNAL           SLSERRTAG( 'E','X','T',' ') // Generic error in an external library
#define SLS_ERROR_INVALIDDATA        SLSERRTAG( 'I','N','D','A') // Invalid data found when processing input
#define SLS_ERROR_OPTION_NOT_FOUND   SLSERRTAG(0xF8,'O','P','T') // Option not found
#define SLS_ERROR_PROTOCOL_NOT_FOUND SLSERRTAG(0xF8,'P','R','O') // Protocol not found
#define SLS_ERROR_STREAM_NOT_FOUND   SLSERRTAG(0xF8,'S','T','R') // Stream of the StreamID not found
#define SLS_ERROR_UNKNOWN            SLSERRTAG( 'U','N','K','N') // Unknown error, typically from an external library
#define SLS_ERROR_INVALID_SOCK       SLSERRTAG( 'I','N','V','S') // Unknown error, typically from an external library

#define SAFE_DELETE(p) {if (p) { delete p; p = NULL; }}
#define msleep(ms) usleep(ms*1000)

#define TS_PACK_LEN 188
#define TS_UDP_LEN 1316 //7*188
#define SHORT_STR_MAX_LEN 256
#define STR_MAX_LEN 1024
#define URL_MAX_LEN STR_MAX_LEN
#define STR_DATE_TIME_LEN 32
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
#define IP_MAX_LEN INET6_ADDRSTRLEN

int64_t get_time_in_milliseconds(void);
int64_t get_time(void);
void    get_time_formatted(char *destination, int64_t current_time_sec, char *format);
void    get_time_as_string(char *current_time);
char *string_to_uppercase(char *str);
char *trim(char *str);
void    remove_quotation_marks(char *str);

uint32_t hash_key(const char *data, int length);
int      get_host_by_name(const char *hostname, char *ip);
int      create_directory(const char *path);

int read_pid();
int write_pid(int pid);
int remove_pid();
int send_cmd(const char *cmd);


void split_string(std::string str, std::string separator, std::vector<std::string> &result, int count = -1);
std::string find_string(std::vector<std::string> &src, std::string &dst);

char *get_os_name();

#define TS_SYNC_BYTE 0x47
#define TS_PACK_LEN 188
#define INVALID_PID -1
#define PAT_PID 0
#define INVALID_DTS_PTS -1
#define MAX_PES_PAYLOAD 200 * 1024

struct ts_info {
  int      es_pid;
  int64_t  dts;
  int64_t  pts;
  bool     need_spspps;
  int      sps_len;
  uint8_t  sps[TS_PACK_LEN];
  int      pps_len;
  uint8_t  pps[TS_PACK_LEN];
  uint8_t  ts_data[TS_UDP_LEN];
  uint8_t  pat[TS_PACK_LEN];
  int      pat_len;
  int      pmt_pid;
  uint8_t  pmt[TS_PACK_LEN];
  int      pmt_len;

};
void init_ts_info(ts_info *ti);
int  parse_ts_info(const uint8_t *packet, ts_info *ti);

#endif
