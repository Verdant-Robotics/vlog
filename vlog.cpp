#include "vlog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <mutex>

static char log_file[512] = {};

bool vlog_option_location  = false;    // Log the file, line, function for each message?
bool vlog_option_thread_id = false;    // Log the thread id for each message?
bool vlog_option_timelog   = true;     // Log the time for each message?
bool vlog_option_time_date = false;    // Date or timestamp in seconds
bool vlog_option_print_category = false; // Should the category be logged?
bool vlog_option_print_level = true;   // Should the level be logged?
char* vlog_option_file = log_file;     // where to log
int vlog_option_level = VL_WARNING;    // Log level to use
unsigned int vlog_option_category = 0xFFFFFFFF; // Log categories to use, bitfield
bool vlog_option_exit_on_fatal = true;

static bool vlog_init_done = false;
static std::mutex vlog_mutex;
static FILE* log_stream = nullptr;

static const struct log_categories {
    const char *str;
    enum LogCategory cat;
} log_categories[] = {
  { "UNKNOWN", VCAT_UNKNOWN },
  { "GENERAL", VCAT_GENERAL },
  { "DETECT" , VCAT_DETECT  },
  { "TRACK"  , VCAT_TRACK   },
  { "SENSOR" , VCAT_SENSOR  },
  { "HAL"    , VCAT_HAL     },
  { "GUI"    , VCAT_GUI     },
  { "TEST"   , VCAT_TEST    },
  { "NODE"   , VCAT_NODE    },
  { "ASSERT" , VCAT_ASSERT  }
};

static const struct log_levels {
    const char *str;
    enum LogLevel lvl;
} log_levels[] = {
  { "OFF"    , VL_OFF     },
  { "FATAL"  , VL_FATAL   },
  { "ALWAYS" , VL_ALWAYS  },
  { "SEVERE" , VL_SEVERE  },
  { "ERROR"  , VL_ERROR   },
  { "WARNING", VL_WARNING },
  { "INFO"   , VL_INFO    },
  { "CONFIG" , VL_CONFIG  },
  { "DEBUG"  , VL_DEBUG   },
  { "FINE"   , VL_FINE    },
  { "FINER"  , VL_FINER   },
  { "FINEST" , VL_FINEST  }
};

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

const char* vlog_vars =
  R"(
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
)";

static bool var_matches(const char *var, const char *opt)
{
  return strncasecmp(var, opt, strlen(opt)) == 0;
}

static const char *getval(const char *var)
{
    const char *r = var;
    while( (*r != '=') && (*r != 0) ) r++;
    return ++r;
}

bool vlog_init()
{
  std::lock_guard<std::mutex> guard(vlog_mutex);
  if (!vlog_init_done) {
      vlog_init_done = true;

      log_stream = stdout;

      char **env;
      for( env = environ; *env != nullptr; env++ ) {
          char* var = *env;
          const char *val = getval(var);

          if (var_matches(var, "VLOG_FILE")) {
              if (var_matches(val, "stdout")) {
                  // Nothing to do, this is the default
              } else if (var_matches(val, "stderr")) {
                  log_stream = stderr;
              } else {
                  FILE *f = fopen(val, "w+");
                  if (f != nullptr) {
                      log_stream = f;
                  } else {
                      fprintf(stderr, "Could not log to file %s , logging to stdout\n", val);
                  }
              }
          } else if (var_matches(var, "VLOG_EXIT_ON_FATAL")) {
            vlog_option_exit_on_fatal = (*val == '1');
          } else if (var_matches(var, "VLOG_SRC_LOCATION")) {
            vlog_option_location = (*val == '1');
          } else if (var_matches(var, "VLOG_THREAD_ID")) {
            vlog_option_thread_id = (*val == '1');
          } else if (var_matches(var, "VLOG_TIME_LOG")) {
            vlog_option_timelog = (*val == '1');
          } else if (var_matches(var, "VLOG_PRINT_CATEGORY")) {
            vlog_option_print_category = (*val == '1');
          } else if (var_matches(var, "VLOG_PRINT_LEVEL")) {
            vlog_option_print_level = (*val == '1');
          } else if (var_matches(var, "VLOG_TIME_FORMAT")) {
              if (var_matches(val, "date")) {
                  vlog_option_time_date = true;
              } else if (var_matches(val, "stamp")) {
                  vlog_option_time_date = false;
              }
          } else if (var_matches(var, "VLOG_LEVEL")) {
              bool found = false;
              for(auto& elem: log_levels) {
                  if (var_matches(val, elem.str)) {
                      vlog_option_level = elem.lvl;
//                      printf("Setting vlog level to %s\n", elem.str);
                      found = true;
                      break;
                  }
              }
              if (!found) {
                  if (*val == '0') {
                      vlog_option_level = 0;
                  } else {
                      int converted_val = atoi(val);
                      if (converted_val != 0) {
                          vlog_option_level = converted_val;
                      }
                  }
              }
          } else if (var_matches(var, "VLOG_CATEGORY")) {
              if (var_matches(val, "ALL")) {
                  // Nothing to do, this is the default
              } else {
                  unsigned int mask = 0;
                  for(auto& elem: log_categories) {
                      if (strcasestr(val, elem.str)) {
//                          printf("Setting vlog category to %s\n", elem.str);
                          mask |= (1 << elem.cat);
                      }
                  }
                  if (mask != 0) {
                      mask |= 1 << VCAT_ASSERT;  // Always show assertion failures.
                      vlog_option_category = mask;
                  }
              }
          }
      }

  }
  return true;
}

void vlog_fini()
{
    // Close the handles we have
    if ((log_stream != stdout) && (log_stream != stderr)) {
        fclose(log_stream);
    }
}

static pid_t gettid()
{
  return syscall(SYS_gettid);
}

static inline bool match_category(int category)
{
  return (vlog_option_category & (1 << category)) != 0;
}

static const char *get_level_str(int level)
{
    for(auto& elem: log_levels) {
        if (elem.lvl == level) {
            return elem.str;
        }
    }
    // We can do this because we only allow a single thread to be printing
    static char buf[64];
    sprintf(buf, "LVL_%d", level);
    return buf;
}

void vlog_func(int level, int category, bool newline, const char *file, int line, const char *func, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);

  if (!vlog_init_done) {
      vlog_init();
  }

  if (level > vlog_option_level && level > VL_FATAL) return;
  if (level > VL_ALWAYS) {
      // Fatal and always are printed for all categories
      if (!match_category(category)) return;
  }

  std::lock_guard<std::mutex> guard(vlog_mutex);

  // Do the printing
  if (newline) { fprintf(log_stream, "\n"); }
  if (vlog_option_print_level && (level != VL_ALWAYS)) {
    fprintf(log_stream, "%7s ", get_level_str(level));
  }
  if (vlog_option_print_category) {
      bool found = false;
      for(auto& elem: log_categories) {
          if (elem.cat == category) {
              fprintf(log_stream, "[%7s] ", elem.str);
              found = true;
              break;
          }
      }
      if (!found) {
          fprintf(log_stream, "[UNKNOWN] ");
      }
  }
  if (vlog_option_timelog) {
      if (vlog_option_time_date) {
          // TODO: Not implemented so far
      } else {
         double now = time_now();
         fprintf(log_stream, "[%f] ", now);
      }
  }
  if (vlog_option_thread_id) {
    fprintf(log_stream, "<%d> ", gettid());
  }
  if (vlog_option_location) {
      fprintf(log_stream, "%s:%d,{%s} ", file, line, func);
  }
  vfprintf(log_stream, fmt, args);
  va_end(args);
  fflush(log_stream);

  if (vlog_option_exit_on_fatal) {
    // TODO - print stack
    // TODO - add callback for cleaning up drivers, etc.
    exit(-1);
  }
}

void vlog_flush() // Ensure all data is on disk
{
  std::lock_guard<std::mutex> guard(vlog_mutex);
  if (!vlog_init_done) {
      vlog_init();
  }

  fflush(log_stream);
}
