add_requires("asmjit", "base-n", "lazy_importer", "spdlog", "stduuid", "thread-pool")

target("ACClient")
    set_kind("shared")
    set_warnings("all")
    
    add_files("src/**.cpp")
    add_headerfiles("src/**.hpp")
    add_includedirs("src/")
    add_deps("ACShared", "LinuxPE", "safetyhook")
    add_defines(
        "BS_THREAD_POOL_ENABLE_NATIVE_HANDLES", 
        "BS_THREAD_POOL_ENABLE_PRIORITY", 
        "BS_THREAD_POOL_NATIVE_EXTENSIONS", 
        "UUID_SYSTEM_GENERATOR"
    )
    add_cxflags("/GR-")
    add_packages(
        "asmjit",
        "base-n",
        "hopscotch-map",
        "lazy_importer",
        "spdlog",
        "stduuid",
        "thread-pool",
        "tiltedcore",
        "wil",
        "zydis"
    )
    add_syslinks("Ntdll", "Kernel32", "Ole32", "User32", "Version")
    set_policy("build.optimization.lto", true)

add_rules("plugin.vsxmake.autoupdate")