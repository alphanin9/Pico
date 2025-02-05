#pragma once

// Precompiled header
#include <Shared/Common.hpp>
#include <Shared/Disassembler/Disassembler.hpp>
#include <Shared/EnvironmentIntegrity/CodeIntegrity.hpp>
#include <Shared/EnvironmentIntegrity/DriverPaths.hpp>
#include <Shared/EnvironmentIntegrity/SecureBoot.hpp>
#include <Shared/EnvironmentIntegrity/SignatureVerifier.hpp>
#include <Shared/MemoryUtils/AOBPattern.hpp>
#include <Shared/MemoryUtils/MemUtils.hpp>
#include <Shared/PE/PE.hpp>
#include <Shared/ProcessEnv/MemoryEnv.hpp>
#include <Shared/ProcessEnv/ProcessEnv.hpp>
#include <Shared/StdlibTypeDefinitions.hpp>
#include <Shared/SystemEnv/HandleEnumerator.hpp>
#include <Shared/SystemEnv/ProcessEnumerator.hpp>
#include <Shared/Util/Alignment.hpp>
#include <Shared/Util/FsUtils.hpp>
#include <Shared/Util/Hashing.hpp>
#include <Shared/Util/MsTaken.hpp>
#include <Shared/Util/NonCopyable.hpp>
#include <Shared/Util/NtdllCalls.hpp>
#include <Shared/Util/StaticString.hpp>
#include <Shared/Util/StringConverter.hpp>
#include <Shared/Util/StringUtils.hpp>
