#include "vlog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <mutex>
#include <assert.h>

static double time_sim_start = -1;
static double time_sim_ratio = 1;
static double time_real_start = -1;

void setSimTimeParams(double sim_start, double sim_ratio)
{
  time_sim_ratio = sim_ratio;
  time_real_start = time_now();
  time_sim_start = sim_start;
}

double time_now() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  double now = double(ts.tv_sec) + double(ts.tv_nsec) / 1e9;

  if (time_sim_start < 0) {
    return now;
  } else {
    double real_time_elapsed = now - time_real_start;
    double real_time_scaled = real_time_elapsed/time_sim_ratio;
    double sim_now = time_sim_start + real_time_scaled;
    return sim_now;
  }
}

static char log_file[512] = {};
static char tee_file[512] = {};
static char tee_opened_file[512] = {};
static char sbuffer[4096];

bool vlog_option_location  = false;    // Log the file, line, function for each message?
bool vlog_option_thread_id = false;    // Log the thread id for each message?
bool vlog_option_timelog   = true;     // Log the time for each message?
bool vlog_option_time_date = false;    // Date or timestamp in seconds
bool vlog_option_print_category = false; // Should the category be logged?
bool vlog_option_print_level = true;   // Should the level be logged?
char* vlog_option_file = log_file;     // where to log
char* vlog_option_tee_file = tee_file;
int vlog_option_level = VL_WARNING;    // Log level to use
unsigned int vlog_option_category = 0xFFFFFFFF; // Log categories to use, bitfield
bool vlog_option_exit_on_fatal = true;

static bool vlog_init_done = false;
static std::recursive_mutex vlog_mutex;
static FILE* log_stream = nullptr;
static FILE* tee_stream = nullptr;

#if ENABLE_BACKTRACE

#include "backward/backward.h"
#include "backward/callstack.h"

static void SignalHandlerPrinter( backward::StackTrace& st, FILE* fp )
{
  // Assume all terminals supports ANSI colors
  bool color = isatty( fileno( fp ) );

  std::stringstream output;
  PrintCallstack( output, st, color );

  fprintf(log_stream, "%s", output.str().c_str());
  fflush(log_stream);
  if (tee_stream) {
    fprintf(tee_stream, "%s", output.str().c_str());
    fflush(tee_stream);
  }
}

namespace backward
{
  PrintFunctionDef PrintFunction = SignalHandlerPrinter;
}

static backward::SignalHandling* shptr = nullptr;

#endif // defined ENABLE_BACKTRACE


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
  { "ASSERT" , VCAT_ASSERT  },
  { "VID"    , VCAT_VID     },
  { "DB"     , VCAT_DB      },
};

static const struct log_levels {
    const char *str;
    const char *display_str;
    enum LogLevel lvl;
} log_levels[] = {
  { "FATAL",   "\e[1;31mFATAL\e[m"  , VL_FATAL   },
  { "ALWAYS",  "\e[35mALWAYS\e[m"   , VL_ALWAYS  },
  { "SEVERE",  "\e[31mSEVERE\e[m"   , VL_SEVERE  },
  { "ERROR",   "\e[31mERROR\e[m"    , VL_ERROR   },
  { "WARNING", "\e[33mWARNING\e[m"  , VL_WARNING },
  { "INFO",    "\e[0mINFO\e[0m"     , VL_INFO    },
  { "CONFIG",  "\e[34mCONFIG\e[m"   , VL_CONFIG  },
  { "DEBUG",   "\e[1mDEBUG\e[m"     , VL_DEBUG   },
  { "FINE",    "\e[32mFINE\e[m"     , VL_FINE    },
  { "FINER",   "\e[32mFINER\e[m"    , VL_FINER   },
  { "FINEST",  "\e[1;32mFINEST\e[m" , VL_FINEST  }
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

void set_log_level_string(const char *level)
{
  std::lock_guard<std::recursive_mutex> guard(vlog_mutex);
  bool found = false;
  for(auto& elem: log_levels) {
      if (var_matches(level, elem.str)) {
          vlog_option_level = elem.lvl;
//        printf("Setting vlog level to %s\n", elem.str);
          found = true;
          break;
      }
  }
  if (!found) {
      if (*level == '0') {
          vlog_option_level = 0;
      } else {
          int converted_val = atoi(level);
          if (converted_val != 0) {
              vlog_option_level = converted_val;
          }
      }
  }
}

bool vlog_init()
{
  std::lock_guard<std::recursive_mutex> guard(vlog_mutex);
  if (!vlog_init_done) {
      vlog_init_done = true;

      log_stream = stdout;

#if ENABLE_BACKTRACE
      shptr = new backward::SignalHandling();
#endif // ENABLE_BACKTRACE

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
              set_log_level_string(val);
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
#if ENABLE_BACKTRACE
  if (shptr != nullptr) {
    delete shptr;
    shptr = nullptr;
  }
#endif // ENABLE_BACKTRACE

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
            return elem.display_str;
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
  char *ptr = sbuffer;
  *ptr = 0;

  if (!vlog_init_done) {
      vlog_init();
  }

  if (level > vlog_option_level) 
  {
    va_end(args);
    return;
  }
  
  if (level > VL_ALWAYS) {
      // Fatal and always are printed for all categories
      if (!match_category(category)) {
        va_end(args);
        return;
      }
  }

  std::lock_guard<std::recursive_mutex> guard(vlog_mutex);

  // check the tee file
  if (strcmp(tee_file, tee_opened_file)) {
    if (tee_stream != nullptr) {
      fclose(tee_stream);
      tee_stream = nullptr;
    }
    tee_stream = fopen(tee_file, "a");
    if (tee_stream) {
      strcpy(tee_opened_file, tee_file);
    }
  }

  // Do the printing
  if (newline) { // only print the preamble if there is a newline
    ptr += sprintf(ptr, "\n");
    if (vlog_option_print_level && (level != VL_ALWAYS)) {
      ptr += sprintf(ptr, "%10s ", get_level_str(level));
    }
    if (vlog_option_print_category) {
        bool found = false;
        for(auto& elem: log_categories) {
            if (elem.cat == category) {
                ptr += sprintf(ptr, "[%7s] ", elem.str);
                found = true;
                break;
            }
        }
        if (!found) {
            ptr += sprintf(ptr, "[UNKNOWN] ");
        }
    }
    if (vlog_option_timelog) {
        if (vlog_option_time_date) {
            // TODO: Not implemented so far
        } else {
           double now = time_now();
           ptr += sprintf(ptr, "[%f] ", now);
        }
    }
    if (vlog_option_thread_id) {
      ptr += sprintf(ptr, "<%d> ", gettid());
    }
    if (vlog_option_location) {
        ptr += sprintf(ptr, "%s:%d,{%s} ", file, line, func);
    }
  }
  ptr += vsprintf(ptr, fmt, args);
  va_end(args);
  fprintf(log_stream, "%s", sbuffer);
  fflush(log_stream);
  if (tee_stream) {
    fprintf(tee_stream, "%s", sbuffer);
    fflush(tee_stream);
  }


  if (vlog_option_exit_on_fatal && level == VL_FATAL) {
    // print stack
#if ENABLE_BACKTRACE
    std::stringstream out;
    PrintCurrentCallstack(out, true, nullptr);
    fprintf(log_stream, "\n%s\n", out.str().c_str());
    fflush(log_stream);
    if (tee_stream) {
      fprintf(tee_stream, "\n%s\n", out.str().c_str());
      fflush(tee_stream);
    }
#endif

    // TODO - add callback for cleaning up drivers, etc.
    fprintf(log_stream, "\n");
    fflush(log_stream);
    assert(false); // This helps break in the debugger
    exit(-1);
  }
}

void vlog_flush() // Ensure all data is on disk
{
  std::lock_guard<std::recursive_mutex> guard(vlog_mutex);
  if (!vlog_init_done) {
      vlog_init();
  }

  fflush(log_stream);
  if (tee_stream) {
    fflush(tee_stream);
  }

}
