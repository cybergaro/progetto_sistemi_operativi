#ifdef GUI
    #include "gui.h"
#else
    #include "cliview.h"
#endif

int main(int argc, char const *argv[])
{

    #ifdef GUI
        initGUI();
    #else
        printMap();
    #endif

    return 0;
}