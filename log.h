#ifndef MY_LOGGER
#define MY_LOGGER

#include <stdio.h>

typedef enum { OFF, DEBUG, INFO, WARN, ERROR } LOG_LEVEL;

// Log out a message
void loggf(LOG_LEVEL level, char *msg, ...);

// Used to set logging level for program
// Levels:
// * OFF
// * DEBUG
// * INFO
// * WARN
int set_log_level(LOG_LEVEL level);

// Set the log file
int set_log_file(FILE *file);

// Close the log file
void close_log_file();

// Initialize logger with default settings (INFO level, stdout)
void init_logger();

#endif
