//
// Created by maxwe on 2022-08-17.
//

#include "init.hpp"

#define PRIVATE_CONTEXT_ENVVAR              "AGATE_PRIVATE_CONTEXT"
#define SHARED_NAMESPACE_ENVVAR             "AGATE_SHARED_NAMESPACE"
#define CHANNEL_DEFAULT_CAPACITY_ENVVAR     "AGATE_CHANNEL_DEFAULT_CAPACITY"
#define CHANNEL_DEFAULT_MESSAGE_SIZE_ENVVAR "AGATE_CHANNEL_DEFAULT_MESSAGE_SIZE"
#define CHANNEL_DEFAULT_TIMEOUT_MS_ENVVAR   "AGATE_CHANNEL_DEFAULT_TIMEOUT_MS"


#define AGT_DEFAULT_ENABLE_SHARED_LIB true
#define AGT_DEFAULT_SHARED_NAMESPACE "agate-default-namespace"
#define AGT_DEFAULT_DEFAULT_CAPACITY 255
#define AGT_DEFAULT_DEFAULT_MESSAGE_SIZE 196
#define AGT_DEFAULT_DEFAULT_TIMEOUT_MS 5000


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


#include <string_view>
#include <optional>
#include <charconv>

namespace {
  agt_u32_t getAsyncSizeFromHeaderVersion(int headerVersion) noexcept {
    (void)headerVersion;
    return 40;
  }

  agt_u32_t getSignalSizeFromHeaderVersion(int headerVersion) noexcept {
    (void)headerVersion;
    return 24;
  }

  std::optional<bool>      parseBool(std::string_view str) noexcept {
    if (str.empty())
      return std::nullopt;
    switch(str.front()) {
      case '0':
        if (str.size() > 1)
          return std::nullopt;
        else
          return false;
      case '1':
        if (str.size() > 1)
          return std::nullopt;
        else
          return true;
      case 'F':
        if (str == "FALSE")
          return false;
        else
          return std::nullopt;
      case 'N':
        if (str == "NO")
          return false;
        else
          return std::nullopt;
      case 'T':
        if (str == "TRUE")
          return true;
        else
          return std::nullopt;
      case 'Y':
        if (str == "YES")
          return true;
        else
          return std::nullopt;
      case 'f':
        if (str == "false")
          return false;
        else
          return std::nullopt;
      case 'n':
        if (str == "no")
          return false;
        else
          return std::nullopt;
      case 't':
        if (str == "true")
          return true;
        else
          return std::nullopt;
      case 'y':
        if (str == "yes")
          return true;
      default:
        return std::nullopt;
    }
  }

  std::optional<agt_u32_t> parseUInt(std::string_view str) noexcept {
    agt_u32_t val;
    auto first = str.data();
    auto last  = first + str.size();
    auto [end, err] = std::from_chars(first, last, val);
    if (end != last || err == std::errc::result_out_of_range)
      return std::nullopt;
    return val;
  }
}

void agt::getInitOptions(ctx_init_options& opt, int headerVersion) noexcept {
  opt.sharedCtxNamespace.clear();
  const DWORD bufLen = 256;
  char envVarBuf[bufLen];
  std::string_view value;

  auto getEnvVar = [&](const char* envVarName) mutable {
    DWORD valLength = GetEnvironmentVariable(envVarName, envVarBuf, bufLen);
    value = { envVarBuf, valLength };
    return valLength != 0;
  };

  // std::memset(&opt, 0, sizeof(ctx_init_options)); // Undefined behaviour because there's a member that's a vector, so

  opt.apiHeaderVersion = headerVersion;
  opt.asyncStructSize  = getAsyncSizeFromHeaderVersion(headerVersion);
  opt.signalStructSize = getSignalSizeFromHeaderVersion(headerVersion);

  size_t valueCount;
  if (getEnvVar(PRIVATE_CONTEXT_ENVVAR)) {
    // If the variable is defined to be empty or to be an invalid value,
    // default to disabling the shared libraries anyways. Applications
    // that absolutely require IPC capabilities can manually set this
    // environment variable to false before initializing agate so as to
    // overwrite whatever value the program may have been started with.
    opt.sharedLibIsEnabled = !parseBool(value).value_or(true);
  }
  else {
    opt.sharedLibIsEnabled = AGT_DEFAULT_ENABLE_SHARED_LIB;
    // DWORD err = GetLastError();
    // if (err == ERROR_ENVVAR_NOT_FOUND)

    assert(GetLastError() == ERROR_ENVVAR_NOT_FOUND); // Idk what else to do
  }

  if (!getEnvVar(SHARED_NAMESPACE_ENVVAR) || value.empty()) {
    value = std::string_view{AGT_DEFAULT_SHARED_NAMESPACE};
  }
  opt.sharedCtxNamespace.assign(value.begin(), value.end());


  if (getEnvVar(CHANNEL_DEFAULT_CAPACITY_ENVVAR))
    opt.channelDefaultCapacity = parseUInt(value).value_or(AGT_DEFAULT_DEFAULT_CAPACITY);
  else
    opt.channelDefaultCapacity = AGT_DEFAULT_DEFAULT_CAPACITY;

  if (getEnvVar(CHANNEL_DEFAULT_MESSAGE_SIZE_ENVVAR))
    opt.channelDefaultMessageSize = parseUInt(value).value_or(AGT_DEFAULT_DEFAULT_MESSAGE_SIZE);
  else
    opt.channelDefaultMessageSize = AGT_DEFAULT_DEFAULT_CAPACITY;

  if (getEnvVar(CHANNEL_DEFAULT_TIMEOUT_MS_ENVVAR))
    opt.defaultTimeoutMs = parseUInt(value).value_or(AGT_DEFAULT_DEFAULT_TIMEOUT_MS);
  else
    opt.defaultTimeoutMs = AGT_DEFAULT_DEFAULT_TIMEOUT_MS;
}