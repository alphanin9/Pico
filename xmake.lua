-- Set up shared defines for all project members
set_languages("c++latest")
set_warnings("all")

set_arch("x64")
set_runtimes("MT")
set_symbols("debug")
set_strip("all")
set_optimize("fastest")

-- Shared requirements
add_requires("hopscotch-map", "simdutf", "tiltedcore", "wil")

add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")

includes("deps")
includes("ACShared")
includes("ACLauncher")
includes("ACClient")