#pragma once
// STL
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

// TiltedCore and deps
#include <TiltedCore/Stl.hpp>
#include <tsl/hopscotch_map.h>
#include <tsl/hopscotch_set.h>

// Generated headers and PHNT-made ones
#include <Shared/Windows/NtExApi.hpp>
#include <Shared/Windows/NtMemApi.hpp>
#include <Shared/Windows/NtMisc.hpp>
#include <Shared/Windows/NtProcApi.hpp>
#include <Shared/Windows/Ntdll.hpp>

// General Windows API
#include <Windows.h>

// Crypto and signature verification
#include <SoftPub.h>
#include <WinTrust.h>
#include <mscat.h>
#include <ncrypt.h>
#include <wincrypt.h>

// WIL
// Note: this HAS to be 1st
#include <wil/stl.h>
#include <wil/resource.h>
#include <wil/filesystem.h>
#include <wil/win32_helpers.h>

// LinuxPE
#include <coff/image.hpp>
#include <nt/image.hpp>
