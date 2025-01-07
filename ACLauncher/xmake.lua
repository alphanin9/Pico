add_requires("bddisasm")

target("AC launcher")
    set_default(true)
    set_kind("binary")
    add_cxxflags("/MP /GR- /EHsc")
    add_cxxflags("/Zi /Ob2 /Oi")

    add_files("src/**.cpp")
    add_deps("ACShared")
    add_packages("bddisasm", "hopscotch-map", "simdutf", "tiltedcore", "wil")
    add_syslinks("Ntdll", "Kernel32", "User32", "Version")

add_rules("plugin.vsxmake.autoupdate")