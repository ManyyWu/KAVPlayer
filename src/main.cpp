#include "KAVPlayer.h"
#include <QtWidgets/QApplication>


#if defined(_DEBUG) && defined(_WIN32)
#include <stdio.h>
#include <Windows.h>

void CreateConsole(void)
{
    errno_t err;
    if (!AllocConsole())
        MessageBox(NULL, TEXT("Failed to create the console!"), TEXT("Error"), MB_ICONERROR);
    freopen("CONOUT$", "w+t", stdout);
    freopen("CONOUT$", "w+t", stderr);
}
#endif

#undef main
int main (int argc, char *argv[])
{
#if defined(_DEBUG) && defined(_WIN32)
//    CreateConsole();
#endif

    QApplication a(argc, argv);
    KAVPlayer w;
    int ret = w.run();
    if (ret < 0)
        return (-ret);

    return a.exec();
}
