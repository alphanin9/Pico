target("AC launcher")
    set_default(true)
    set_kind("binary")

    add_files("src/**.cpp")
    add_deps("ACShared")
    add_packages(
        "hopscotch-map", 
        "simdutf", 
        "tiltedcore", 
        "wil", 
        "zydis"
    )
    add_syslinks("Ntdll", "Kernel32", "User32", "Version")

add_rules("plugin.vsxmake.autoupdate")