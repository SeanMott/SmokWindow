#pragma once

//C API implementation of Desktop Window
//manages creating and handling the window

#include <BTDSTD_C/Math/Vectors.h>
#include <BTDSTD_C/Logger.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>//<vk_mem_alloc.h>
#include <GLFW/glfw3.h>

//defines a swapchain
typedef struct SMWindow_Desktop_Swapchain
{
	//--vars

	VkExtent2D extents;
	
	uint32 framesInFlight;
	uint32 imageCount;
	
	VkFramebuffer framebuffers[3];

	VkImage depthImage[3];
	VmaAllocation depthImageMemoy[3];
	VkImageView depthView[3];

	VkImage images[3];
	VkImageView imageViews[3];
	
	VkRenderPass renderpass;
	
	VkSwapchainKHR swapchain;
	VkDevice device;
	
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkFormat depthFormat;
	VkSurfaceCapabilitiesKHR capabilities;

	VkImageLayout imageLayoutOutputRenderPassSetting; //the output format for the swapchain render pass will put it's images in

} SMWindow_Desktop_Swapchain;

#if defined(__cplusplus)
extern "C" {
#endif

	//initalize default values for desktop swapchain
	void SMWindow_Desktop_Swapchain_InitalizeDefaultValues(SMWindow_Desktop_Swapchain* swapchain);

	//creates a swapchain
	int SMWindow_Desktop_Swapchain_CreateSwapchain(SMWindow_Desktop_Swapchain* swapchain,
		VkPhysicalDevice gpu, VkDevice device, const uint32 graphicsQueueIndex, const uint32 presentQueueIndex,
		GLFWwindow* window, VkSurfaceKHR surface, VmaAllocator allocator, VkImageLayout imageLayoutOutputRenderPassSetting);

	//destroys a swapchain
	void SMWindow_Desktop_Swapchain_DestroySwapchain(SMWindow_Desktop_Swapchain* swapchain,
		VkDevice device, VmaAllocator allocator);

	//remakes a swapchain
	int SMWindow_Desktop_Swapchain_RemakeSwapchain(SMWindow_Desktop_Swapchain* swapchain,
		VkDevice device, VkPhysicalDevice gpu, const uint32 graphicsQueueIndex, const uint32 presentQueueIndex,
		GLFWwindow* window, VkSurfaceKHR surface, VmaAllocator allocator);

#if defined(__cplusplus)
}
#endif

//defines a create info for creating a window
typedef struct SMWindow_Desktop_Window_WindowCreateInfo
{
	int shouldBeResizable; //should the window be resizeable
	BTD_Math_I32Vec2 size; //the window size
	const char* windowName; //the name

	void(*onWindowCloseCallback)(GLFWwindow*); //callback for when the window is closed
	void(*onWindowResizeCallback)(GLFWwindow*, int, int); //callback for when the window's framebuffer/size is changed
	//void(*onWindowOriantationChangedCallback)(GLFWwindow*) = nullptr; //callback for when the window's oriantation is changed (this is a mobile thing)
	void(*onWindowKeyboardInputCallback)(GLFWwindow*, int, int, int, int); //callback for keyboard input
} SMWindow_Desktop_Window_WindowCreateInfo;

//defines a window
typedef struct SMWindow_Desktop_Window
{
	int isResized;
	int isRunning;
	BTD_Math_I32Vec2 size;

	VkSurfaceKHR surface;
	GLFWwindow* window;
} SMWindow_Desktop_Window;

#if defined(__cplusplus)
extern "C" {
#endif

	//creates a window
	int SMWindow_Desktop_Window_Create(SMWindow_Desktop_Window* window, SMWindow_Desktop_Window_WindowCreateInfo* info, VkInstance vulkanInstance);

	//shutsdown a display
	void SMWindow_Desktop_Window_ShutdownWindow(SMWindow_Desktop_Window* window);

	//destroys a window
	void SMWindow_Desktop_Window_DestroyWindow(SMWindow_Desktop_Window* window, VkInstance vulkanInstance);

#if defined(__cplusplus)
}
#endif