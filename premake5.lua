include "C:\\SmokSDK\\SmokGraphics"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "SmokWindow"
kind "StaticLib"
language "C++"

targetdir ("bin/" .. outputdir .. "/%{prj.name}")
objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

files 
{
    "includes/**.h",
    "src/**.c",
    "includes/**.hpp",
    "src/**.cpp",
}

includedirs
{
    "includes",
    
    --"C:\\SmokSDK\\Libraries\\BTD-Libs\\yaml-cpp\\include",
    "C:\\SmokSDK\\Libraries\\BTD-Libs\\glm",
    "C:\\SmokSDK\\Libraries\\BTD-Libs\\glfw\\include",
    
    "C:\\VulkanSDK\\1.3.275.0\\Include",
    "C:\\SmokSDK\\Libraries\\SmokGraphics-Lib\\VulkanMemoryAllocator\\include",

    "C:\\SmokSDK\\BTDSTD\\BTDSTD\\includes",
    "C:\\SmokSDK\\BTDSTD\\BTDSTD_C\\includes",
    
    "C:\\SmokSDK\\SmokGraphics\\includes"
}

links
{
    "SmokGraphics",
    "GLFW",
    "C:\\VulkanSDK\\1.3.275.0\\Lib\\vulkan-1.lib",
}
                
defines
{
    "GLM_FORCE_RADIANS",
    "GLM_FORCE_DEPTH_ZERO_TO_ONE",
    "GLM_ENABLE_EXPERIMENTAL"
}
                
flags
{
    "NoRuntimeChecks",
    "MultiProcessorCompile"
}

--platforms
filter "system:windows"
cppdialect "C++17"
cdialect "C99"
staticruntime "On"
systemversion "latest"

defines
{
    "Window_Build",
    "Desktop_Build"
}

--configs
filter "configurations:Debug"
defines "DEBUG"
symbols "On"

filter "configurations:Release"
defines "RELEASE"
optimize "On"

filter "configurations:Dist"
defines "DIST"
optimize "On"

defines
{
    "NDEBUG"
}