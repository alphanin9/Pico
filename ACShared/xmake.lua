add_requires("bddisasm", "lazy_importer")

target("ACShared")
    set_kind("static")
    add_files("src/**.cpp")
    add_includedirs("src/", { public = true })
    add_headerfiles("src/**.hpp")
    add_syslinks("Kernel32", "Ncrypt", "Ntdll", "User32", "Version", "Wintrust")
    add_defines("NOMINMAX")
    add_deps("LinuxPE")
    add_packages("bddisasm", "hopscotch-map", "lazy_importer", "simdutf", "tiltedcore", "wil")
    set_pcxxheader("src/Shared/Pico.hpp")

add_rules("plugin.vsxmake.autoupdate")