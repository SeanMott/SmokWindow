//C API implementation of Desktop Window

#include <SmokWindow/Desktop/DesktopWindow.h>

#include <SmokGraphics/Utils/Image.h>

#include <BTDSTD_C/Types.h>

//initalize default values for desktop swapchain
void SMWindow_Desktop_Swapchain_InitalizeDefaultValues(SMWindow_Desktop_Swapchain* swapchain)
{
	swapchain->framesInFlight = 2;
	swapchain->imageCount = 0;
}

//private internal function for checking and validating settings of the swapchain
static int CheckSwapchainSettings(SMWindow_Desktop_Swapchain* swapchain,
	VkPhysicalDevice gpu, GLFWwindow* window, VkSurfaceKHR surface)
{
	VkSurfaceFormatKHR desiredSurfaceFormat = { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	//gets swapchain details
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &swapchain->capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, NULL);
	VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)calloc(formatCount, sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, formats);

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, NULL);
	VkPresentModeKHR* presentModes = (VkPresentModeKHR*)calloc(presentModeCount, sizeof(VkPresentModeKHR));
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, presentModes);

	//gets image count
	swapchain->imageCount = swapchain->capabilities.minImageCount + 1;
	if (swapchain->capabilities.maxImageCount > 0 && swapchain->imageCount > swapchain->capabilities.maxImageCount)
		swapchain->imageCount = swapchain->capabilities.maxImageCount;

	//gets surface format
	int found = 0;
	for (uint32 i = 0; i < formatCount; ++i)
	{
		if (formats[i].format == desiredSurfaceFormat.format && formats[i].colorSpace == desiredSurfaceFormat.colorSpace)
		{
			swapchain->surfaceFormat = formats[i];
			found = 1;
			break;
		}
	}
	if (!found)
		swapchain->surfaceFormat = formats[0];

	//gets present mode
	found = 0;
	for (uint32 i = 0; i < presentModeCount; ++i)
	{
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			swapchain->presentMode = presentModes[i];
			found = 1;
			break;
		}
	}
	if (!found)
		swapchain->presentMode = VK_PRESENT_MODE_FIFO_KHR;

	//gets the depth format
	VkFormat depthFormatCandidates[3];
	depthFormatCandidates[0] = VK_FORMAT_D32_SFLOAT;
	depthFormatCandidates[1] = VK_FORMAT_D32_SFLOAT_S8_UINT;
	depthFormatCandidates[2] = VK_FORMAT_D24_UNORM_S8_UINT;
	for (uint32 i = 0; i < 3; ++i)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(gpu, depthFormatCandidates[i], &props);

		if ((props.optimalTilingFeatures & ((VkFormatFeatureFlags)VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) ==
			((VkFormatFeatureFlags)VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			swapchain->depthFormat = depthFormatCandidates[i];
			break;
		}
	}

	//free candidates
	free(presentModes); presentModes = NULL; free(formats); formats = NULL;

	//gets the size
	if (swapchain->capabilities.currentExtent.width != UINT32_MAX)
		swapchain->extents = swapchain->capabilities.currentExtent;

	//gets window size and clap to the GPU limits
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		swapchain->extents.width = Smok_Util_Typepun(width, uint32);
		swapchain->extents.height = Smok_Util_Typepun(height, uint32);

		//claps it to GPU settings
		swapchain->extents.width = BTD_Math_ClampUint32(swapchain->extents.width,
			swapchain->capabilities.minImageExtent.width, swapchain->capabilities.maxImageExtent.width);
		swapchain->extents.height = BTD_Math_ClampUint32(swapchain->extents.height,
			swapchain->capabilities.minImageExtent.height, swapchain->capabilities.maxImageExtent.height);
	}

	return 1;
}

//gets the size of the swapchain
static void GetSwapchainSize(GLFWwindow* window, BTD_Math_I32Vec2* size)
{
	//get size
	glfwGetFramebufferSize(window, &size->x, &size->y);
	while (size->x == 0 || size->y == 0) {
		glfwGetFramebufferSize(window, &size->x, &size->y);
		glfwWaitEvents();
	}
}

//private internal destruction function for swapchain
static void DestroySwapchain(SMWindow_Desktop_Swapchain* swapchain,
	VkDevice device, VmaAllocator allocator)
{
	//cleans up the render pass
	if (swapchain->renderpass != VK_NULL_HANDLE)
		vkDestroyRenderPass(device, swapchain->renderpass, NULL);

	//cleans up images and views
	for (uint32 i = 0; i < swapchain->imageCount; ++i)
	{
		if(swapchain->framebuffers[i] != VK_NULL_HANDLE)
			vkDestroyFramebuffer(device, swapchain->framebuffers[i], NULL);

		if (swapchain->depthView[i] != VK_NULL_HANDLE)
			SMGraphics_Util_ImageView_Destroy(swapchain->depthView[i], device);

		if (swapchain->depthImage[i] != VK_NULL_HANDLE)
			SMGraphics_Util_Image_Destroy(swapchain->depthImage[i], swapchain->depthImageMemoy[i], allocator);

		if (swapchain->imageViews[i] != VK_NULL_HANDLE)
			SMGraphics_Util_ImageView_Destroy(swapchain->imageViews[i], device);
	}

	if (swapchain->swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(device, swapchain->swapchain, NULL);
}

//private internal createion function for swapchain
static int CreateSwapchain(SMWindow_Desktop_Swapchain* swapchain,
	VkDevice device, VkPhysicalDevice gpu, const uint32 graphicsQueueIndex, const uint32 presentQueueIndex,
	GLFWwindow* window, VkSurfaceKHR surface, 
	VmaAllocator allocator, VkImageLayout imageLayoutOutputRenderPassSetting)
{
	//creates the swapchain
	VkSwapchainCreateInfoKHR createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = swapchain->imageCount;
	createInfo.imageFormat = swapchain->surfaceFormat.format;
	createInfo.imageColorSpace = swapchain->surfaceFormat.colorSpace;
	createInfo.imageExtent = swapchain->extents;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { graphicsQueueIndex, presentQueueIndex };
	if (graphicsQueueIndex != presentQueueIndex)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = NULL;
	}

	createInfo.preTransform = swapchain->capabilities.currentTransform;

	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = swapchain->presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain->swapchain) != VK_SUCCESS)
	{
		BTD_LogError("Smok Window", "Swapchain", "CreateSwapchain", "Failed to create swapchain!");
		return 0;
	}

	//gets the swapchain images
	uint32 imageCount = 0;
	vkGetSwapchainImagesKHR(device, swapchain->swapchain, &imageCount, NULL);
	vkGetSwapchainImagesKHR(device, swapchain->swapchain, &imageCount, swapchain->images);

	//creates the swapchain image views
	VkImageViewCreateInfo imageViewCreateInfo = SMGraphics_Util_ImageView_CreateInfo_Default();
	SMGraphics_Util_ImageView_CreateInfo_SetFormat(&imageViewCreateInfo, swapchain->surfaceFormat.format);
	SMGraphics_Util_ImageView_CreateInfo_SetAspectMask(&imageViewCreateInfo, VK_IMAGE_ASPECT_COLOR_BIT);
	for (uint32 i = 0; i < imageCount; ++i)
	{
		SMGraphics_Util_ImageView_CreateInfo_SetImage(&imageViewCreateInfo, swapchain->images[i]);
		if (!SMGraphics_Util_ImageView_Create(&swapchain->imageViews[i], &imageViewCreateInfo, device))
		{
			DestroySwapchain(swapchain, device, allocator);
			BTD_LogError("Smok Window", "Swapchain", "CreateSwapchain", "Failed to create swapchain image views!");
			return 0;
		}
	}

	//creates the depth images
	VkImageCreateInfo imageInfo = SMGraphics_Util_Image_CreateInfo_Default();
	SMGraphics_Util_Image_CreateInfo_SetFormat(&imageInfo, swapchain->depthFormat);
	SMGraphics_Util_Image_CreateInfo_SetSize(&imageInfo, swapchain->extents.width, swapchain->extents.height);
	SMGraphics_Util_Image_CreateInfo_SetUsage(&imageInfo, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	//creates depth image view
	imageViewCreateInfo = SMGraphics_Util_ImageView_CreateInfo_Default();
	SMGraphics_Util_ImageView_CreateInfo_SetFormat(&imageViewCreateInfo, swapchain->depthFormat);
	SMGraphics_Util_ImageView_CreateInfo_SetAspectMask(&imageViewCreateInfo, VK_IMAGE_ASPECT_DEPTH_BIT);

	for (uint32 i = 0; i < imageCount; ++i)
	{
		//creates depth image
		if (!SMGraphics_Util_Image_Create(&swapchain->depthImage[i], &swapchain->depthImageMemoy[i], &imageInfo, allocator))
		{
			DestroySwapchain(swapchain, device, allocator);
			BTD_LogError("Smok Window", "Swapchain", "CreateSwapchain", "Failed to create swapchain image views!");
			return 0;
		}

		//create depth image view
		SMGraphics_Util_ImageView_CreateInfo_SetImage(&imageViewCreateInfo, swapchain->depthImage[i]);
		if(!SMGraphics_Util_ImageView_Create(&swapchain->depthView[i], &imageViewCreateInfo, device))
		{
			DestroySwapchain(swapchain, device, allocator);
			BTD_LogError("Smok Window", "Swapchain", "CreateSwapchain", "Failed to create swapchain image views!");
			return 0;
		}
	}

	//generate color attachment
	VkAttachmentDescription colorAttachment = { 0 };
	colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	
	//if (info->isRenderingStraightToScreen == 1)
		//colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	//else
	//colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = imageLayoutOutputRenderPassSetting;

	VkAttachmentReference colorAttachmentRef = { 0 };
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//generate depth attachment
	VkAttachmentDescription depthAttachment = {0};
	depthAttachment.format = swapchain->depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {0};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//subpass
	VkSubpassDescription subpass = { 0 };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {0};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription _attachments[2] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = _attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, NULL, &swapchain->renderpass) != VK_SUCCESS)
	{
		DestroySwapchain(swapchain, device, allocator);
		BTD_LogError("Smok Graphics", "Render Pass", "SMGraphics_RenderPass_Create", "Failed to create render pass!");
		return 0;
	}

	//creates the frame buffer
	VkFramebufferCreateInfo framebufferInfo = {0};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = swapchain->renderpass;
	framebufferInfo.attachmentCount = 0;
	framebufferInfo.pAttachments = NULL;
	framebufferInfo.width = swapchain->extents.width;
	framebufferInfo.height = swapchain->extents.height;
	framebufferInfo.layers = 1;
	for (uint32 i = 0; i < imageCount; ++i)
	{
		VkImageView attachments[2] = {
		swapchain->imageViews[i], swapchain->depthView[i]
		};
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = attachments;
		
		if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapchain->framebuffers[i]) != VK_SUCCESS)
		{
			DestroySwapchain(swapchain, device, allocator);
			BTD_LogError("Smok Window", "Swapchain", "CreateSwapchain", "Failed to create swapchain frame buffer!");
			return 0;
		}
	}

	swapchain->imageLayoutOutputRenderPassSetting = imageLayoutOutputRenderPassSetting;

	return 1;
}

int SMWindow_Desktop_Swapchain_CreateSwapchain(SMWindow_Desktop_Swapchain* swapchain,
	VkPhysicalDevice gpu, VkDevice device, const uint32 graphicsQueueIndex, const uint32 presentQueueIndex,
	GLFWwindow* window, VkSurfaceKHR surface, VmaAllocator allocator, VkImageLayout imageLayoutOutputRenderPassSetting)
{
	//checks the settings
	if (!CheckSwapchainSettings(swapchain,
		gpu,
		window, surface))
	{
		BTD_LogError("Smok Window", "Swapchain", "SMWindow_Desktop_Swapchain_CreateSwapchain",
			"Failed to find desired present mode, screen format, and or depth formats!");
		return 0;
	}

	return CreateSwapchain(swapchain,
		device, gpu, graphicsQueueIndex, presentQueueIndex,
		window, surface,
		allocator, imageLayoutOutputRenderPassSetting);
}


//destroys a swapchain
void SMWindow_Desktop_Swapchain_DestroySwapchain(SMWindow_Desktop_Swapchain* swapchain,
	VkDevice device, VmaAllocator allocator)
{
	DestroySwapchain(swapchain, device, allocator);
}

//remakes a swapchain
int SMWindow_Desktop_Swapchain_RemakeSwapchain(SMWindow_Desktop_Swapchain* swapchain,
	VkDevice device, VkPhysicalDevice gpu, const uint32 graphicsQueueIndex, const uint32 presentQueueIndex,
	GLFWwindow* window, VkSurfaceKHR surface, VmaAllocator allocator)
{
	vkDeviceWaitIdle(device);

	BTD_Math_I32Vec2 size;
	GetSwapchainSize(window, &size);
	swapchain->extents = Smok_Util_Typepun(size, VkExtent2D);

	DestroySwapchain(swapchain, device, allocator);
	return CreateSwapchain(swapchain, device, gpu, graphicsQueueIndex, presentQueueIndex, window, surface, allocator, swapchain->imageLayoutOutputRenderPassSetting);
}

//creates a window
int SMWindow_Desktop_Window_Create(SMWindow_Desktop_Window* window, SMWindow_Desktop_Window_WindowCreateInfo* info, VkInstance vulkanInstance)
{
	window->size = info->size;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window->window = glfwCreateWindow(info->size.x, info->size.y, info->windowName, NULL, NULL);
	if (!window->window)
	{
		BTD_LogError("Framework Test", "Smok Graphics Window", "main", "Failed to create a window!");
		return 0;
	}

	if (info->onWindowCloseCallback)
		glfwSetWindowCloseCallback(window->window, info->onWindowCloseCallback);
	if (info->onWindowResizeCallback)
		glfwSetFramebufferSizeCallback(window->window, info->onWindowResizeCallback);
	if (info->onWindowKeyboardInputCallback)
		glfwSetKeyCallback(window->window, info->onWindowKeyboardInputCallback);

	glfwCreateWindowSurface(vulkanInstance, window->window, NULL, &window->surface);

	window->isRunning = 1;
	glfwSetWindowUserPointer(window->window, window);

	return 1;
}

//shutsdown a display
void SMWindow_Desktop_Window_ShutdownWindow(SMWindow_Desktop_Window* window)
{
	if (!window->isRunning)
		return;

	glfwSetWindowShouldClose(window->window, 1);
	window->isRunning = 0;
}

//destroys a window
void SMWindow_Desktop_Window_DestroyWindow(SMWindow_Desktop_Window* window, VkInstance vulkanInstance)
{
	SMWindow_Desktop_Window_ShutdownWindow(window);

	vkDestroySurfaceKHR(vulkanInstance, window->surface, NULL);
	glfwDestroyWindow(window->window); window->window = NULL;
}