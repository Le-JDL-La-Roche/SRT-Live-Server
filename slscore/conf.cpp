#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "conf.hpp"
#include "common.hpp"
#include "SLSLog.hpp"


sls_conf_base_t  sls_first_conf = { "", NULL, NULL };
sls_runtime_conf_t *sls_runtime_conf_t::first = NULL;

sls_runtime_conf_t::sls_runtime_conf_t(char *c, create_conf_func f, sls_conf_cmd_t *cmd, int len) {
  conf_name = c;
  create_fn = f;
  conf_cmd = cmd;
  conf_cmd_size = len;
  next = NULL;

  this->next = first;
  first = this;
}

sls_conf_cmd_t *sls_conf_find(const char *n, sls_conf_cmd_t *cmd, int size) {
  for (int i = 0; i < size; i++) {
    if (strcmp(n, cmd->name) == 0) {
      return cmd;
    }
    cmd++;
  }
  return NULL;
}

const char *sls_conf_set_int(const char *v, sls_conf_cmd_t *cmd, void *conf) {
  char *p = (char *)conf;
  int     v1;
  int *np;
  char *value;

  np = (int *)(p + cmd->offset);

  v1 = atoi(v);
  if (v1 < cmd->min || v1 > cmd->max)
    return SLS_CONF_OUT_RANGE;
  *np = v1;
  return SLS_CONF_OK;
}

const char *sls_conf_set_string(const char *v, sls_conf_cmd_t *cmd, void *conf) {
  char *p = (char *)conf;
  char *np;
  int     len = strlen(v);

  if (len < cmd->min || len > cmd->max)
    return SLS_CONF_OUT_RANGE;

  np = (char *)(p + cmd->offset);
  memcpy(np, v, len);
  np[len] = 0;
  return SLS_CONF_OK;
}

const char *sls_conf_set_double(const char *v, sls_conf_cmd_t *cmd, void *conf) {
  char *p = (char *)conf;
  double   v1;
  double *np;
  char *value;

  np = (double *)(p + cmd->offset);

  v1 = atof(v);
  if (v1 < cmd->min || v1 > cmd->max)
    return SLS_CONF_OUT_RANGE;
  *np = v1;
  return SLS_CONF_OK;
}

const char *sls_conf_set_bool(const char *v, sls_conf_cmd_t *cmd, void *conf) {
  char *p = (char *)conf;
  bool *np;
  char *value;

  np = (bool *)(p + cmd->offset);

  if (0 == strcmp(v, "true")) {
    *np = true;
    return SLS_CONF_OK;
  } else if (0 == strcmp(v, "false")) {
    *np = false;
    return SLS_CONF_OK;
  } else {
    return SLS_CONF_WRONG_TYPE;
  }
}


int sls_conf_get_conf_count(sls_conf_base_t *c) {
  int count = 0;
  while (c) {
    c = c->sibling;
    count++;
  }
  return count;
}

vector<string> sls_conf_string_split(const string &str, const string &delim) {
  vector<string> res;
  if ("" == str) return res;

  char *strs = new char[str.length() + 1];
  strcpy(strs, str.c_str());

  char *d = new char[delim.length() + 1];
  strcpy(d, delim.c_str());

  char *p = strtok(strs, d);
  while (p) {
    string s = p;
    res.push_back(s);
    p = strtok(NULL, d);
  }

  delete[] strs;
  delete[] d;
  return res;
}

string &trim(string &s) {
  if (s.empty()) {
    return s;
  }
  s.erase(0, s.find_first_not_of(" "));
  s.erase(s.find_last_not_of(" ") + 1);
  return s;
}

string &replace_all(string &str, const string &old_value, const string &new_value) {
  while (true) {
    string::size_type pos(0);
    if ((pos = str.find(old_value)) != string::npos)
      str.replace(pos, old_value.length(), new_value);
    else
      break;
  }
  return  str;
}

sls_conf_base_t *sls_conf_create_block_by_name(string n, sls_runtime_conf_t *&p_runtime) {
  sls_conf_base_t *p = NULL;
  p_runtime = sls_runtime_conf_t::first;
  while (p_runtime) {
    if (strcmp(n.c_str(), p_runtime->conf_name) == 0) {
      p = p_runtime->create_fn();
      //sls_add_conf_to_runtime(p, p_runtime);
      break;
    }
    p_runtime = p_runtime->next;
  }
  return p;

}

int sls_conf_parse_block(ifstream &ifs, int &line, sls_conf_base_t *b, bool &child, sls_runtime_conf_t *p_runtime, int brackets_layers) {
  int ret = SLS_ERROR;
  sls_conf_base_t *block = NULL;
  string str_line, str_line_last;
  string n, v, line_end_flag;
  int index;

  while (getline(ifs, str_line)) {
    line++;
    index = str_line.find('#');

    if (index != -1) {
      str_line = str_line.substr(0, index);
    }

    str_line = replace_all(str_line, "\t", "");
    str_line = trim(str_line);

    if (str_line.length() == 0) {
      log(LOG_DBUG, "Line %d is a comment.", line, str_line.c_str());
      continue;
    }

    line_end_flag = str_line.substr(str_line.length() - 1);

    if (line_end_flag == ";") {
      if (!b) {
        log(LOG_ERRO, "Line %d, block not found.", line, str_line.c_str());
        ret = SLS_ERROR;
        break;
      }

      str_line = str_line.substr(0, str_line.length() - 1);

      str_line = replace_all(str_line, "\t", "");
      str_line = trim(str_line);

      int index = str_line.find(' ');
      if (index == -1) {
        log(LOG_ERRO, "Line %d, no space separator.", line, str_line.c_str());
        ret = SLS_ERROR;
        break;
      }
      n = str_line.substr(0, index);
      v = str_line.substr(index + 1, str_line.length() - (index + 1));
      v = trim(v);

      sls_conf_cmd_t *it = sls_conf_find(n.c_str(), p_runtime->conf_cmd, p_runtime->conf_cmd_size);
      if (!it) {
        log(LOG_ERRO, "Line %d, wrong name='%s'.", line, str_line.c_str(), n.c_str());
        ret = SLS_ERROR;
        break;
      }

      const char *r = it->set(v.c_str(), it, b);
      if (r != SLS_CONF_OK) {
        log(LOG_ERRO, "Line %d, set failed, %s, name='%s', value='%s'.", line, r, n.c_str(), v.c_str());
        ret = SLS_ERROR;
        break;
      }

      log(LOG_DBUG, "Line %d, set name='%s', value='%s'.", line, n.c_str(), v.c_str());

    } else if (line_end_flag == "{") {
      str_line = str_line.substr(0, str_line.length() - 1);
      str_line = replace_all(str_line, "\t", "");
      str_line = trim(str_line);

      n = str_line;
      if (n.length() == 0) {
        if (str_line_last.length() == 0) {
          log(LOG_ERRO, "Line %d, no name found.", line);
          ret = SLS_ERROR;
          break;
        }
        n = str_line_last;
        str_line_last = "";
      }

      block = sls_conf_create_block_by_name(n, p_runtime);
      if (!block) {
        log(LOG_ERRO, "Line %d, name='%s' not found.", line, n.c_str());
        ret = SLS_ERROR;
        break;
      }
      if (child) b->child = block;
      else b->sibling = block;
      b = block;
      child = true;
      brackets_layers++;
      ret = sls_conf_parse_block(ifs, line, b, child, p_runtime, brackets_layers);
      if (ret != SLS_OK) {
        log(LOG_ERRO, "Line %d, parse block='%s' failed.", line, block->name);
        ret = SLS_ERROR;
        break;
      }
    } else if (line_end_flag == "}") {
      if (str_line != line_end_flag) {
        log(LOG_ERRO, "Line %d, end indicator '}' with more info.", line);
        ret = SLS_ERROR;
        break;
      }
      brackets_layers--;
      ret = SLS_OK;
      child = false;
      break;
    } else {
      log(LOG_ERRO, "Line %d, invalid end flag, except ';', '{', '}'.", line);
      ret = SLS_ERROR;
      break;
    }
    str_line_last = str_line;
  }
  return ret;

}

int sls_conf_open(const char *conf_file) {
  ifstream    ifs(conf_file);
  int         ret = 0;
  int         line = 0;
  bool        child = true;
  int         brackets_layers = 0;

  sls_runtime_conf_t *p_runtime = NULL;

  // printf("+----------------------------------------+\n");
  // printf("|                                        |\n");
  // printf("|            SRT Live Server             |\n");
  // printf("|                                        |\n");
  // printf("+----------------------------------------+\n\n");

  printf("     ___  ___  _____   _     _             ___                          \n");
  printf("    / __|| _ \\|_   _| | |   (_)__ __ ___  / __| ___  _ _ __ __ ___  _ _ \n");
  printf("    \\__ \\|   /  | |   | |__ | |\\ V // -_) \\__ \\/ -_)| '_|\\ V // -_)| '_|\n");
  printf("    |___/|_|_\\  |_|   |____||_| \\_/ \\___| |___/\\___||_|   \\_/ \\___||_|  \n");
  printf("                                                                  \n\n");

  log(LOG_DFLT, "Parsing configuration file '%s'.", conf_file);
  if (!ifs.is_open()) {
    log(LOG_FATL, "Open configuration file failed. Please check if the file exists.", conf_file);
    return SLS_ERROR;
  }

  ret = sls_conf_parse_block(ifs, line, &sls_first_conf, child, p_runtime, brackets_layers);
  if (ret != SLS_OK) {
    if (0 == brackets_layers) {
      log(LOG_FATL, "Parse configuration file failed.", conf_file);
    } else {
      log(LOG_FATL, "Parse configuration file failed, please check number of '{' and '}'.", conf_file);
    }
  }
  return ret;
}

void sls_conf_release(sls_conf_base_t *c) {
  sls_conf_base_t *c_b;
  if (c->child != NULL) {
    c_b = c->child;
    sls_conf_release(c_b);
    c->child = NULL;
  }
  if (c != NULL && c->sibling != NULL) {
    c_b = c->sibling;
    sls_conf_release(c_b);
    c->sibling = NULL;
  }

  if (c->child == NULL && c->sibling == NULL) {
    delete c;
    log(LOG_DBUG, "sls_conf_release Deleted '%s'.", c->name);
    return;
  }
}

void sls_conf_close() {
  sls_conf_base_t *c = sls_first_conf.child;
  if (c != NULL)
    sls_conf_release(c);
}

sls_conf_base_t *sls_conf_get_root_conf() {
  return sls_first_conf.child;
}


int sls_parse_argv(int argc, char *argv[], sls_opt_t *sls_opt, sls_conf_cmd_t *conf_cmd_opt, int cmd_size) {
  char opt_name[256] = { 0 };
  char opt_value[256] = { 0 };
  char temp[256] = { 0 };

  int ret = SLS_OK;
  int i = 1;//skip
  int len = cmd_size;

  //special for '-h'
  if (argc == 2) {
    strcpy(temp, argv[1]);
    remove_quotation_marks(temp);
    if (strcmp(temp, "-h") == 0) {
      printf("SLS help:\n");
      for (i = 0; i < len; i++) {
        printf("-%s %s.\n", conf_cmd_opt[i].name, conf_cmd_opt[i].mark);
      }
    } else {
      printf("Wrong parameter '%s'.\n", argv[1]);
    }
    return SLS_ERROR;
  }
  while (i < argc) {
    strcpy(temp, argv[i]);
    len = strlen(temp);
    if (len == 0) {
      printf("Wrong parameter.\n");
      ret = SLS_ERROR;
      return ret;
    }
    remove_quotation_marks(temp);
    if (temp[0] != '-') {
      printf("Wrong parameter '%s', the first character must be '-'.\n", opt_name);
      ret = SLS_ERROR;
      return ret;
    }
    strcpy(opt_name, temp + 1);

    sls_conf_cmd_t *it = sls_conf_find(opt_name, conf_cmd_opt, cmd_size);
    if (!it) {
      printf("Wrong parameter '%s'.\n", argv[i]);
      ret = SLS_ERROR;
      return ret;
    }
    i++;
    strcpy(opt_value, argv[i++]);
    remove_quotation_marks(opt_value);
    const char *r = it->set(opt_value, it, sls_opt);
    if (r != SLS_CONF_OK) {
      printf("Parameter set failed, %s, name='%s', value='%s'.\n", r, opt_name, opt_value);
      ret = SLS_ERROR;
      return ret;
    }
  }
  return ret;
}

