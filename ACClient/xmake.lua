add_requires("base-n", "spdlog", "stduuid", "thread-pool")

target("ACClient")
    set_kind("shared")
    add_cxxflags("/Zi /Ob2 /Oi")

    add_files("src/**.cpp")
    add_files("src/**.asm")
    add_headerfiles("src/**.hpp")
    add_includedirs("src/")
    add_deps("ACShared", "LinuxPE", "safetyhook")
    add_defines("BS_THREAD_POOL_ENABLE_NATIVE_HANDLES", "BS_THREAD_POOL_ENABLE_PRIORITY", "BS_THREAD_POOL_NATIVE_EXTENSIONS", "UUID_SYSTEM_GENERATOR")
    add_packages("base-n", "hopscotch-map", "simdutf", "spdlog", "stduuid", "thread-pool", "tiltedcore", "wil", "zydis")
    add_syslinks("Ntdll", "Kernel32", "Ole32", "User32", "Version")

add_rules("plugin.vsxmake.autoupdate")