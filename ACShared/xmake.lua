add_requires("lazy_importer", "openssl")

target("ACShared")
    set_kind("static")
    set_warnings("all")
    
    add_files("src/**.cpp")
    add_includedirs("src/", { public = true })
    add_headerfiles("src/**.hpp")
    
    add_defines("NOMINMAX")
    add_deps("LinuxPE")
    add_packages(
        "hopscotch-map", 
        "lazy_importer", 
        "openssl", 
        "simdutf", 
        "tiltedcore", 
        "wil", 
        "zydis"
    )

    add_syslinks("Kernel32", "Ncrypt", "Ntdll", "User32", "Version", "Wintrust")

    set_pcxxheader("src/Shared/Pico.hpp")

add_rules("plugin.vsxmake.autoupdate")