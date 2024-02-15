//
// Created by maxwe on 2022-12-18.
//

#ifndef AGATE_INTERNAL_MODULE_HPP
#define AGATE_INTERNAL_MODULE_HPP

#include "config.hpp"
#include "dllexport.hpp"

#include "sys_string.hpp"
#include "version.hpp"
#include "align.hpp"

#include "dictionary.hpp"

namespace agt {

  using function_ptr = void(*)();

  // Make sure this is in sync with the module bits of agt_init_flag_bits_t in core.h
  enum module_id : agt_u32_t {
    AGT_CORE_MODULE     = 0x0,
    AGT_AGENTS_MODULE   = 0x1,
    AGT_ASYNC_MODULE    = 0x2,
    AGT_CHANNELS_MODULE = 0x4,
    AGT_MODULE_MAX_ENUM_PLUS_ONE,
  };

  inline constexpr const char* module_names[] = {
      "core",
      "agents",
      "async",
      "channels"
  };

  static_assert( is_pow2(AGT_MODULE_MAX_ENUM_PLUS_ONE - 1) );

  inline constexpr size_t MaxModuleCount = std::countr_zero(AGT_MODULE_MAX_ENUM_PLUS_ONE - 1) + 2;

  inline constexpr size_t module_index(module_id id) noexcept {
    AGT_if_not_consteval {
      AGT_assert( std::has_single_bit(static_cast<std::underlying_type_t<module_id>>(id)) && id < AGT_MODULE_MAX_ENUM_PLUS_ONE );
    }
    if (id == AGT_CORE_MODULE)
      return 0;
    return std::countr_zero(static_cast<std::underlying_type_t<module_id>>(id)) + 1;
  }


  struct dispatch_flags {
    std::string_view flags;
  };



  // TODO: this was implemented for my original method of dynamic linking, whereby
  //       exported functions would be named such that there is a public name followed
  //       by a suffix indicating the dispatch flags that would be used to select a
  //       specific function to export to the public name (ex. the name function named
  //       agate_api_pool_alloc__st would be the single-threaded implementation of
  //       agt_pool_alloc, and agate_api_pool_alloc__mt would be the function selected
  //       by multi-threaded implementations. I ultimately deemed this to be way too
  //       complicated to be practical, and also not as future proof, as it would not be
  //       possible to link to functions (or dispatch on flags) the static loader is not aware of.
  //       I will instead simply have each module export a function that takes the
  //       configuration settings being used to initialize the library, and declares all
  //       of the functions it exports, along with the addresses of specific implmentations
  //       selected for the given config settings.
  //       I need to update the module_accessor class to reflect the new style of initialization/linking
  class module_accessor {
    inline constexpr static size_t MaxNameLength = 20;
    inline constexpr static size_t MaxPathLength = 260;

    struct function_family {
      function_ptr*   entryAddresses;
      dispatch_flags* entryFlags;
      size_t          entryCount;
    };

    struct function_info {
      function_ptr     address       = {};
      std::string_view dispatchFlags = {};
    };

    class module_walker {
      char*                           imageBase;
      PIMAGE_NT_HEADERS               header;
      std::span<IMAGE_SECTION_HEADER> sections;

      static PIMAGE_NT_HEADERS               get_nt_headers(void* base) noexcept {
        WORD signature;
        std::memcpy(&signature, base, 2);
        if (signature == 0x5A4D) {
          auto dosHeader = static_cast<PIMAGE_DOS_HEADER>(base);
          auto ntHeaderOffset = dosHeader->e_lfanew;
          return reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<char*>(base) + ntHeaderOffset);
        }
        else {
          DWORD ntSignature;
          std::memcpy(&ntSignature, base, 4);
          if (ntSignature == 0x50450000)
            return static_cast<PIMAGE_NT_HEADERS>(base);
          AGT_assert(ntSignature == 0x50450000);
          return nullptr; // Uh oh...
        }
      }

      static std::span<IMAGE_SECTION_HEADER> get_sections(PIMAGE_NT_HEADERS ntHeader) noexcept {
        WORD sectionCount;
        sectionCount = ntHeader->FileHeader.NumberOfSections;
        constexpr static size_t ImageSize = sizeof(IMAGE_NT_HEADERS);
        constexpr static size_t OImageSIze = 0x18 + 0x60 + (16*8);
        return { (PIMAGE_SECTION_HEADER)(((char*)ntHeader) + sizeof(IMAGE_NT_HEADERS)), sectionCount };
      }

      [[nodiscard]] DWORD                    translate_offset(DWORD addr) const noexcept {
        for (auto& section : sections) {
          DWORD sectionVAddr = section.VirtualAddress;
          if (addr >= sectionVAddr && addr < (sectionVAddr + section.SizeOfRawData))
            return addr - sectionVAddr + section.PointerToRawData;
        }

        return 0;
      }

    public:
      explicit module_walker(void* handle)
          : imageBase((char*)handle),
            header(get_nt_headers(handle)),
            sections(get_sections(header))
      { }

      [[nodiscard]] void* directory_entry(DWORD dirEntry) const noexcept {
        return at_offset(header->OptionalHeader.DataDirectory[dirEntry].VirtualAddress);
      }

      template <typename T = void>
      [[nodiscard]] T*    at_offset(DWORD virtualOffset) const noexcept {
        auto offset = translate_offset(virtualOffset);
        return offset ? reinterpret_cast<T*>(imageBase + offset) : nullptr;
      }
    };


    void load_exported_functions() noexcept {

      constexpr static std::string_view ApiPrefixString = "_agate_api_";

      struct function_info {
        function_ptr     address       = {};
        std::string_view dispatchFlags = {};
      };

      module_walker walker{m_handle};

      auto exportDirectory = static_cast<PIMAGE_EXPORT_DIRECTORY>(walker.directory_entry(IMAGE_DIRECTORY_ENTRY_EXPORT));

      auto exportedNameCount     = exportDirectory->NumberOfNames;
      // auto exportedFunctionCount = exportDirectory->NumberOfFunctions;

      auto pOrdinalTable = walker.at_offset<WORD>(exportDirectory->AddressOfNameOrdinals);
      auto pNameTable    = walker.at_offset<DWORD>(exportDirectory->AddressOfNames);
      auto pFuncTable    = walker.at_offset<DWORD>(exportDirectory->AddressOfFunctions);


      auto funcNames = std::span{ pNameTable, exportedNameCount };

      dictionary<std::vector<function_info>> exportDictionary = {};

      for (size_t i = 0; i < exportedNameCount; ++i) {
        auto pName = walker.at_offset<const char>(pNameTable[i]);
        std::string_view name{pName};
        if (name.starts_with(ApiPrefixString)) {
          name.remove_prefix(ApiPrefixString.size());
          size_t pos = name.find("__");
          if (pos != std::string_view::npos) {
            std::string_view firstNamePart = name.substr(0, pos);

            auto& fam = exportDictionary[firstNamePart];
            auto& func = fam.emplace_back();

            WORD ordinal = pOrdinalTable[i];

            func.address       = walker.at_offset<void()>(pFuncTable[ordinal]);
            func.dispatchFlags = name.substr(pos + 2);
          }
        }
      }


      for (auto&& entry : exportDictionary) {
        auto& entryVec = entry.get();
        auto [exportPos, isNew] = m_exports.try_emplace(entry.key());
        AGT_invariant( isNew );
        auto& fam = exportPos->get();
        size_t count = entryVec.size();
        auto ptrArray = new function_ptr[count];
        auto flagsArray = new dispatch_flags[count];
        for (size_t i = 0; i < count; ++i) {
          ptrArray[i]         = entryVec[i].address;
          flagsArray[i].flags = entryVec[i].dispatchFlags;
        }
        fam.entryAddresses = ptrArray;
        fam.entryFlags     = flagsArray;
        fam.entryCount     = count;
      }

    }

    static double flag_match(dispatch_flags dispatchFlags, dispatch_flags desiredFlags) noexcept;

  public:

    static std::unique_ptr<module_accessor> load(sys_cstring libName) noexcept {

      using pfn_get_module_version = agt_u32_t(*)();

      HMODULE handle;

      if (!(handle = LoadLibraryW(libName)))
        return nullptr;

      auto m = std::make_unique<module_accessor>();

      m->m_handle = handle;

      auto copyResult = ::wcscpy_s(m->m_name, libName);
      AGT_assert(copyResult == 0);

      auto pfnGetModuleVersion = (pfn_get_module_version) GetProcAddress(handle, AGT_GET_MODULE_VERSION_NAME);

      if (!pfnGetModuleVersion) {
        FreeLibrary(handle);
        return nullptr;
      }

      m->m_version = version::from_integer(pfnGetModuleVersion());

      auto libPathBufferLength = (DWORD)std::size(m->m_path);
      auto pathResult = GetModuleFileNameW(handle, m->m_path, libPathBufferLength);
      AGT_assert(pathResult <= libPathBufferLength);

      m->load_exported_functions();


      return std::move(m);
    }

    // If module exports at least one function in the specified family, returns the address and match score of the family entry with the greatest match score
    // If module does not export any functions from the specified family, returns null address, and score should be ignored.
    [[nodiscard]] std::pair<function_ptr, double> lookup(std::string_view name, dispatch_flags desiredFlags) const noexcept {
      auto result = m_exports.find(name);
      if (result == m_exports.end())
        return { nullptr, 0 };
      auto&& family = result->get();
      function_ptr bestFuncPtr = family.entryAddresses[0];
      double       bestScore   = flag_match(family.entryFlags[0], desiredFlags);
      for (size_t i = 1; i < family.entryCount; ++i) {
        double score = flag_match(family.entryFlags[i], desiredFlags);
        if (bestScore < score) {
          bestScore = score;
          bestFuncPtr = family.entryAddresses[i];
        }
      }
      return { bestFuncPtr, bestScore };
    }

    [[nodiscard]] version         version() const noexcept {
      return m_version;
    }

    [[nodiscard]] sys_string_view name() const noexcept {
      return { (sys_cstring)m_name };
    }

    [[nodiscard]] sys_string_view path() const noexcept {
      return { (sys_cstring)m_path };
    }

  private:
    void*                       m_handle              = nullptr;
    class version               m_version             = {};
    dictionary<function_family> m_exports             = {};
    sys_char                    m_name[MaxNameLength] = {};
    sys_char                    m_path[MaxPathLength] = {};
  };
}

#endif//AGATE_INTERNAL_MODULE_HPP
