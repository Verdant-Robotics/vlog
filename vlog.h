#pragma once

#include <stdarg.h>
#include <time.h>

#if defined(NDEBUG)
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#else
#include <assert.h>
#endif

// To add a new category or log level, please modify vlog.cpp the
// static arrays log_levels and log_categories
enum LogCategory
{
  VCAT_UNKNOWN = 0,
  VCAT_GENERAL,
  VCAT_DETECT,
  VCAT_TRACK,
  VCAT_SENSOR,
  VCAT_GUI,
  VCAT_HAL,
  VCAT_TEST,
  VCAT_NODE,
  VCAT_ASSERT,
  VCAT_VID,
  VCAT_DB
};

enum LogLevel
{
  VL_FATAL = 0,
  VL_ALWAYS = 2,
  VL_SEVERE = 5,
  VL_ERROR = 10,
  VL_WARNING = 15,
  VL_INFO = 20,
  VL_CONFIG = 25,
  VL_DEBUG = 30,
  VL_FINE = 35,
  VL_FINER = 40,
  VL_FINEST = 50
};

/*
   Environment variables to control logging:

    VLOG_FILE -> stdout (default), stderr, <file path>
       This variable controls where the logging is going

    VLOG_SRC_LOCATION -> 1 , 0 (default)
       This variable controls whether we print the file, line and function name where
       the logging originated

    VLOG_THREAD_ID -> 1 , 0 (default)
       This variable controls the printing of the thread id that is logging

    VLOG_TIME_LOG -> 1, 0 (default)
       This variable controls whether a timestamp/date is included on each log

    VLOG_TIME_FORMAT -> stamp (default), date
       This variable controls if the log writes the time in date format or in timestamp (floating point number representing seconds)

    VLOG_LEVEL -> ERROR (default), ...
       This variable controls the level of logging, by default only error or more severe are printed. Numbers are also accepted.

    VLOG_CATEGORY -> ALL (default), GENERAL, DETECT, ...
       This variable controls which categories are printed. All is the default, but a comma separated list of categories can be added

    VLOG_PRINT_CATEGORY -> 1, 0 (default)
       This variable controls if the category is logged on each message

    VLOG_PRINT_LEVEL -> 1 (default), 0
       This variable controls if the level is logged on each message

    VLOG_COLOR -> 1 (default), 0
       This variable controls if we print color, useful for CI
 */

void setSimTimeParams(double sim_start, double sim_ratio);
double time_now();

void vlog_func(int level, int category, bool newline, const char *file, int line, const char *func, const char *fmt, ...)  __attribute__ (( format( printf, 7, 8 ) ));

#define vlog(level, category, ...) \
  vlog_func(level, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

// Function that does not do a new line, to continue logging
#define vlog_cont(level, category, ...) \
  vlog_func(level, category, false, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_fatal(category, ...) \
  vlog_func(VL_FATAL, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_severe(category, ...) \
  vlog_func(VL_SEVERE, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_error(category, ...) \
  vlog_func(VL_ERROR, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_warning(category, ...) \
  vlog_func(VL_WARNING, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_info(category, ...) \
  vlog_func(VL_INFO, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_config(category, ...) \
  vlog_func(VL_CONFIG, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_debug(category, ...) \
  vlog_func(VL_DEBUG, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_fine(category, ...) \
  vlog_func(VL_FINE, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_finer(category, ...) \
  vlog_func(VL_FINER, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_finest(category, ...) \
  vlog_func(VL_FINEST, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define vlog_always(...) \
  vlog_func(VL_ALWAYS, VCAT_UNKNOWN, true, __FILE__, __LINE__, __func__, __VA_ARGS__ )

#define VLOG_ASSERT(expr, ...)                                          \
  do {                                                                  \
    if (!(expr)) {                                                      \
      bool old_val = vlog_option_location;                              \
      vlog_option_location = true;                                      \
      vlog_func(VL_FATAL, VCAT_ASSERT, true, __FILE__, __LINE__,        \
                __func__,                                               \
                "Assertion failed: " #expr " " __VA_ARGS__ );           \
      vlog_option_location = old_val;                                   \
      assert(false);                                                    \
    }                                                                   \
  } while (0)                                                           

bool vlog_init();
void vlog_fini();
void vlog_flush(); // Ensure all data is on disk

void set_log_level_string(const char *level);

// These variables are for manual setting of logging before init

extern bool vlog_option_location;  // Log the file, line, function for each message?
extern bool vlog_option_thread_id; // Log the thread id for each message?
extern bool vlog_option_timelog;   // Log the time for each message?
extern bool vlog_option_time_date; // Date or timestamp in seconds
extern bool vlog_option_print_category; // Should the category be logged?
extern bool vlog_option_print_level; // Should the level be logged?
extern char* vlog_option_file;     // where to log
extern char* vlog_option_tee_file; // File where to log simultaneously
extern int vlog_option_level;      // Log level to use
extern unsigned int vlog_option_category;   // Log categories to use, bitfield
extern bool vlog_option_exit_on_fatal; // Call exit after a vlog_fatal
extern bool vlog_option_color; // Display color in terminal or not

extern const char* vlog_vars; // Use this variable to print help on vlog if needed

