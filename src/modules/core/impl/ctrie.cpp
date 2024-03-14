//
// Created by maxwe on 2024-03-12.
//

#include "config.hpp"
#include "ctrie.hpp"

#include "agate/atomic.hpp"

#include "core/attr.hpp"


#include <bit>


namespace agt::impl {

  // bounded strlen implementation.
  bool strlen_safe(const char* str, size_t maxLength, size_t& result) noexcept {
#if 0
    for (size_t i = 0; i < maxLength; ++i) {
      if (str[i] == '\0') {
        result = i;
        return true;
      }
    }
    return false;
#else

    constexpr static auto get_zero_byte_mask = [](uintptr_t bytes) {
      return ((bytes - 0x0101010101010101ULL) & ~bytes & 0x8080808080808080ULL);
    };

    const uintptr_t* alignedStr;
    auto alignedEnd = reinterpret_cast<const uintptr_t*>(reinterpret_cast<uintptr_t>(str + maxLength + 7) & -0x7);
    size_t initialOffset = reinterpret_cast<uintptr_t>(str) & 0x7;


    uintptr_t qword;
    uintptr_t byteMask;



    if (initialOffset != 0) {
      alignedStr = reinterpret_cast<const uintptr_t*>(reinterpret_cast<uintptr_t>(str) & -0x7);
      qword = *alignedStr;
      qword |= ~(static_cast<uintptr_t>(-1) << (initialOffset * 8));
    }
    else {
      alignedStr = reinterpret_cast<const uintptr_t*>(str);
      qword      = *alignedStr;
    }


    do {
      byteMask = get_zero_byte_mask(qword);
      if (byteMask != 0)
        break;
      ++alignedStr;
      if (alignedStr == alignedEnd)
        return false;
      qword = *alignedStr;
    } while ( true );

    size_t totalLength = (reinterpret_cast<const char*>(alignedStr) - str) + (std::countr_zero(byteMask) / 8);

    if (totalLength > maxLength) [[unlikely]]
      return false;

    result = totalLength;
    return true;
#endif
  }


  // A registry entry contains:
  //   -

  // This is a ctrie that maps strings to objects. This is the shared registry

  class ctrie {

    uint64_t hash_string(agt_ctx_t ctx, agt_u32_t length, const char* val) noexcept {

    }

    struct inode;
    struct cnode;
    struct snode;

    using branch = uintptr_t;

    struct inode {
      cnode* node;
    };

    struct cnode {
      uint64_t bitmap;
      branch*  array;
    };

    struct snode {
      agt_u32_t refCount;
      agt_u32_t keyLength;
      object*   obj;
      char      keyData[];
    };

    struct insert_state {
      agt_string_t name;
      snode**      pResult;
    };


    static cnode* cnodeInsert(cnode* target, void*& mem, size_t& memSize) noexcept {

    }

    static bool try_insert(inode& in, agt_ctx_t ctx, agt_u64_t hash, size_t depth, insert_state& state) noexcept {

    }

    static agt_status_t do_insert(inode& root, agt_ctx_t ctx, agt_u64_t hash, size_t length, const char* val, snode*& result) noexcept {

      insert_state state{
        .name = {
          .data   = val,
          .length = length
        },
        .pResult = &result
      };
      if (!try_insert(root, ctx, hash, 0, state))

    }



  public:

    agt_status_t reserve(agt_ctx_t ctx, const agt_name_desc_t& nameDesc, agt_name_result_t& result) noexcept {
      const auto nameData = nameDesc.name.data;
      if (!nameData)
        return AGT_ERROR_INVALID_ARGUMENT;
      size_t nameLength;
      if (nameDesc.name.length != 0)
        nameLength = nameDesc.name.length;
      else if (!strlen_safe(nameDesc.name.data, attr::max_name_length(get_instance(ctx)), nameLength))
        return AGT_ERROR_NAME_TOO_LONG;

      const auto hash = hash_string(ctx, nameLength, nameDesc.name.data);
      snode* result;
      auto status = do_insert(m_root, ctx, hash, nameLength, nameDesc.name.data, result);
      if (status == AGT_ERROR_NAME_ALREADY_IN_USE) {
        // Do something lmao
      }

      return status;
    }

  private:
    inode m_root {
      nullptr
    };
  };

}


namespace agt {

  impl::ctrie& get_ctrie(agt_ctx_t ctx) noexcept;

  agt_status_t AGT_stdcall reserve_name_local(agt_ctx_t ctx, const agt_name_desc_t* pNameDesc, agt_name_result_t* pResult) {
    if (!pNameDesc || !pResult || !pNameDesc->name.data)
      return AGT_ERROR_INVALID_ARGUMENT;
    return get_ctrie(ctx).reserve(ctx, *pNameDesc, *pResult);
  }
}