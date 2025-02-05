add_requires("zydis")

target("LinuxPE")
    set_default(false)
    set_warnings("none")
    set_arch("x64")
    set_kind("headeronly")
    set_group("deps")
    add_headerfiles("linux-pe/includes/**.hpp", "linux-pe/includes/linuxpe")
    add_includedirs("linux-pe/includes/", { public = true })

target("safetyhook")
    set_default(false)
    set_warnings("none")
    set_kind("static")
    set_group("deps")
    -- NOTE: The .asm files are there to paste the machine code of into src/mid_hook.cpp
    add_files("safetyhook/src/**.cpp")
    add_headerfiles("safetyhook/include/**.hpp")
    add_includedirs("safetyhook/include/", { public = true })
    add_packages("zydis")

