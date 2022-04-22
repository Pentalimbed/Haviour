#include "app.h"

int main(int argc, char* argv[])
{
    if (auto result = Haviour::initApp())
    {
        return result;
    }
    Haviour::mainLoop();
    Haviour::exitApp();
}