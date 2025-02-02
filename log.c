#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int log_level = OFF;

static FILE *log_file = NULL;

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

int set_log_file(FILE *file) {
  log_file = file;
  return 0;
}

void close_log_file() {
  if (log_file != NULL && log_file != stdout) {
    fclose(log_file);
  }
}

// Initialize logger with default settings (INFO level, stdout)
void init_logger() {
  set_log_level(INFO);
  set_log_file(stdout);
}