#pragma once

#include <stdarg.h>
#include <time.h>

#include <functional>
#include <string>

#if defined(NDEBUG)
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#else
#include <assert.h>
#endif

// TODO(jhurliman): Enable this to weed out all the assert() calls
// #undef assert
// #define assert(cond) static_assert(cond)

// Sample categories
#define VCAT_UNKNOWN "UNKNOWN"
#define VCAT_GENERAL "GENERAL"
#define VCAT_ASSERT "ASSERT"

// Environment variables
#define VLOG_CATEGORY "VLOG_CATEGORY"
#define VLOG_LEVEL "VLOG_LEVEL"
#define VLOG_TIME_FORMAT "VLOG_TIME_FORMAT"
#define VLOG_COLOR "VLOG_COLOR"
#define VLOG_PRINT_LEVEL "VLOG_PRINT_LEVEL"
#define VLOG_PRINT_CATEGORY "VLOG_PRINT_CATEGORY"
#define VLOG_TIME_LOG "VLOG_TIME_LOG"
#define VLOG_THREAD_NAME "VLOG_THREAD_NAME"
#define VLOG_THREAD_ID "VLOG_THREAD_ID"
#define VLOG_SRC_LOCATION "VLOG_SRC_LOCATION"
#define VLOG_EXIT_ON_FATAL "VLOG_EXIT_ON_FATAL"
#define VLOG_FILE "VLOG_FILE"

enum LogLevel {
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

using VlogHandler =
    std::function<void(int level, const char* category, const char* threadName, const char* file, int line,
                       const char* func, const char* logMsg, int msgLen)>;

using VlogNewFileHandler = std::function<void(const char* filename)>;

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
       This variable controls if the log writes the time in date format or in timestamp (floating point number
   representing seconds)

    VLOG_LEVEL -> ERROR (default), ...
       This variable controls the level of logging, by default only error or more severe are printed. Numbers
   are also accepted.

    VLOG_CATEGORY -> ALL (default), GENERAL, DETECT, ...
       This variable controls which categories are printed. All is the default, but a semicolon separated list
   of categories can be added

    VLOG_PRINT_CATEGORY -> 1, 0 (default)
       This variable controls if the category is logged on each message

    VLOG_PRINT_LEVEL -> 1 (default), 0
       This variable controls if the level is logged on each message

    VLOG_COLOR -> 1 (default), 0
       This variable controls if we print color, useful for CI
 */

void setSimTimeParams(double sim_start, double sim_ratio);

/// set_sim_time is used in situations where simulation time is completely
/// dictated by the application, and not tied to a uniform monotonically increasing
/// clock.
/// e.g. Replayer might use this to set the time based on consecutive message timestamps
/// this mode allows more control of sim time.
///
/// \note when set_sim_time is used time will only be updated every time set_sim_time is called
/// instead of increading uniformly according to the system clock
void set_sim_time(double t);
bool is_sim_time();
double time_now();

#ifdef _MSC_VER
// No equivalent in MSVC for printf-style checks, so we leave it empty
#define PRINTF_ATTRIBUTE(a, b)
#else
// For GCC and Clang
#define PRINTF_ATTRIBUTE(a, b) __attribute__((format(printf, a, b)))
#endif

void vlog_func(int level, const char* category, bool newline, const char* file, int line, const char* func,
               const char* fmt, ...) PRINTF_ATTRIBUTE(7, 8);

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define vlog(level, category, ...) vlog_func(level, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Function that does not do a new line, to continue logging
#define vlog_cont(level, category, ...) \
  vlog_func(level, category, false, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_fatal(category, ...) \
  vlog_func(VL_FATAL, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_severe(category, ...) \
  vlog_func(VL_SEVERE, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_error(category, ...) \
  vlog_func(VL_ERROR, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_warning(category, ...) \
  vlog_func(VL_WARNING, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_info(category, ...) vlog_func(VL_INFO, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_config(category, ...) \
  vlog_func(VL_CONFIG, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_debug(category, ...) \
  vlog_func(VL_DEBUG, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_fine(category, ...) vlog_func(VL_FINE, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_finer(category, ...) \
  vlog_func(VL_FINER, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_finest(category, ...) \
  vlog_func(VL_FINEST, category, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define vlog_always(...) vlog_func(VL_ALWAYS, VCAT_UNKNOWN, true, __FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef __llvm__
#define VLOG_ASSERT(expr, ...)                                             \
  do {                                                                     \
    if (unlikely(!(expr))) {                                               \
      bool old_val = vlog_option_location;                                 \
      vlog_option_location = true;                                         \
      vlog_func(VL_FATAL, VCAT_ASSERT, true, __FILE__, __LINE__, __func__, \
                "Assertion failed: " #expr " " __VA_ARGS__);               \
      vlog_option_location = old_val;                                      \
      __builtin_debugtrap();                                               \
    }                                                                      \
  } while (0)
#else
#define VLOG_ASSERT(expr, ...)                                             \
  do {                                                                     \
    if (unlikely(!(expr))) {                                               \
      bool old_val = vlog_option_location;                                 \
      vlog_option_location = true;                                         \
      vlog_func(VL_FATAL, VCAT_ASSERT, true, __FILE__, __LINE__, __func__, \
                "Assertion failed: " #expr " " __VA_ARGS__);               \
      vlog_option_location = old_val;                                      \
      __builtin_trap();                                                    \
    }                                                                      \
  } while (0)
#endif

bool vlog_init();
void vlog_fini();
void vlog_flush();  // Ensure all data is on disk

void set_log_level_string(const char* level);

int vlog_add_callback(VlogHandler callback);
void vlog_clear_callback(int id);
void vlog_clear_callbacks();

int vlog_add_new_file_callback(VlogNewFileHandler cb);

// This function should only be used inside callbacks, it is not safe otherwise
const char* get_level_str(int level);

// These variables are for manual setting of logging before init

extern volatile bool vlog_option_location;        // Log the file, line, function for each message?
extern volatile bool vlog_option_thread_id;       // Log the thread id for each message?
extern volatile bool vlog_option_thread_name;     // Log the thread name for each message?
extern volatile bool vlog_option_timelog;         // Log the time for each message?
extern volatile bool vlog_option_time_date;       // Date or timestamp in seconds
extern volatile bool vlog_option_print_category;  // Should the category be logged?
extern volatile bool vlog_option_print_level;     // Should the level be logged?
extern volatile char* vlog_option_file;           // where to log
extern volatile char* vlog_option_tee_file;       // File where to log simultaneously
extern int vlog_option_level;                     // Log level to use
extern const char* vlog_option_category;          // Log categories to use, semicolon separated words
extern volatile bool vlog_option_exit_on_fatal;   // Call exit after a vlog_fatal
extern volatile bool vlog_option_color;           // Display color in terminal or not
extern const char* vlog_vars;                     // Use this variable to print help on vlog if needed

int getOptionLevel();
void setOptionLevel(int level);
const char* getOptionCategory();
void setOptionCategory(const char* cat);

std::string FormatString(const char* fmt, ...);

#ifdef _WIN32
typedef int pid_t;
#endif

pid_t GetThreadId();
