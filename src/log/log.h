#ifndef _AVPLEYRWIDGET_LOG_H_
#define _AVPLEYRWIDGET_LOG_H_

#include <cstdio>
#include <cstdarg>

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include "libavutil/log.h"
}

enum LogLevel {
    LOG_FATAL   = 0, 
    LOG_ERROR   = 1,
    LOG_WARNING = 2,
    LOG_INFO    = 3,
    LOG_DEBUG   = 4,
    LOG_VERBOSE = 5
};

#define FATALN(format, ...)   fatal(format, FILENAME, __LINE__, ##__VA_ARGS__)
#define ERRORN(format, ...)   error(format, FILENAME, __LINE__, ##__VA_ARGS__)
#define WARNINGN(format, ...) warning(format, FILENAME, __LINE__, ##__VA_ARGS__)

#define ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

static const struct {
    int         level;
    const char *str;
}log_level_map[7] = {{LOG_FATAL,   "[F] "},
                     {LOG_ERROR,   "[E] "},
                     {LOG_WARNING, "[W] "},
                     {LOG_INFO,    "[I] "},
                     {LOG_DEBUG,   "[D] "},
                     {LOG_VERBOSE, "[V] "}
};

#ifdef _WIN32
class _declspec(dllimport) Logger {
#else
class Logger {
#endif
private:
    FILE *fplog;
    bool  dis_info_log;
    bool  dis_debug_log;
    bool  dis_verbose_log;
    bool  dis_level_label;

private:
    int  write_log   (int level, const char * fmt, va_list *vl); 

public:
    int  init        (const char *file_name);
    int  fatal       (const char *fmt, ...);
    int  error       (const char *fmt, ...);
    int  warning     (const char *fmt, ...);
    int  info        (const char *fmt, ...);
    int  debug       (const char *fmt, ...);
    int  verbose     (const char *fmt, ...);
    void close       ();
    void dis_info    ();
    void dis_debug   ();
    void dis_verbose ();
    void dis_label   ();
    void en_info     ();
    void en_debug    ();
    void en_verbose  ();
    void en_label    ();
    
public:
    Logger           ();
};

#ifdef _WIN32
extern Logger _declspec(dllimport) logger;
#else
extern Logger logger;
#endif

#endif /* _AVPLEYRWIDGET_LOG_H_ */
