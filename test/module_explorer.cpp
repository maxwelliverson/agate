//
// Created by maxwe on 2022-12-19.
//


#include <span>
#include <cassert>
#include <cstdio>
#include <regex>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma comment(lib, "mincore")

namespace {

  struct export_info {
    const char* name;
    char        section[8];
    WORD        ordinal;
    DWORD       rva;
    void(*      address)();
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
        assert(ntSignature == 0x50450000);
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


    void                get_section(DWORD rva, char(&buffer)[8]) const noexcept {
      std::memcpy(buffer, "???????", 8);

      for (auto& section : sections) {
        DWORD sectionVAddr = section.VirtualAddress;
        if (rva >= sectionVAddr && rva < (sectionVAddr + section.SizeOfRawData)) {
          std::memcpy(buffer, section.Name, sizeof(section.Name));
          return;
        }
      }
    }

    [[nodiscard]] void* directory_entry(DWORD dirEntry) const noexcept {
      return at_offset(header->OptionalHeader.DataDirectory[dirEntry].VirtualAddress);
    }

    template <typename T = void>
    [[nodiscard]] T*    at_offset(DWORD virtualOffset) const noexcept {
      auto offset = translate_offset(virtualOffset);
      return offset ? reinterpret_cast<T*>(imageBase + offset) : nullptr;
    }
  };

  void iterate_exports(HMODULE handle,
                       int(* pfnFilter)(const char* funcName, void* pUserData),
                       void (*pfnCallback)(const export_info* pExportInfo, void* pUserData),
                       void* pUserData) noexcept {

    assert(handle != nullptr);
    assert(pfnCallback != nullptr);

    module_walker walker{handle};

    auto exportDirectory = static_cast<PIMAGE_EXPORT_DIRECTORY>(walker.directory_entry(IMAGE_DIRECTORY_ENTRY_EXPORT));

    auto exportedNameCount     = exportDirectory->NumberOfNames;
    auto exportedFunctionCount = exportDirectory->NumberOfFunctions;

    auto pOrdinalTable = walker.at_offset<WORD>(exportDirectory->AddressOfNameOrdinals);
    auto pNameTable    = walker.at_offset<DWORD>(exportDirectory->AddressOfNames);
    auto pFuncTable    = walker.at_offset<DWORD>(exportDirectory->AddressOfFunctions);


    auto funcNames = std::span{ pNameTable, exportedNameCount };

    export_info exportInfo;

    for (size_t i = 0; i < exportedNameCount; ++i) {
      auto name = walker.at_offset<const char>(pNameTable[i]);

      if (!pfnFilter || pfnFilter(name, pUserData)) {
        WORD ordinal = pOrdinalTable[i];
        DWORD rva    = pFuncTable[ordinal];
        auto func = walker.at_offset<void()>(pFuncTable[ordinal]);

        exportInfo.name    = name;
        walker.get_section(rva, exportInfo.section);
        exportInfo.ordinal = ordinal;
        exportInfo.rva     = rva;
        exportInfo.address = func;

        pfnCallback(&exportInfo, pUserData);
      }
    }


  }


  struct user_data {
    std::regex glob;
    bool       printVerbose;
    FILE*      printDst;
  };

  int  filter_by_glob(const char* funcName, void* pUserData) {
    auto data = static_cast<user_data*>(pUserData);
    return std::regex_match(funcName, data->glob);
  }

  void print_callback(const export_info* exportInfo, void* pUserData) {
    auto data = static_cast<user_data*>(pUserData);
    if (data->printVerbose) {
      fprintf(data->printDst, "#%-4d %-8s 0x%-8lx %p   %s\n", exportInfo->ordinal, exportInfo->section, exportInfo->rva, exportInfo->address, exportInfo->name);
    }
    else {
      fprintf(data->printDst, "%s\n", exportInfo->name);
    }
  }

  HMODULE lookup_module(const char* moduleName) {
    return LoadLibraryA(moduleName);
  }

}

void print_help(char* path, int exitCode = 0) {
  fprintf(stderr,
          "USAGE: \"%s [OPTIONS] <library>\"", path);
  exit(exitCode);
}

int main(int argc, char** argv) {

  if (argc < 2)
    print_help(argv[0], -1);

  char* path = argv[0];

  int(* filterPtr)(const char*, void*);
  const char* moduleName = nullptr;
  filterPtr = nullptr;

  user_data data;
  data.printVerbose = false;
  data.printDst     = stdout;

  for (int i = 1; i < argc; ++i) {

    char* opt = argv[i];

    if (strcmp(opt, "-h") == 0 || strcmp(opt, "--help") == 0)
      print_help(path);

    if (i + 1 == argc) {
      moduleName = opt;
    }
    else if (strcmp(opt, "-f") == 0 || strcmp(opt, "--filter") == 0) {
      if (i + 1 == argc) {
        fprintf(stderr, "Option %s <regex> argument not specified\n", opt);
        print_help(path, -1);
      }

      filterPtr = filter_by_glob;
      data.glob.assign(argv[i+1]);
      ++i;
    }
    else if (strcmp(opt, "-v") == 0 || strcmp(opt, "--verbose") == 0){
      data.printVerbose = true;
    }
    else {
      fprintf(stderr, "Unknown option %s\n", opt);
      print_help(path, -1);
    }
  }

  auto moduleHandle = lookup_module(moduleName);
  if (!moduleHandle) {
    fprintf(stderr, "File \"%s\" not found", moduleName);
    return -1;
  }

  if (data.printVerbose) {
    fprintf(data.printDst, "Ord   Sect     RVA        Address            Name\n"
                           "---------------------------------------------------------------\n");
  }
  iterate_exports(moduleHandle, filterPtr, print_callback, &data);

  return 0;
}
