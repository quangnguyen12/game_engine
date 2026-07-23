#include "VulkanApp.h"
#include <iostream>
#include <exception>
#include <cstdlib>

int main()
{
    VulkanApp app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}