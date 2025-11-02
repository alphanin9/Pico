target("LinuxPE")
    set_default(false)
    set_warnings("none")
    set_arch("x64")
    set_kind("headeronly")
    set_group("deps")
    add_headerfiles("linux-pe/includes/**.hpp", "linux-pe/includes/linuxpe")
    add_includedirs("linux-pe/includes/", { public = true })
    add_cxflags("/GR-")

-- Note: can add more stripped packages here as well! Like stripping down Asmjit's unnecessary features