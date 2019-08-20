#include "log.h"
#include "error/error.h"
#include <cstdio>
#include <cstdarg>

Logger logger;

int Logger::write_log(int level, const char * fmt, va_list *vl) 
{
    char    err_str[4096]; ///////////////////////////////////////

    if (!fplog)
        return KERROR(KEUNINITED);

    if (dis_level_label) {
        vsprintf(err_str, fmt, *vl);
    } else {
        sprintf(err_str, "%s", log_level_map[level].str);
        vsprintf(err_str + 4, fmt, *vl);
    }
#ifdef _DEBUG
    fputs(err_str, stderr);
    fflush(stderr);
#endif /* _DEBUG */
    fputs(err_str, fplog);
    fflush(fplog);

    return 0;
}

int Logger::init (const char *file_name)
{
    if (!file_name)
        return KERROR(KEINVAL);

    fplog = fopen(file_name, "a");
    if (!fplog)
        return KERROR(KELOG_FILE_OPEN_FAIL);

    logger.debug("Logger has been inited.\n");

    return 0;
}

int Logger::fatal (const char * fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    int ret = write_log(LOG_FATAL, fmt, &vl);
    va_end(vl);

    return ret;
}

int Logger::error (const char * fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    int ret = write_log(LOG_ERROR, fmt, &vl);
    va_end(vl);

    return ret;
}

int Logger::warning (const char * fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    int ret = write_log(LOG_WARNING, fmt, &vl);
    va_end(vl);

    return ret;
}

int Logger::info (const char * fmt, ...)
{
    if (dis_info_log)
        return 0;

    va_list vl;
    va_start(vl, fmt);
    int ret = write_log(LOG_INFO, fmt, &vl);
    va_end(vl);

    return ret;
}

int Logger::debug (const char * fmt, ...)
{
#ifdef _DEBUG
    if (dis_debug_log)
        return 0;

    va_list vl;
    va_start(vl, fmt);
    int ret = write_log(LOG_DEBUG, fmt, &vl);
    va_end(vl);

    return ret;
#else /* _DEBUG */
    return 0;
#endif /* _DEBUG */
}

int Logger::verbose (const char * fmt, ...)
{
#ifdef _DEBUG
    if (dis_verbose_log)
        return 0;

    va_list vl;
    va_start(vl, fmt);
    int ret = write_log(LOG_VERBOSE, fmt, &vl);
    va_end(vl);

    return ret;
#else /* _DEBUG */
    return 0;
#endif /* _DEBUG */
}

void Logger::close ()
{
    fclose(fplog);
    logger.debug("Logger closed.\n");
}

void Logger::dis_info ()
{
    dis_info_log = true;
}

void Logger::dis_debug ()
{
    dis_debug_log = true;
}

void Logger::dis_verbose ()
{
    dis_verbose_log = true;
}

void Logger::dis_label ()
{
    dis_level_label = true;
}

void Logger::en_info ()
{
    dis_info_log = false;
}

void Logger::en_debug ()
{
    dis_debug_log = false;
}

void Logger::en_verbose ()
{
    dis_verbose_log = false;
}

void Logger::en_label ()
{
    dis_level_label = false;
}

Logger::Logger ()
{
    fplog = NULL;
    dis_info_log = false;
    dis_debug_log = false;
    dis_verbose_log = false;
    dis_level_label = false;
}
