#include "utils.h"

#ifdef _WIN32
#include <Windows.h>
#endif

void init_dynload()
{
#ifdef _WIN32
    /* Calling SetDllDirectory with the empty string (but not NULL) removes the
     * current working directory from the DLL search path as a security pre-caution. */
    SetDllDirectory(TEXT(""));
#endif
}
