#pragma once

#include "Core/ExternalVideoExport.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace mw {

#ifdef _WIN32

bool RunOwnedProcess(const std::filesystem::path& executable,
                     const std::vector<std::wstring>& arguments,
                     const std::function<bool()>& cancellationCallback,
                     const std::function<void(std::string_view)>& stdoutChunkCallback,
                     ExternalProcessResult& result,
                     std::string& error,
                     std::uint32_t timeoutMilliseconds = 0U);

#endif

} // namespace mw
