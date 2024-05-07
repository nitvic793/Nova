// SimTestBed.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <TestRunner.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int main()
{
    {
        char currentDir[1024] = {};
        GetModuleFileNameA(0, currentDir, 1024);
        char* lastSlash = strrchr(currentDir, '\\');
        if (lastSlash)
        {
            *lastSlash = 0;
            SetCurrentDirectoryA(currentDir);
        }
    }

    std::cout << "Init\n";

    nv::TestRunner testRunner;
    testRunner.Run();
}

