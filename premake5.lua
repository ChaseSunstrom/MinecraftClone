workspace "MinecraftClone"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "MinecraftClone"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "MinecraftClone"
    location "MinecraftClone"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("obj/" .. outputdir .. "/%{prj.name}")

    files {
        "MinecraftClone/src/**.h",
        "MinecraftClone/src/**.c",
        "MinecraftClone/src/**.cpp",
        "MinecraftClone/src/**.hpp"
    }

    includedirs {
        "include",
    }

    libdirs {
        "lib"
    }

    links {
        "opengl32.lib",
        "lib/glfw3.lib",
        "lib/glew32s.lib"
    }

    filter "configurations:Debug"
    defines { "DEBUG", "_ITERATOR_DEBUG_LEVEL=2", "GLEW_STATIC" }
    runtime "Debug"
    symbols "on"
        
filter "configurations:Release"
    defines { "NDEBUG", "_ITERATOR_DEBUG_LEVEL=0", "GLEW_STATIC" }
    runtime "Release"
    optimize "on"
