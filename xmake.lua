-- Set up shared defines for all project members
set_languages("c++23")

set_plat("windows")
set_arch("x64")
set_runtimes("MD")
set_symbols("debug")
set_strip("all")
set_optimize("smallest")
set_exceptions("no-cxx")

-- Shared requirements
add_requires("hopscotch-map", "tiltedcore", "wil", "zydis")
add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")

includes("deps")
includes("ACShared")
includes("ACLauncher")
includes("ACClient")