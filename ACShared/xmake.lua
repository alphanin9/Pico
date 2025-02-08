add_requires("lazy_importer", "picosha2")

target("ACShared")
    set_kind("static")
    set_warnings("all")
    set_exceptions("no-cxx")
    
    add_files("src/**.cpp")
    add_includedirs("src/", { public = true })
    add_headerfiles("src/**.hpp")
    
    add_defines("NOMINMAX")
    add_deps("LinuxPE")
    add_packages(
        "hopscotch-map", 
        "lazy_importer", 
        "picosha2",
        "simdutf",
        "tiltedcore",
        "wil",
        "zydis"
    )

    add_syslinks("Kernel32", "Ncrypt", "Ntdll", "User32", "Version", "Wintrust")
    set_pcxxheader("src/Shared/Pico.hpp")
    set_policy("build.optimization.lto", true)

add_rules("plugin.vsxmake.autoupdate")