#ifndef MY_LOGGER
#define MY_LOGGER

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
#endif
