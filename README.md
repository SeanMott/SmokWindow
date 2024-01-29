Smok Window


Smok Window is a single header file for managing windows and swapchains in Vulkan.

All it needs is one .cpp file to have the macro #define SMOK_WINDOW_IMPL

Smok Window depends on Smok Graphics anbd BTDSTD

Smok Window is written in a C style Pod Of Data style but still with namespaces. Ie Smok::Window::DesktopWindow_Create(&GContext, &window)