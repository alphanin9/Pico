-- Set up shared defines for all project members
set_languages("c++latest")

set_plat("windows")
set_arch("x64")
set_runtimes("MD")
set_symbols("debug")
set_strip("all")
set_optimize("fastest")
set_toolchains("clang-cl")

-- Shared requirements
add_requires("hopscotch-map", "simdutf", "tiltedcore", "wil", "zydis")
add_cxxflags("/GR-")
add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")

includes("deps")
includes("ACShared")
includes("ACLauncher")
includes("ACClient")