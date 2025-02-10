target("ACLauncher")
    set_default(true)
    set_kind("binary")
    set_warnings("all")

    add_files("src/**.cpp")
    add_deps("ACShared")
    add_packages(
        "hopscotch-map", 
        "tiltedcore", 
        "wil", 
        "zydis"
    )
    add_syslinks("Ntdll", "Kernel32", "User32", "Version")
    add_cxflags("/GR-")

add_rules("plugin.vsxmake.autoupdate")