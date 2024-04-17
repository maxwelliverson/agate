//
// Created by maxwe on 2023-08-23.
//

#include "dynamic_code.hpp"

#include "dictionary.hpp"

namespace agt {
}// namespace agt

struct function_info {

};


struct location {
  function_info* function;
  uint64_t       offset;
};


struct label_info {
  location location;
  agt::vector<struct location> refs;
};

class agt::dc::x64::module_builder::impl {

};

class agt::dc::x64::function_builder::impl {
  module_builder::impl*     module;
  function_info*            info;
  agt::vector<std::byte, 0> buffer;

public:

  [[nodiscard]] location current() const noexcept {
    return { info, buffer.size() };
  }

  void reserve(uint32_t bytes) noexcept {
    buffer.reserve(buffer.size() + bytes);
  }

  void put(uint32_t byte) noexcept {
    assert( !(byte & ~0xFF));
    buffer.push_back(std::byte(byte));
  }

  void putREX(uint32_t byte) noexcept {
    if (byte != 0)
      put(byte);
  }

  void putSizeOverload(bool sizeIsOverloaded) noexcept {
    if (sizeIsOverloaded)
      put(0x66);
  }
};



class agt::dc::function::impl {
  uint32_t entryOffset;
  uint32_t codeSize;
public:

};


class agt::dc::module::impl {
  dictionary<function::impl*> functions;
  void*                       baseAddress;
  size_t                      size;
};

enum reg_size {
  byte,
  word,
  dword,
  qword
};

enum modrm_mode {
  rm_memory,
  rm_reg = 0x3
};

inline static reg_size get_size(agt::dc::x64::gp_reg reg) noexcept {
  return static_cast<reg_size>(std::to_underlying(reg) >> 4);
}

inline static bool is_extended(agt::dc::x64::gp_reg reg) noexcept {
  return std::to_underlying(reg) & 0x8;
}

inline static uint32_t get_reg_index(agt::dc::x64::gp_reg reg) noexcept {
  return static_cast<uint32_t>(std::to_underlying(reg) & 0x7);
}

inline static uint32_t get_modrm(uint32_t& rexPrefix, modrm_mode mode, agt::dc::x64::gp_reg reg, agt::dc::x64::gp_reg rm) noexcept {
  if (is_extended(reg))
    rexPrefix |= 0x44;
  if (is_extended(rm))
    rexPrefix |= 0x41;
  return (static_cast<uint32_t>(mode) << 6) | (get_reg_index(reg) << 3) | get_reg_index(rm);
}

using namespace agt::dc::x64;



function_builder& function_builder::mov(gp_reg dst, gp_reg src) noexcept {
  const auto dstSize = get_size(dst);
  assert( dstSize != get_size(src) );
  uint32_t opCode;
  uint32_t rexPrefix = 0;
  uint32_t modRM = get_modrm(rexPrefix, rm_reg, src, dst);
  bool     hasSizeOverloadByte = false;

  switch (dstSize) {
    case byte:
      opCode = 0x88;
      break;
    case word:
      opCode = 0x89;
      hasSizeOverloadByte = true;
      break;
    case dword:
      opCode = 0x89;
      break;
    case qword:
      opCode = 0x89;
      rexPrefix |= 0x48;
      break;
  }
  pImpl->putSizeOverload(hasSizeOverloadByte);
  pImpl->putREX(rexPrefix);
  pImpl->put(opCode);
  pImpl->put(modRM);

  return *this;
}

function_builder& function_builder::mov(gp_reg dst, uint64_t imm) noexcept {
  return *this;
}

function_builder& function_builder::load(gp_reg dst, const void* address) noexcept {
  return *this;
}

function_builder& function_builder::load(gp_reg dst, gp_reg srcAddress, size_t offset) noexcept {
  return *this;
}

function_builder& function_builder::loadIndexed(gp_reg dst, const void* address, gp_reg index, int stride) noexcept {
  return *this;
}

function_builder& function_builder::loadIndexed(gp_reg dst, const void* address, size_t offset, gp_reg index, int stride) noexcept {
  return *this;
}

function_builder& function_builder::store(void* address, gp_reg src) noexcept {
  return *this;
}

function_builder& function_builder::store(gp_reg dstAddress, gp_reg src, size_t offset) noexcept {
  return *this;
}

function_builder& function_builder::storeIndexed(void* address, gp_reg src, gp_reg index, int stride) noexcept {
  return *this;
}

function_builder& function_builder::storeIndexed(gp_reg dstAddress, gp_reg src, size_t offset, gp_reg index, int stride) noexcept {
  return *this;
}



function_builder& function_builder::add(gp_reg dst, gp_reg src) noexcept {
  return *this;
}
function_builder& function_builder::add(gp_reg dst, uint64_t imm) noexcept {
  return *this;
}

function_builder& function_builder::imul(gp_reg dst, gp_reg src) noexcept {
  return *this;
}
function_builder& function_builder::imul(gp_reg dst, uint64_t imm) noexcept {
  return *this;
}

function_builder& function_builder::idiv(gp_reg dst, gp_reg src) noexcept {
  return *this;
}
function_builder& function_builder::idiv(gp_reg dst, uint64_t imm) noexcept {
  return *this;
}

function_builder& function_builder::shiftRight(gp_reg dst, gp_reg src) noexcept {
  return *this;
}
function_builder& function_builder::shiftRight(gp_reg dst, uint32_t imm) noexcept {
  return *this;
}

function_builder& function_builder::shiftLeft(gp_reg dst, gp_reg src) noexcept {
  return *this;
}
function_builder& function_builder::shiftLeft(gp_reg dst, uint32_t imm) noexcept {
  return *this;
}

function_builder& function_builder::popcount(gp_reg dst, gp_reg src) noexcept {
  return *this;
}


function_builder& function_builder::push(gp_reg src) noexcept {
  const auto regSize = get_size(src);
  assert( regSize == qword || regSize == word );
  pImpl->putSizeOverload(regSize == word);
  pImpl->putREX(is_extended(src) ? 0x41 : 0);
  pImpl->put(0x50 + get_reg_index(src));
  return *this;
}
function_builder& function_builder::pop(gp_reg dst) noexcept {
  const auto regSize = get_size(dst);
  assert( regSize == qword || regSize == word );
  pImpl->putSizeOverload(regSize == word);
  pImpl->putREX(is_extended(dst) ? 0x41 : 0);
  pImpl->put(0x58 + get_reg_index(dst));
  return *this;
}