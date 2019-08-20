#include "error.h"

const char * kerr2str (int err_code)
{
    if (err_code > KERROR_MAX)
        return "undefined error code.\n";
    return err_string_map[err_code];
}

AVPLAYERWIDGET_EXPORT const char * getErrString (int err_code)
{
    return kerr2str(err_code);
}
