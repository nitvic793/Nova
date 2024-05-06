// SimTestBed.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <TestRunner.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int main()
{
    std::cout << "Init\n";

    nv::TestRunner testRunner;
    testRunner.Run();
}

