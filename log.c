#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int log_level = OFF;

void loggf(LOG_LEVEL level, char *msg, ...) {
  // log out message
  //
  // char *full_msg = strcat("[LOGGER]: ", msg);
  char *levels[] = {"OFF", "DEBUG", "INFO", "WARN", "ERROR"};
  if (level >= log_level) {
    va_list fmt_args;
    va_start(fmt_args, msg);
    fprintf(stdout, "[%s]: ", levels[(int)level]);
    vfprintf(stdout, msg, fmt_args);
    va_end(fmt_args);
  }
};

int set_log_level(LOG_LEVEL level) {
  // setting log level
  if (log_level != OFF) {
    return 1;
  }

  log_level = level;
  return 0;
}
