#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdarg>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>

#include "common.hpp"
#include "SLSLog.hpp"


/**
 * Formats a string.
 * @param format_string The format string.
 * @return The formatted string.
 */
std::string format_string(const char *format_string, ...) {
  std::string str;
  return str;
}

#define HAVE_GETTIMEOFDAY 1

/**
 * Gets the current time in milliseconds.
 * @return The current time in milliseconds.
 */
int64_t get_time_in_milliseconds(void) {
  return get_time() / 1000;
}

/**
 * Gets the current time.
 * @return The current time.
 */
int64_t get_time(void) {
#if HAVE_GETTIMEOFDAY
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#elif HAVE_GETSYSTEMTIMEASFILETIME
  FILETIME ft;
  int64_t t;
  GetSystemTimeAsFileTime(&ft);
  t = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
  return t / 10 - 11644473600000000; /* Jan 1, 1601 */
#else
  return -1;
#endif
}

/**
 * Gets the current time as a string.
 * @param current_time The current time string.
 */
void get_time_as_string(char *current_time) {
  if (NULL == current_time) {
    return;
  }
  int64_t current_time_sec = get_time() / 1000000;
  get_time_formatted(current_time, current_time_sec, "%Y-%m-%d %H:%M:%S");
}

/**
 * Formats the time.
 * @param destination The destination string.
 * @param current_time_sec The current time in seconds.
 * @param format The format string.
 */
void get_time_formatted(char *destination, int64_t current_time_sec, char *format) {
  time_t rawtime;
  struct tm *timeinfo;
  char time_format[32] = { 0 };

  time(&rawtime);
  rawtime = (time_t)current_time_sec;
  timeinfo = localtime(&rawtime);
  strftime(time_format, sizeof(time_format), format, timeinfo);
  strcpy(destination, time_format);
  return;
}

/**
 * Converts a string to uppercase.
 * @param str The string to convert.
 * @return The converted string.
 */
char *string_to_uppercase(char *str) {
  char *original = str;
  for (; *str != '\0'; str++)
    *str = toupper(*str);
  return original;
}

/**
 * Remove leading and trailing whitespace from a string.
 * @param str The string to trim.
 */
char *trim(char *str) {
  char *end;
  while (isspace(*str)) str++;
  if (*str == 0)
    return str;
  end = str + strlen(str) - 1;
  while (end > str && isspace(*end)) end--;
  *(end + 1) = 0;
  return str;
}

#define hash(key, c)   ((uint32_t) key * 31 + c)

/**
 * Hashes a key.
 * @param data The data to hash.
 * @param length The length of the data.
 * @return The hashed key.
 */
uint32_t hash_key(const char *data, int length) {
  uint32_t  i, key;

  key = 0;

  for (i = 0; i < length; i++) {
    key = hash(key, data[i]);
  }
  return key;
}

/**
 * Gets the IP address of a hostname.
 * @param hostname The hostname.
 * @param ip The IP address.
 * @return The status of the operation.
 */
int get_host_by_name(const char *hostname, char *ip) {
  char *ptr, **pptr;
  struct hostent *hptr;
  char   str[32];
  ptr = (char *)hostname;
  int ret = SLS_ERROR;

  if ((hptr = gethostbyname(ptr)) == NULL) {
    printf("Unable to get host by name for host %s.\n", ptr);
    return ret;
  }

  switch (hptr->h_addrtype) {
  case AF_INET:
  case AF_INET6:
    strcpy(ip, inet_ntop(hptr->h_addrtype, hptr->h_addr, str, sizeof(str)));
    ret = SLS_OK;
    break;
  default:
    printf("Unknown address type.\n");
    break;
  }

  return ret;
}

/**
 * Replaces a character in a string.
 * @param buf The string.
 * @param dst The character to replace.
 * @param ch The replacement character.
 */
static void str_replace(char *buf, const char dst, const char ch) {
  char *p = NULL;
  while (1) {
    p = strchr(buf, dst);
    if (p == NULL)
      break;
    *p++ = ch;
  }
}

static size_t max_alloc_size = 1024000;

/**
 * Allocates memory.
 * @param size The size of the memory to allocate.
 * @return The allocated memory.
 */
static void *malloc_av(size_t size) {
  void *ptr = NULL;

  if (size > (max_alloc_size - 32))
    return NULL;

#if HAVE_POSIX_MEMALIGN
  if (size)
    if (posix_memalign(&ptr, ALIGN, size))
      ptr = NULL;
#elif HAVE_ALIGNED_MALLOC
  ptr = _aligned_malloc(size, ALIGN);
#elif HAVE_MEMALIGN
#ifndef __DJGPP__
  ptr = memalign(ALIGN, size);
#else
  ptr = memalign(size, ALIGN);
#endif
#else
  ptr = malloc(size);
#endif

  if (ptr)
    memset(ptr, 0, size);
  return ptr;
}

/**
 * Frees memory.
 * @param arg The memory to free.
 */
static void free_av(void *arg) {
  free(arg);
}

/**
 * Duplicates a string.
 * @param s The string to duplicate.
 * @return The duplicated string.
 */
static char *strdup_av(const char *s) {
  char *ptr = NULL;
  if (s) {
    size_t len = strlen(s) + 1;
    ptr = (char *)malloc_av(len);
    if (ptr)
      memcpy(ptr, s, len);
  }
  return ptr;
}

/**
 * Converts a character to lowercase.
 * @param c The character to convert.
 * @return The converted character.
 */
static inline int tolower_av(int c) {
  if (c >= 'A' && c <= 'Z')
    c ^= 0x20;
  return c;
}

/**
 * Compares two strings case-insensitively.
 * @param a The first string.
 * @param b The second string.
 * @param n The number of characters to compare.
 * @return The result of the comparison.
 */
static int strncasecmp_av(const char *a, const char *b, size_t n) {
  uint8_t c1, c2;
  if (n <= 0)
    return 0;
  do {
    c1 = tolower_av(*a++);
    c2 = tolower_av(*b++);
  } while (--n && c1 && c1 == c2);
  return c1 - c2;
}

/**
 * Creates a directory and all its parent directories.
 * @param path The path of the directory to create.
 * @return The result of the operation.
 */
int create_directory(const char *path) {
  int ret = 0;
  char *temp = strdup_av(path);
  char *pos = temp;
  char tmp_ch = '\0';

  if (!path || !temp) {
    return -1;
  }

  if (!strncasecmp_av(temp, "/", 1) || !strncasecmp_av(temp, "\\", 1)) {
    pos++;
  } else if (!strncasecmp_av(temp, "./", 2) || !strncasecmp_av(temp, ".\\", 2)) {
    pos += 2;
  }

  for (; *pos != '\0'; ++pos) {
    if (*pos == '/' || *pos == '\\') {
      tmp_ch = *pos;
      *pos = '\0';
      ret = mkdir(temp, 0755);
      *pos = tmp_ch;
    }
  }

  if ((*(pos - 1) != '/') || (*(pos - 1) != '\\')) {
    ret = mkdir(temp, 0755);
  }

  free_av(temp);
  return ret;
}

/**
 * Removes quotation marks from a string.
 * @param s The string to remove quotation marks from.
 */
void remove_quotation_marks(char *s) {
  int len = strlen(s);
  if (len < 2)
    return;

  if ((s[0] == '\'' && s[len - 1] == '\'')
    || (s[0] == '"' && s[len - 1] == '"')) {
    for (int i = 0; i < len - 2; i++) {
      s[i] = s[i + 1];
    }
    s[len - 2] = 0x0;
  }
}

static char pid_path_name[] = "/run";
static char pid_file_name[] = "/run/sls.pid";

/**
 * Reads the PID from a file.
 * @return The PID.
 */
int read_pid() {
  struct stat stat_file;
  int ret = stat(pid_file_name, &stat_file);
  if (0 != ret) {
    log(LOG_DFLT, "No PID file '%s'.\n", pid_file_name);
    return 0;
  }

  int fd = open(pid_file_name, O_RDONLY);
  if (0 == fd) {
    log(LOG_WARN, "Open PID file '%s' failed.\n", pid_file_name);
    return 0;
  }
  char pid[128] = { 0 };
  int n = read(fd, pid, sizeof(pid));
  ret = atoi(pid);
  close(fd);
  return ret;
}

/**
 * Writes the PID to a file.
 * @param pid The PID to write.
 * @return The result of the operation.
 */
int write_pid(int pid) {
  struct stat stat_file;
  int fd = 0;

  if (create_directory(pid_path_name) == -1 && errno != EEXIST) {
    log(LOG_FATL, "Write PID file '%s' failed.\n", pid_path_name);
    return -1;
  }
  fd = open(pid_file_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IXOTH);

  if (0 == fd) {
    log(LOG_FATL, "Open file '%s' failed, '%s'.\n", pid_file_name, strerror(errno));
    return -1;
  }
  char buf[128] = { 0 };
  sprintf(buf, "%d", pid);
  write(fd, buf, strlen(buf));
  close(fd);
  log(LOG_DFLT, "PID file written at '%s' (PID %s).\n", pid_file_name, buf);
  return 0;
}

/**
 * Removes the PID file.
 * @return The result of the operation.
 */
int remove_pid() {
  struct stat stat_file;
  if (0 == stat(pid_file_name, &stat_file)) {
    FILE *fd = fopen(pid_file_name, "w");
    fclose(fd);
  }
  return 0;
}

/**
 * Sends a command to the process with the PID read from the PID file.
 * @param cmd The command to send.
 * @return The result of the operation.
 */
int send_cmd(const char *cmd) {
  if (NULL == cmd) {
    printf("Unable to send command, command is null.\n");
    return SLS_ERROR;
  }
  int pid = read_pid();
  if (0 >= pid) {
    printf("Unable to send command, PID is invalid.\n", pid);
    return SLS_OK;
  }

  if (strcmp(cmd, "reload") == 0) {
    printf("Command 'reload' sent, PID %d, send SIGHUP to it.\n", pid);
    kill(pid, SIGHUP);
    return SLS_OK;
  }

  if (strcmp(cmd, "stop") == 0) {
    printf("Command 'stop' sent, PID %d, send SIGINT to it.\n", pid);
    kill(pid, SIGINT);
    return SLS_OK;
  }
  return SLS_OK;
}

/**
 * Splits a string into a vector of substrings.
 * @param str The string to split.
 * @param separator The separator to use for splitting.
 * @param result The vector to store the result in.
 * @param count The maximum number of splits to perform.
 */
void split_string(std::string str, std::string separator, std::vector<std::string> &result, int count) {
  result.clear();
  string::size_type position = str.find(separator);
  string::size_type lastPosition = 0;
  uint32_t separatorLength = (uint32_t)separator.length();

  int i = 0;
  while (position != str.npos) {
    result.push_back(str.substr(lastPosition, position - lastPosition));
    lastPosition = position + separatorLength;
    position = str.find(separator, lastPosition);
    i++;
    if (i == count)
      break;
  }
  result.push_back(str.substr(lastPosition, string::npos));
}

/**
 * Finds a string in a vector of strings.
 * @param src The vector of strings to search in.
 * @param dst The string to search for.
 * @return The found string, or an empty string if not found.
 */
std::string find_string(std::vector<std::string> &src, std::string &dst) {
  std::string ret = std::string("");
  std::vector<std::string>::iterator it;
  for (it = src.begin(); it != src.end();) {
    std::string str = *it;
    it++;
    string::size_type pos = str.find(dst);
    if (pos != std::string::npos) {
      ret = str;
      break;
    }
  }
  return ret;
}

// H264 NAL unit types
enum {
  H264_NAL_UNSPECIFIED = 0,
  H264_NAL_SLICE = 1,
  H264_NAL_DPA = 2,
  H264_NAL_DPB = 3,
  H264_NAL_DPC = 4,
  H264_NAL_IDR_SLICE = 5,
  H264_NAL_SEI = 6,
  H264_NAL_SPS = 7,
  H264_NAL_PPS = 8,
  H264_NAL_AUD = 9,
};

/**
 * Parses the PTS from a PES packet.
 * @param buf The buffer containing the PES packet.
 * @return The parsed PTS.
 */
static int64_t parse_pes_pts(const uint8_t *buf) {
  int64_t pts = 0;
  int64_t tmp = (int64_t)((buf[0] & 0x0e) << 29);
  pts = pts | tmp;
  tmp = (int64_t)((((int64_t)(buf[1] & 0xFF) << 8) | (buf[2]) >> 1) << 15);
  pts = pts | tmp;
  tmp = (int64_t)((((int64_t)(buf[3] & 0xFF) << 8) | (buf[4])) >> 1);
  pts = pts | tmp;
  return pts;
}

/**
 * Parses the SPS and PPS from an H264 stream.
 * @param es The buffer containing the H264 stream.
 * @param es_len The length of the buffer.
 * @param ti The ts_info structure to store the parsed SPS and PPS in.
 * @return The result of the operation.
 */
static int parse_sps_pps(const uint8_t *es, int es_len, ts_info *ti) {
  int ret = SLS_ERROR;
  int pos = 0;
  uint8_t *p = NULL;
  uint8_t *p_end = NULL;
  uint8_t nal_type = 0;
  while (pos < es_len - 4) {
    bool b_nal = false;
    if (0x0 == es[pos] &&
      0x0 == es[pos + 1] &&
      0x0 == es[pos + 2] &&
      (0x1 == es[pos + 3] || (0x0 == es[pos + 3] && 0x1 == es[pos + 4]))) {
      if (p != NULL) {
        p_end = (uint8_t *)es + pos;
        if (H264_NAL_SPS == nal_type) {
          ti->sps_len = p_end - p;
          memcpy(ti->sps, p, ti->sps_len);
        } else if (H264_NAL_PPS == nal_type) {
          ti->pps_len = p_end - p;
          memcpy(ti->pps, p, ti->pps_len);
        } else {
          printf("Unable to parse SPS/PPS (wrong NAL type: %d).\n", nal_type);
        }

        if (ti->sps_len > 0 && ti->pps_len > 0) {
          p = NULL;
          ret = SLS_OK;
          break;
        }
      }
      int nal_pos = pos + (es[pos + 3] ? 4 : 5);
      nal_type = es[nal_pos] & 0x1f;
      if (H264_NAL_SPS == nal_type || H264_NAL_PPS == nal_type) {
        p = (uint8_t *)es + pos;
      }
      pos = nal_pos;
    } else {
      pos++;
    }
  }

  if (p != NULL) {
    p_end = (uint8_t *)es + es_len;
    if (H264_NAL_SPS == nal_type) {
      ti->sps_len = p_end - p;
      memcpy(ti->sps, p, ti->sps_len);
    } else if (H264_NAL_PPS == nal_type) {
      ti->pps_len = p_end - p;
      memcpy(ti->pps, p, ti->pps_len);
    } else {
      printf("Unable to parse SPS/PPS (wrong NAL type: %d).\n", nal_type);
    }
    if (ti->sps_len > 0 && ti->pps_len > 0) {
      ret = SLS_OK;
    }
  }
  return ret;
}

/**
 * Converts a PES frame to an ES frame.
 * @param pes_frame The PES frame to convert.
 * @param pes_len The length of the PES frame.
 * @param ti The ts_info structure to store the converted ES frame in.
 * @param pid The PID of the PES frame.
 * @return The result of the operation.
 */
static int pes_to_es(const uint8_t *pes_frame, int pes_len, ts_info *ti, int pid) {
  if (!pes_frame) {
    printf("Unable to convert PES to ES (PES frame is null).\n");
    return SLS_ERROR;
  }
  uint8_t *pes = (uint8_t *)pes_frame;
  uint8_t *pes_end = (uint8_t *)pes_frame + pes_len;

  if (pes[0] != 0x00 ||
    pes[1] != 0x00 ||
    pes[2] != 0x01) {
    return SLS_ERROR;
  }
  pes += 3;

  int stream_id = (pes[0] & 0xFF);
  if (stream_id != 0xE0 && stream_id != 0xC0) {
    printf("Unable to convert PES to ES (wrong PES stream ID: 0x%x).\n", stream_id);
    return SLS_ERROR;
  }
  pes++;

  int total_size = ((int)(pes[0] << 8)) | pes[1];
  pes += 2;
  if (0 == total_size)
    total_size = MAX_PES_PAYLOAD;
  int flags = 0;
  flags = (pes[0] & 0x7F);
  pes++;
  flags = (pes[0] & 0xFF);
  pes++;

  int header_len = (pes[0] & 0xFF);
  pes++;
  ti->dts = INVALID_DTS_PTS;
  ti->pts = INVALID_DTS_PTS;
  if ((flags & 0xc0) == 0x80) {
    ti->dts = ti->pts = parse_pes_pts(pes);
    pes += 5;
  } else if ((flags & 0xc0) == 0xc0) {
    ti->pts = parse_pes_pts(pes);
    pes += 5;
    ti->dts = parse_pes_pts(pes);
    pes += 5;
  }

  int ret = SLS_OK;
  if (ti->need_spspps) {
    ret = parse_sps_pps(pes, pes_end - pes, ti);
    if (ti->sps_len > 0 && ti->pps_len > 0 && ti->pat_len > 0 && ti->pat_len > 0) {
      uint8_t *p = ti->ts_data;
      int pos = 0;
      uint8_t tmp;

      memcpy(p + pos, ti->pat, TS_PACK_LEN);
      pos += TS_PACK_LEN;
      memcpy(p + pos, ti->pmt, TS_PACK_LEN);
      pos += TS_PACK_LEN;

      int len = ti->sps_len + ti->pps_len;
      len = len + 9 + 5;
      if (len > TS_PACK_LEN - 4) {
        printf("Unable to convert PES to ES (abnormal PES size).\n");
        return ret;
      }
      pos++;
      ti->es_pid = pid;
      tmp = ti->es_pid >> 8;
      p[pos++] = 0x40 | tmp;
      tmp = ti->es_pid;
      p[pos++] = tmp;
      p[pos] = 0x10;
      int ad_len = TS_PACK_LEN - 4 - len - 1;
      if (ad_len > 0) {
        p[pos++] = 0x30;
        p[pos++] = ad_len;
        p[pos++] = 0x00;
        memset(p + pos, 0xFF, ad_len - 1);
        pos += ad_len - 1;
      } else {
        pos++;
      }

      p[pos++] = 0;
      p[pos++] = 0;
      p[pos++] = 1;
      p[pos++] = stream_id;
      p[pos++] = 0; //total size
      p[pos++] = 0; //total size
      p[pos++] = 0x80; //flag
      p[pos++] = 0x80; //flag
      p[pos++] = 5; //header_len
      p[pos++] = 0; //pts
      p[pos++] = 0;
      p[pos++] = 0;
      p[pos++] = 0;
      p[pos++] = 0;
      memcpy(p + pos, ti->sps, ti->sps_len);
      pos += ti->sps_len;
      memcpy(p + pos, ti->pps, ti->pps_len);
      pos += ti->pps_len;
    }
  }
  return ret;
}

/**
 * Parse the Program Association Table (PAT) from the Transport Stream (TS).
 * @param pat_data Pointer to the PAT data.
 * @param len Length of the PAT data.
 * @param ti Pointer to the TS info structure.
 * @return SLS_OK on success, SLS_ERROR on failure.
 */
static int parse_pat(const uint8_t *pat_data, int len, ts_info *ti) {
  uint8_t *buffer = (uint8_t *)pat_data;
  int table_id = buffer[0];
  int section_syntax_indicator = buffer[1] >> 7;
  int zero = buffer[1] >> 6 & 0x1;
  int reserved_1 = buffer[1] >> 4 & 0x3;
  int section_length = (buffer[1] & 0x0F) << 8 | buffer[2];
  int transport_stream_id = buffer[3] << 8 | buffer[4];
  int reserved_2 = buffer[5] >> 6;
  int version_number = buffer[5] >> 1 & 0x1F;
  int current_next_indicator = (buffer[5] << 7) >> 7;
  int section_number = buffer[6];
  int last_section_number = buffer[7];

  int crc_32 = (buffer[len - 4] & 0x000000FF) << 24
    | (buffer[len - 3] & 0x000000FF) << 16
    | (buffer[len - 2] & 0x000000FF) << 8
    | (buffer[len - 1] & 0x000000FF);

  int n = 0;
  for (n = 0; n < section_length - 12; n += 4) {
    unsigned  program_num = buffer[8 + n] << 8 | buffer[9 + n];
    int reserved_3 = buffer[10 + n] >> 5;
    int network_pid = 0x00;
    if (program_num == 0x00) {
      network_pid = (buffer[10 + n] & 0x1F) << 8 | buffer[11 + n];
    } else {
      ti->pmt_pid = (buffer[10 + n] & 0x1F) << 8 | buffer[11 + n];
    }
  }
  return SLS_OK;
}

/**
 * Parse the TS packet info.
 * @param packet Pointer to the TS packet.
 * @param ti Pointer to the TS info structure.
 * @return SLS_OK on success, SLS_ERROR on failure.
 */
int parse_ts_info(const uint8_t *packet, ts_info *ti) {
  if (packet[0] != TS_SYNC_BYTE) {
    printf("Unable to parse TS packet info (packet[0] is 0x%x, 0x47 expected).\n", packet[0]);
    return SLS_ERROR;
  }

  int is_start = packet[1] & 0x40;
  if (0 == is_start) {
    return SLS_ERROR;
  }

  int pid = (int)((packet[1] & 0x1F) << 8) | (packet[2] & 0xFF);
  if (PAT_PID == pid) {
    memcpy(ti->pat, packet, TS_PACK_LEN);
    ti->pat_len = TS_PACK_LEN;
  } else {
    if (ti->pmt_pid == pid) {
      memcpy(ti->pmt, packet, TS_PACK_LEN);
      ti->pmt_len = TS_PACK_LEN;
      return SLS_OK;
    }
    if (INVALID_PID != ti->es_pid) {
      if (pid != ti->es_pid) {
        return SLS_ERROR;
      }
    }
  }

  int afc = (packet[3] >> 4) & 3;
  if (afc == 0)
    return SLS_ERROR;
  int has_adaptation = afc & 2;
  int has_payload = afc & 1;
  bool is_discontinuity = (has_adaptation == 1) &&
    (packet[4] != 0) &&
    ((packet[5] & 0x80) != 0);

  int pos = 4;
  int p = (packet[pos] & 0xFF);
  if (has_adaptation != 0) {
    int64_t pcr_h;
    int pcr_l;
    pos += p + 1;
  }
  if (pos >= TS_PACK_LEN || 1 != has_payload) {
    printf("Unable to parse TS packet info (payload length is %d > 188).\n", pos);
    return SLS_ERROR;
  }

  if (pid == PAT_PID) {
    if (is_start)
      pos++;
    return parse_pat(packet + pos, TS_PACK_LEN - pos, ti);
  }

  int ret = pes_to_es(packet + pos, TS_PACK_LEN - pos, ti, pid);
  if (ti->dts != INVALID_DTS_PTS) {
    ti->es_pid = pid;
  }
  if (ti->sps_len > 0 && ti->pps_len > 0) {
    ti->es_pid = pid;
  }
  return ret;
}

/**
 * Initialize the Transport Stream (TS) info structure.
 * @param ts_info Pointer to the TS info structure.
 */
void init_ts_info(ts_info *ts_info) {
  if (ts_info != NULL) {
    ts_info->es_pid = INVALID_PID;
    ts_info->dts = INVALID_DTS_PTS;
    ts_info->pts = INVALID_DTS_PTS;
    ts_info->sps_len = 0;
    ts_info->pps_len = 0;
    ts_info->pat_len = 0;
    ts_info->pmt_len = 0;
    ts_info->pmt_pid = INVALID_PID;
    ts_info->need_spspps = false;

    memset(ts_info->ts_data, 0, TS_UDP_LEN);

    for (int i = 0; i < TS_UDP_LEN; ) {
      ts_info->ts_data[i] = 0x47;
      ts_info->ts_data[i + 1] = 0x1F;
      ts_info->ts_data[i + 2] = 0xFF;
      ts_info->ts_data[i + 3] = 0x00;
      i += TS_PACK_LEN;
    }
  }
}

/**
 * Get OS name.
 */
char *get_os_name() {
  ifstream stream("/etc/os-release");
  string line;
  regex nameRegex("^PRETTY_NAME=\"(.*?)\"$");
  smatch match;

  string name;
  while (getline(stream, line)) {
    if (regex_search(line, match, nameRegex)) {
      name = match[1].str();
      break;
    }
  }
  return strdup(name.c_str());
}