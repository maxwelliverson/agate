//
// Created by maxwe on 2022-08-18.
//

#include "alloc-test.h"

#include <iostream>
#include <vector>
#include <cassert>
#include <concepts>
#include <charconv>
#include <chrono>
#include <string>
#include <optional>
#include <span>
#include <string_view>
#include <sstream>
#include <random>
#include <ranges>
#include <map>


#define ALLOC_TEST_UNIFORM_DISTRIBUTION "default-uniform"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


using test_clock      = std::chrono::high_resolution_clock;
using test_duration   = typename test_clock::duration;
using test_time_point = typename test_clock::time_point;
using test_float_duration = std::chrono::duration<double, typename test_duration::period>;


struct library {
  std::string name;
  std::string path;
  std::string args;
  bool        isAlias;
  void*       handle;
};



enum op_kind {
  alloc_op = 0,
  free_op = 1
};


struct random_op {
  enum { alloc_op, free_op } kind;
  union {
    uint32_t count;
    uint32_t index;
  };
};

struct alloc_desc {
  uint32_t offset;
  uint32_t size;
};


class random_generator {
  std::mt19937                mersenne;
  void*                       state;
  AllocTestDistributionVTable vtable;
public:
  random_generator(const AllocTestDistributionVTable& dist, const AllocTestOptions& options) noexcept
      : mersenne(options.rngSeed),
        state(nullptr),
        vtable(dist)
  {
    if (vtable.newDistribution)
      state = vtable.newDistribution(&options);
  }

  random_generator(const random_generator&) = delete;

  ~random_generator() {
    if (state)
      vtable.deleteDistribution(state);
  }


  [[nodiscard]] AllocTestOp next() noexcept {
    uint32_t value = mersenne();
    AllocTestOp op{};
    vtable.distribute(state, value, &op);
    return op;
  }
};



struct nameclash_info {
  std::string_view type;
  std::string      name;
  std::string      libA;
  std::string      libB;
};


struct algorithm_desc {
  std::string name;
  std::string desc;
  std::string libPath;
  AllocTestAlgorithmVTable vtable;
};

struct distribution_desc {
  std::string name;
  std::string desc;
  std::string libPath;
  AllocTestDistributionVTable vtable;
};


struct AllocTestRegistry_st {
  std::string                              currentNamespace;
  std::string_view                         currentPath;
  std::map<std::string, algorithm_desc>    algorithms;
  std::map<std::string, distribution_desc> distributions;
  std::vector<nameclash_info>              nameclashList;
};


struct test_state;

class test_runner;

struct test_statistics {
  uint64_t bytesPerUnit;
  uint64_t totalUnitCount;
  uint64_t totalUnitsAllocated;
  uint64_t totalUnitsFreed;

  uint64_t maxAllocCapacity;
  uint64_t aveCapacity;

  uint32_t totalAllocCount;
  uint32_t totalFreeCount;
  uint32_t badAllocCount;
  uint32_t badFreeCount;
  uint32_t totalOpsAtCapacity;

  test_duration totalTestTime;
  test_float_duration aveTimePerAlloc;
  test_float_duration aveTimePerFree;
  test_float_duration aveTimePerOp;

public:
  void print(std::string_view algorithmName, const test_runner & options) noexcept;

};

struct algorithm_result {
  std::string              name;
  test_statistics          stats;
  std::vector<std::string> debugSteps;
};



class test_runner {

  class uniform_distribution {
    uint32_t minSize;
    uint32_t maxSize;
    double   minSizeDoub;
    double   maxSizeDoub;
  public:
    uniform_distribution(const AllocTestOptions& opt) noexcept
        : minSize(opt.minAllocSize),
          maxSize(opt.maxAllocSize),
          minSizeDoub(minSize),
          maxSizeDoub(maxSize)
    {}

    void distribute(uint32_t value, AllocTestOp& op) const noexcept {
      op.kind = static_cast<AllocTestOpKind>(value & 0x1);
      if (op.kind == ALLOC_TEST_OP_ALLOC) {
        double frac = (double)(value >> 1) / ((double)INT32_MAX);
        double lerped  = std::lerp(minSizeDoub, maxSizeDoub, frac);
        auto   size  = static_cast<uint32_t>(lerped);
        op.size = std::clamp(size, minSize, maxSize); // Account for any weird edge case rounding errors...
      }
      else
        op.index = value >> 1;
    }
  };

  inline constexpr static AllocTestDistributionVTable UniformDistributionVTable{
      [](void* state, uint32_t value, AllocTestOp* op){
        static_cast<uniform_distribution*>(state)->distribute(value, *op);
      },
      [](const AllocTestOptions* opt) -> void* { return new uniform_distribution(*opt); },
      [](void* state) { delete static_cast<uniform_distribution*>(state); }
  };

  std::vector<std::string>       libPathList;
  std::map<std::string, library> libraries;
  AllocTestRegistry_st           registry;
  std::vector<algorithm_result>  results;
  bool                           listAndExit;
  bool                           printVerbose;
  bool                           printDebug;
  AllocTestOptions               globalOptions;



  [[noreturn]] void invalidDistributionError(const std::string& str) const noexcept;

  [[noreturn]] void invalidAlgorithmError(const std::string& str) const noexcept;






  void checkIfValidAlgorithms() const noexcept;

  void parseTrailingArgs(const std::vector<std::string>& trailingArgs) noexcept;

  void parseLibraries(const std::string& librariesString) noexcept;

  void loadLibrary(library& lib) noexcept;

  void parse(int argc, char** argv) noexcept;

  void cleanup() noexcept;


  void printListAlgorithm(const algorithm_desc& alg, size_t maxAlgNameLength) noexcept {
    std::stringstream descStr;
    std::string_view  desc{};

    auto libSpacing = (int)(maxAlgNameLength + 3);

    if (!alg.desc.empty()) {
      if (alg.desc.find('\n') != std::string::npos) {
        for (char c : alg.desc) {
          if (c == '\n')
            descStr << "\n\t";
          else
            descStr << c;
        }
        desc = descStr.view();
      }
      else
        desc = alg.desc;
    }

    if (alg.libPath.empty())
      printf("%s\n", alg.name.c_str());
    else
      printf("%-*s (imported from %s)\n", libSpacing, alg.name.c_str(), alg.libPath.c_str());

    if (!alg.desc.empty())
      printf("\t%s", desc.data());
  }

  void executeAlgorithms(test_state& state, std::span<const char* const> algorithms) noexcept;


public:
  test_runner()
      : libraries(),
        registry(),
        listAndExit(false),
        printVerbose(false),
        printDebug(false),
        globalOptions()
  { }

  ~test_runner() {
    cleanup();
  }

  void init(int argc, char** argv) noexcept {
    this->parse(argc, argv);

    for (auto&& [libName, lib] : libraries)
      loadLibrary(lib);
  }


  [[nodiscard]] bool shouldRunTests() const noexcept {
    return !listAndExit;
  }

  [[nodiscard]] const AllocTestOptions& getGlobalOptions() const noexcept {
    return globalOptions;
  }

  [[nodiscard]] const distribution_desc& getDistribution() const noexcept {

    const static distribution_desc UniformDistributionDesc{
        .name = ALLOC_TEST_UNIFORM_DISTRIBUTION,
        .desc = "",
        .libPath = "",
        .vtable = UniformDistributionVTable
    };

    if (strcmp(globalOptions.distributionId, ALLOC_TEST_UNIFORM_DISTRIBUTION) == 0)
      return UniformDistributionDesc;
    std::string distName = globalOptions.distributionId;
    auto result = registry.distributions.find(distName);
    if (result == registry.distributions.end())
      invalidDistributionError(distName);
    auto&& [distId, dist] = *result;
    return dist;
  }

  [[nodiscard]] const algorithm_desc&    getAlgorithm(std::string_view alg) const noexcept {
    std::string algName{alg};
    auto result = registry.algorithms.find(algName);
    if (result == registry.algorithms.end())
      invalidAlgorithmError(algName);
    auto&& [algId, algorithm] = *result;
    return algorithm;
  }


  void execute() noexcept;

  void printResults() noexcept;

  void printList() noexcept {
    size_t maxAlgNameLength = 0;
    for (auto&& algName : registry.algorithms | std::views::keys) {
      if (algName.size() > maxAlgNameLength)
        maxAlgNameLength = algName.size();
    }



    for (auto&& alg : registry.algorithms | std::views::values) {
      if (alg.libPath.empty())
        printListAlgorithm(alg, maxAlgNameLength);
    }

    for (auto&& lib : libPathList) {
      for (auto&& alg : registry.algorithms | std::views::values) {
        if (alg.libPath == lib)
          printListAlgorithm(alg, maxAlgNameLength);
      }
    }
  }
};



struct op {
  uint32_t opKind      : 1;
  uint32_t allocOffset : 16;
  uint32_t count       : 15;
};



struct test_state {
  void*                    alloc;
  AllocTestAlgorithmVTable vtable;
  std::vector<op>          opList;


  [[nodiscard]] uint32_t rawAlloc(uint32_t size) const noexcept {
    return vtable.alloc(alloc, size);
  }

  void                   rawFree(uint32_t offset, uint32_t size) const noexcept {
    vtable.free(alloc, offset, size);
  }


  void                   resetAllocator(const AllocTestOptions& opt) noexcept {
    if (alloc) {
      vtable.deleteAllocator(alloc);
      alloc = vtable.newAllocator(&opt);
    }
  }


  void cleanupTest() noexcept;

  void gatherTestStats(test_statistics& stats) noexcept;

  void executeDebugTest(test_statistics& stats, std::vector<std::string>& debugSteps) noexcept;

  void executeTest(test_statistics& stats) noexcept;

  void loadTest(AllocTestRegistry registry, std::string_view algorithm) noexcept;

  void executeTests(test_statistics& stats, std::span<const char* const> algorithms) noexcept;

  void generateOpList(const test_runner & opt) noexcept;

  void init(const test_runner & opt, std::string_view algorithm) noexcept;

  void clear() noexcept;
};










///

/*
void loadLibrary(test_options& options, library& lib) noexcept {
  for (char& c : lib.path)
    if (c == '/')
      c = '\\';

  HMODULE handle = LoadLibrary(lib.path.c_str());
  if (!handle) {
    fprintf(stderr, "Unable to load library \"%s\"", lib.path.c_str());
    abort();
  }

  lib.handle = handle;

  auto registerLibrary = (decltype(&allocTestRegisterLibrary))(GetProcAddress(handle, "allocTestRegisterLibrary"));

  if (!registerLibrary) {
    fprintf(stderr, "\"%s\" does not export allocTestRegisterLibrary", lib.path.c_str());
    abort();
  }

  if (lib.isAlias)
    options.registry.currentNamespace = lib.name + "::";
  else
    options.registry.currentNamespace = {};

  options.registry.currentPath = lib.path;

  AllocTestLibrary allocTestLibrary;
  allocTestLibrary.registry = &options.registry;
  allocTestLibrary.registerAlgorithm = [](AllocTestRegistry registry, const char* algorithmID, const AllocTestAlgorithmVTable* vtable) {
    std::string algName = registry->currentNamespace + algorithmID;
    auto [iter, isNew] = registry->algorithms.try_emplace(std::move(algName), registry->currentPath, *vtable);
    if (!isNew) {
      auto& nameclash = registry->nameclashList.emplace_back();
      nameclash.type = "Algorithm";
      nameclash.name = algName;
      nameclash.libA = iter->second.first;
      nameclash.libB = registry->currentPath;
    }
  };
  allocTestLibrary.registerDistribution = [](AllocTestRegistry registry, const char* distributionID, const AllocTestDistributionVTable* vtable) {
    std::string distName = registry->currentNamespace + distributionID;
    auto [iter, isNew] = registry->distributions.try_emplace(std::move(distName), registry->currentPath, *vtable);
    if (!isNew) {
      auto& nameclash = registry->nameclashList.emplace_back();
      nameclash.type = "Distribution";
      nameclash.name = distName;
      nameclash.libA = iter->second.first;
      nameclash.libB = registry->currentPath;
    }
  };

  char*  splitArgs;
  std::vector<char*> rawSplitArgs;
  int    libArgc;
  char** libArgv;


  if (lib.args.empty()) {
    splitArgs = nullptr;
    libArgc = 0;
    libArgv = nullptr;
  }
  else {
    size_t stringSize = lib.args.size() + 1;
    splitArgs = new char[stringSize];
    strcpy_s(splitArgs, stringSize, lib.args.c_str());
    char* token;
    const char* delim = " ";
    char *nextToken;
    token = strtok_s(splitArgs, " ", &nextToken);
    while(token) {
      rawSplitArgs.push_back(token);
      token = strtok_s(nullptr, delim, &nextToken);
    }

    libArgc = (int)rawSplitArgs.size();
    libArgv = rawSplitArgs.data();
  }


  registerLibrary(&allocTestLibrary, &options.globalOptions, libArgc, libArgv);

  delete[] splitArgs;
}
*/


/*void printAvailableAlgorithms() noexcept {
  std::stringstream sstr;
  for (auto&& alg : g_availableAlgorithms)
    sstr << "\t" << alg.first << "\n";
  auto string = sstr.str();
  printf("Available Algorithms: \n%s\n", string.c_str());
}*/


void dumpParsedArgs(std::span<const std::pair<std::string, std::string>> parsedArgs) noexcept {
  printf("Parsed Arguments: \n");
  for (const auto& [flag, value] : parsedArgs)
    printf("\nflag=\"%s\"\nvalue=\"%s\"\n", flag.c_str(), value.c_str());
}

void printHelpString() noexcept {

  const std::string helpString = R"help(

  test-alloc - Test, benchmark, and compare custom allocation algorithms

  USAGE:
    test-alloc.exe [OPTIONS] [-- <libname>="<libargs>", ...]
    test-alloc.exe --list [-l=<filenames> | --libraries=<filenames>]
    test-alloc.exe -h | --help

  OPTIONS:
    -b=<integer>,              Specify the total size of the bitmap used by the
    --bitmap-size=<integer>      allocation algorithm. This value is only meaningful
                                 to allocators that make use of bitmaps. Individual
                                 allocators set their own default values.

    -c=<integer>,              Specify the total number of alloc/free operations
    --op-count=<integer>         during the test. Default value: 2048

    -d, --debug                Enable algorithm debugging by having the allocator
                                 print its state after every operation.

    -h, --help                 Print this help and exit.

    -l="[<name>=]<path>, ...", Load the libraries listed within the curly brackets.
    --libraries=                 The curly braces contain a list of comma separated
                                 paths to shared libraries that export
                                 allocTestRegisterLibrary. The algorithms and rng
                                 distributions exported by this library are then made
                                 available for use. Optionally, an import name can be
                                 specfied before the library path, in which case,
                                 every symbol exported by said library will have a
                                 namespace prepended onto it in the form of
                                 "name::symbol".

    --list                     Print the complete ID list of available algorithms and
                                 exit. This list will include any algorithms exported
                                 by libraries specified with -l or --libraries.

    --max-alloc-size=<int>     Set the maximum possible number of units that can be
                                 allocated in a single operation. Default value: 64

    --max-alloc-count=<int>    Set the maximum possible number of total allocations
                                 at any given point. Default value: 1000

    --min-alloc-size=<int>     Set the minimum possible number of units that can be
                                 allocated in a single operation. Default value: 1

    -r=<dist>,                 Specify the distribution used while generating
    --rng-dist=<dist>            allocation sizes.


    -s=<integer>,              Select a specific seed for the random number generator
    --seed=<integer>             used to generate the sequence of test operations.
                                 By specifying this manually, users can ensure the
                                 same sequence of operations is used, allowing direct
                                 comparison of benchmarks across different runs.

    -t=<algorithms>,           Test only the algorithms specified by <algorithms>.
    --test=<algorithms>          <algorithms> must be a comma-delimited list of
                                 valid algorithm IDs. These may be algorithms provided
                                 by libraries specified with -l or --libraries. For a
                                 complete list of valid IDs, see --list.

    -u=<integer>,              Specify the number of bytes per allocation unit.
    --unit-bytes=<integer>       Default value: 4

    -v, --verbose              Print additional details throughout execution.

  )help";

  fputs(helpString.c_str(), stderr);
}

[[noreturn]] void parseError(std::string_view arg) noexcept {
  auto buf = new char[arg.size() + 1];
  std::memcpy(buf, arg.data(), arg.size());
  buf[arg.size()] = '\0';
  fprintf(stderr, "Invalid command line arguments: could not parse \"%s\".\n", buf);
  delete[] buf;
  printHelpString();
  abort();
}

void collectArguments(std::vector<std::pair<std::string, std::string>>& args, std::vector<std::string>& tailingArgs, int argc, char** argv) noexcept {
  if (argc < 2)
    return;

  std::string_view  flag;
  // std::string_view  value;
  std::stringstream superValue;
  bool              collectValue = false;
  bool              collectTailing = false;


  for (int i = 1; i < argc; ++i) {
    
    if (collectTailing) {
      tailingArgs.emplace_back(argv[i]);
      // args.emplace_back(flag, value);
      continue;
    }

    if (collectValue) {
      if (*argv[i] == '-') {
        args.emplace_back(flag, "");
        flag = {};
        --i;
        collectValue = false;
        continue;
      }
      // value = ;
      args.emplace_back(flag, argv[i]);
      flag = {};
      // value = {};
      collectValue = false;
    }
    else if (strcmp(argv[i], "--") == 0) {
      collectTailing = true;
      continue;
    } 
    else {
      auto equalsSign = strchr(argv[i], '=');
      if (!equalsSign) {
        flag = argv[i];
        collectValue = true;
      }
      else {
        flag = { argv[i], static_cast<size_t>(equalsSign - argv[i]) };
        ++equalsSign;
        /*if (*equalsSign == '{') {
          if (auto bracketPos = strchr(equalsSign, '}')) {
            if (*(bracketPos + 1) != '\0')
              parseError(argv[i]);
            auto view = std::string_view{equalsSign};
            view.remove_prefix(1);
            view.remove_suffix(1);
            args.emplace_back(flag, view);
            flag = {};
          }
          else {
            superValue << (equalsSign + 1);
            findClosingBracket = true;
          }
          continue;
        }
        else {

        }*/
        // value = ;
        args.emplace_back(flag, equalsSign);
        flag = {};
        // value = {};
      }
    }
  }

  if (collectValue)
    args.emplace_back(flag, "");
}

void splitString(std::string_view src, const char* delim, std::vector<std::string>& splitStrings) noexcept {
  char* splitBuf = new char[src.size() + 1];
  memcpy(splitBuf, src.data(), src.size());
  splitBuf[src.size()] = '\0';
  char* nextToken;
  char* token = strtok_s(splitBuf, delim, &nextToken);
  while (token) {
    splitStrings.emplace_back(token);
    token = strtok_s(nullptr, delim, &nextToken);
  }
  delete[] splitBuf;
}

std::string_view trim(std::string_view sv, std::string_view charSet = " \t\n\r\f\v") noexcept {
  auto offset = sv.find_first_not_of(charSet);
  if (offset == std::string_view::npos)
    return {};
  sv.remove_prefix(offset);
  offset = sv.find_last_not_of(charSet);
  // offset is guaranteed to not be npos, as this could only be the case if sv only contained characters from charSet,
  // but that was handled by the first check.
  // If there aren't any trailing characters from charSet, offset + 1 == offset.size(), so all is well.
  return sv.substr(0, offset + 1);
}

std::string_view extractLibNameFromPath(std::string_view path) noexcept {
  auto slashPos = path.find_last_of("/\\");
  if (slashPos != std::string_view::npos)
    path.remove_prefix(slashPos + 1);
  auto periodPos = path.find_last_of('.');
  if (periodPos != std::string_view::npos)
    path.remove_suffix(path.size() - periodPos);
  return path;
}

std::pair<std::string_view, std::string_view> splitStringView(std::string_view str, std::string_view delim, bool includeDelimInRemainder = false) noexcept {
  auto result = str.find_first_of(delim);
  if (result == std::string_view::npos)
    return { str, {} };
  if (includeDelimInRemainder)
    return { str.substr(0, result), str.substr(result) };
  auto token = str.substr(0, result);
  auto remainder = str.substr(result);
  result = remainder.find_first_not_of(delim);
  if (result == std::string_view::npos)
    return { token, {} };
  return { token, remainder.substr(result) };
}


/*void parseTrailingArgs(test_options& opt, const std::vector<std::string>& trailingArgs) noexcept {
  std::string_view libName;
  std::string_view libArgs;

  for (auto&& arg : trailingArgs) {
    std::tie(libName, libArgs) = splitStringView(arg, "=");
    if (!libArgs.empty())
      parseError(libName);
    auto libEntry = opt.libraries.find(std::string(libName));
    if (libEntry == opt.libraries.end()) {
      fprintf(stderr, "\"%.*s\" is not a valid library name\n", (int)libName.size(), libName.data());
      abort();
    }
    libEntry->second.args = libArgs;
  }
}

void parseLibraries(test_options& opt, const std::string& librariesString) noexcept {
  std::vector<std::string> libraries;
  splitString(librariesString, ",", libraries);
  std::string_view libName;
  std::string_view libPath;
  bool isAlias = true;

  for (auto&& lib : libraries) {
    std::tie(libName, libPath) = splitStringView(trim(lib), "=");
    if (libPath.empty()) {
      swap(libName, libPath);
      libName = extractLibNameFromPath(libPath);
      isAlias = false;
    }
    auto& libInfo = opt.libraries[std::string(libName)];
    if (!libInfo.name.empty()) {
      fprintf(stderr, "Error: Attempted to load multiple libraries under the name \"%s\"\n", libInfo.name.c_str());
      abort();
    }

    libInfo.name = libName;
    libInfo.path = libPath;
    libInfo.isAlias = isAlias;
    libInfo.handle = nullptr;
    libInfo.args = "";
  }
}*/

template <typename T>
void parseInteger(std::string_view flag, std::string_view value, T& out) noexcept {
  const char* first = value.data();
  const char* last  = first + value.size();
  auto [end, err] = std::from_chars(first, last, out);
  if (end != last) {
    fprintf(stderr, "Invalid value %.*s for flag \"%.*s\". Value must be a valid integer.\n", (int)value.size(), value.data(), (int)flag.size(), flag.data());
    exit(-1);
  }
  if (err != std::errc{}) {
    char errMsgBuf[256];
    strerror_s(errMsgBuf, (int)err);
    fprintf(stderr, "Invalid value %.*s for flag \"%.*s\". %s.\n", (int)value.size(), value.data(), (int)flag.size(), flag.data(), errMsgBuf);
    exit(-1);
  }
}

uint32_t parseIntegerU32(std::string_view flag, std::string_view value) noexcept {
  uint32_t out;
  parseInteger(flag, value, out);
  return out;
}

uint64_t parseIntegerU64(std::string_view flag, std::string_view value) noexcept {
  uint64_t out;
  parseInteger(flag, value, out);
  return out;
}

/*void parseOptions(test_options& opt, int argc, char** argv) noexcept {
  std::vector<std::pair<std::string, std::string>> cmdLineOpts;
  std::vector<std::string> trailingArgs;
  collectArguments(cmdLineOpts, trailingArgs, argc, argv);


  std::vector<std::string> testList;

  std::optional<uint64_t> rngSeed;
  std::optional<uint64_t> bitmapSize;
  std::optional<uint64_t> opCount;
  std::optional<uint32_t> minAllocSize;
  std::optional<uint32_t> maxAllocSize;
  std::optional<uint32_t> maxAllocCount;
  std::optional<uint32_t> bytesPerUnit;
  std::string             rngDistribution;

  for (auto&& [flag, value] : cmdLineOpts) {
    if (flag == "-h" || flag == "--help") {
      printHelpString();
      exit(0);
    }
    if (flag == "-b" || flag == "--bitmap-size") {
      bitmapSize = parseIntegerU64(flag, value);
    }
    else if (flag == "-c" || flag == "--op-count") {
      opCount = parseIntegerU64(flag, value);
    }
    else if (flag == "-d" || flag == "--debug") {
      opt.printDebug = true;
    }
    else if (flag == "-l" || flag == "--libraries") {
      parseLibraries(opt, value);
    }
    else if (flag == "--list") {
      opt.listAndExit = true;
    }
    else if (flag == "--max-alloc-size") {
      maxAllocSize = parseIntegerU32(flag, value);
    }
    else if (flag == "--min-alloc-size") {
      minAllocSize = parseIntegerU32(flag, value);
    }
    else if (flag == "--max-alloc-count") {
      maxAllocCount = parseIntegerU32(flag, value);
    }
    else if (flag == "-r" || flag == "--rng-dist") {
      if (value.empty()) {
        fprintf(stderr, "Flag \"%s\" requires a value.", flag.c_str());
        exit(-1);
      }
      rngDistribution = value;
    }
    else if (flag == "-s" || flag == "--seed") {
      rngSeed = parseIntegerU64(flag, value);
    }
    else if (flag == "-t" || flag == "--test") {
      splitString(value, ",", testList);
    }
    else if (flag == "-u" || flag == "--unit-bytes") {
      bytesPerUnit = parseIntegerU32(flag, value);
    }
    else if (flag == "-v") {
      opt.printVerbose = true;
    }
  }

  if (!trailingArgs.empty())
    parseTrailingArgs(opt, trailingArgs);

  opt.globalOptions.structSize = static_cast<uint32_t>(sizeof(AllocTestOptions));

  opt.globalOptions.rngSeed = rngSeed.value_or(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  opt.globalOptions.allocatorBitMapSize = bitmapSize.value_or(0);
  opt.globalOptions.opCount = opCount.value_or(2048);
  opt.globalOptions.minAllocSize = minAllocSize.value_or(1);
  opt.globalOptions.maxAllocSize = maxAllocSize.value_or(64);
  opt.globalOptions.maxAllocCount = maxAllocCount.value_or(1000);
  opt.globalOptions.bytesPerUnit = bytesPerUnit.value_or(4);

  if (rngDistribution.empty())
    opt.globalOptions.distributionId = "linear";
  else {
    opt.globalOptions.distributionId = strdup(rngDistribution.c_str());
  }



  if (testList.empty()) {
    opt.globalOptions.algorithmIds = nullptr;
    opt.globalOptions.algorithmCount = 0;
  }
  else {
    char** ids = new char*[testList.size()];
    for (int i = 0; i < testList.size(); ++i)
      ids[i] = strdup(testList[i].c_str());
    opt.globalOptions.algorithmIds = ids;
    opt.globalOptions.algorithmCount = testList.size();
  }

  // dumpParsedArgs(cmdLineOpts);


}*/


void test_runner::checkIfValidAlgorithms() const noexcept {
  for (size_t i = 0; i < globalOptions.algorithmCount; ++i) {
    std::string algName = globalOptions.algorithmIds[i];
    if (!registry.algorithms.contains(algName))
      invalidAlgorithmError(algName);
  }
}



void test_runner::loadLibrary(library &lib) noexcept {
  for (char& c : lib.path)
    if (c == '/')
      c = '\\';

  HMODULE handle = LoadLibrary(lib.path.c_str());
  if (!handle) {
    fprintf(stderr, "Unable to load library \"%s\"", lib.path.c_str());
    abort();
  }

  lib.handle = handle;

  auto registerLibrary = (decltype(&allocTestRegisterLibrary))(GetProcAddress(handle, "allocTestRegisterLibrary"));

  if (!registerLibrary) {
    fprintf(stderr, "\"%s\" does not export allocTestRegisterLibrary", lib.path.c_str());
    abort();
  }

  if (lib.isAlias)
    registry.currentNamespace = lib.name + "::";
  else
    registry.currentNamespace = {};

  registry.currentPath = lib.path;
  libPathList.push_back(lib.path);

  AllocTestLibrary allocTestLibrary;
  allocTestLibrary.registry = &registry;
  allocTestLibrary.registerAlgorithm = [](AllocTestRegistry reg, const char* algorithmID, const char* description, const AllocTestAlgorithmVTable* vtable) {
    std::string algName = reg->currentNamespace + algorithmID;
    algorithm_desc desc{
        .name = algName,
        .desc = description,
        .libPath = std::string(reg->currentPath),
        .vtable = *vtable
    };
    auto [iter, isNew] = reg->algorithms.try_emplace(std::move(algName), std::move(desc));
    if (!isNew) {
      auto& nameclash = reg->nameclashList.emplace_back();
      nameclash.type = "Algorithm";
      nameclash.name = algName;
      nameclash.libA = iter->second.libPath;
      nameclash.libB = reg->currentPath;
    }
  };
  allocTestLibrary.registerDistribution = [](AllocTestRegistry reg, const char* distributionID, const char* description, const AllocTestDistributionVTable* vtable) {
    std::string distName = reg->currentNamespace + distributionID;
    distribution_desc desc{
        .name = distName,
        .desc = description,
        .libPath = std::string(reg->currentPath),
        .vtable = *vtable
    };
    auto [iter, isNew] = reg->distributions.try_emplace(std::move(distName), std::move(desc));
    if (!isNew) {
      auto& nameclash = reg->nameclashList.emplace_back();
      nameclash.type = "Distribution";
      nameclash.name = distName;
      nameclash.libA = iter->second.libPath;
      nameclash.libB = reg->currentPath;
    }
  };

  char*  splitArgs;
  std::vector<char*> rawSplitArgs;
  int    libArgc;
  char** libArgv;


  if (lib.args.empty()) {
    splitArgs = nullptr;
    libArgc = 0;
    libArgv = nullptr;
  }
  else {
    size_t stringSize = lib.args.size() + 1;
    splitArgs = new char[stringSize];
    strcpy_s(splitArgs, stringSize, lib.args.c_str());
    char* token;
    const char* delim = " ";
    char *nextToken;
    token = strtok_s(splitArgs, " ", &nextToken);
    while(token) {
      rawSplitArgs.push_back(token);
      token = strtok_s(nullptr, delim, &nextToken);
    }

    libArgc = (int)rawSplitArgs.size();
    libArgv = rawSplitArgs.data();
  }


  registerLibrary(&allocTestLibrary, &globalOptions, libArgc, libArgv);

  delete[] splitArgs;
}

void test_runner::parseTrailingArgs(const std::vector<std::string> &trailingArgs) noexcept {
  std::string_view libName;
  std::string_view libArgs;

  for (auto&& arg : trailingArgs) {
    std::tie(libName, libArgs) = splitStringView(arg, "=");
    if (!libArgs.empty())
      parseError(libName);
    auto libEntry = libraries.find(std::string(libName));
    if (libEntry == libraries.end()) {
      fprintf(stderr, "\"%.*s\" is not a valid library name\n", (int)libName.size(), libName.data());
      abort();
    }
    libEntry->second.args = libArgs;
  }
}

void test_runner::parseLibraries(const std::string &librariesString) noexcept {
  std::vector<std::string> libs;
  splitString(librariesString, ",", libs);
  std::string_view libName;
  std::string_view libPath;
  bool isAlias = true;

  for (auto&& lib : libs) {
    std::tie(libName, libPath) = splitStringView(trim(lib), "=");
    if (libPath.empty()) {
      swap(libName, libPath);
      libName = extractLibNameFromPath(libPath);
      isAlias = false;
    }
    auto& libInfo = libraries[std::string(libName)];
    if (!libInfo.name.empty()) {
      fprintf(stderr, "Error: Attempted to load multiple libraries under the name \"%s\"\n", libInfo.name.c_str());
      abort();
    }

    libInfo.name = libName;
    libInfo.path = libPath;
    libInfo.isAlias = isAlias;
    libInfo.handle = nullptr;
    libInfo.args = "";
  }
}

void test_runner::parse(int argc, char **argv) noexcept {
  std::vector<std::pair<std::string, std::string>> cmdLineOpts;
  std::vector<std::string> trailingArgs;
  collectArguments(cmdLineOpts, trailingArgs, argc, argv);


  std::vector<std::string> testList;

  std::optional<uint64_t> rngSeed;
  std::optional<uint64_t> bitmapSize;
  std::optional<uint64_t> opCount;
  std::optional<uint32_t> minAllocSize;
  std::optional<uint32_t> maxAllocSize;
  std::optional<uint32_t> maxAllocCount;
  std::optional<uint32_t> bytesPerUnit;
  std::string             rngDistribution;

  for (auto&& [flag, value] : cmdLineOpts) {
    if (flag == "-h" || flag == "--help") {
      printHelpString();
      exit(0);
    }
    if (flag == "-b" || flag == "--bitmap-size") {
      bitmapSize = parseIntegerU64(flag, value);
    }
    else if (flag == "-c" || flag == "--op-count") {
      opCount = parseIntegerU64(flag, value);
    }
    else if (flag == "-d" || flag == "--debug") {
      printDebug = true;
    }
    else if (flag == "-l" || flag == "--libraries") {
      parseLibraries(value);
    }
    else if (flag == "--list") {
      listAndExit = true;
    }
    else if (flag == "--max-alloc-size") {
      maxAllocSize = parseIntegerU32(flag, value);
    }
    else if (flag == "--min-alloc-size") {
      minAllocSize = parseIntegerU32(flag, value);
    }
    else if (flag == "--max-alloc-count") {
      maxAllocCount = parseIntegerU32(flag, value);
    }
    else if (flag == "-r" || flag == "--rng-dist") {
      if (value.empty()) {
        fprintf(stderr, "Flag \"%s\" requires a value.", flag.c_str());
        exit(-1);
      }
      rngDistribution = value;
    }
    else if (flag == "-s" || flag == "--seed") {
      rngSeed = parseIntegerU64(flag, value);
    }
    else if (flag == "-t" || flag == "--test") {
      splitString(value, ",", testList);
    }
    else if (flag == "-u" || flag == "--unit-bytes") {
      bytesPerUnit = parseIntegerU32(flag, value);
    }
    else if (flag == "-v") {
      printVerbose = true;
    }
  }

  if (!trailingArgs.empty())
    parseTrailingArgs(trailingArgs);

  globalOptions.structSize = static_cast<uint32_t>(sizeof(AllocTestOptions));

  globalOptions.rngSeed = rngSeed.value_or(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  globalOptions.allocatorBitMapSize = bitmapSize.value_or(0);
  globalOptions.opCount = opCount.value_or(2048);
  globalOptions.minAllocSize = minAllocSize.value_or(1);
  globalOptions.maxAllocSize = maxAllocSize.value_or(64);
  globalOptions.maxAllocCount = maxAllocCount.value_or(1000);
  globalOptions.bytesPerUnit = bytesPerUnit.value_or(4);

  if (rngDistribution.empty() || rngDistribution == ALLOC_TEST_UNIFORM_DISTRIBUTION)
    globalOptions.distributionId = ALLOC_TEST_UNIFORM_DISTRIBUTION;
  else {
    globalOptions.distributionId = strdup(rngDistribution.c_str());
  }



  if (testList.empty()) {
    globalOptions.algorithmIds = nullptr;
    globalOptions.algorithmCount = 0;
  }
  else {
    char** ids = new char*[testList.size()];
    for (int i = 0; i < testList.size(); ++i)
      ids[i] = strdup(testList[i].c_str());
    globalOptions.algorithmIds = ids;
    globalOptions.algorithmCount = testList.size();
  }

  // dumpParsedArgs(cmdLineOpts);
}

void test_runner::cleanup() noexcept {
  if (strcmp(globalOptions.distributionId, ALLOC_TEST_UNIFORM_DISTRIBUTION) != 0)
    free((void*)globalOptions.distributionId);
  globalOptions.distributionId = nullptr;

  assert((globalOptions.algorithmCount > 1) ^ (globalOptions.algorithmIds == nullptr));

  if (globalOptions.algorithmCount > 1) {
    for (size_t i = 0; i < globalOptions.algorithmCount; ++i)
      free((void*)globalOptions.algorithmIds[i]);
    delete[] globalOptions.algorithmIds;
    globalOptions.algorithmIds = nullptr;
  }
}


void test_runner::executeAlgorithms(test_state &state, std::span<const char* const> algorithms) noexcept {

  this->results.resize(algorithms.size());

  for (size_t i = 0; i < algorithms.size(); ++i) {
    std::string algName = algorithms[i];
    auto& result        = results[i];

    result.name = algName;

    // auto alg = registry.algorithms.find(algName);
    // assert(alg != registry.algorithms.end());

    state.init(*this, algName);

    if (printDebug)
      state.executeDebugTest(result.stats, result.debugSteps);
    else
      state.executeTest(result.stats);

    state.gatherTestStats(result.stats);

    state.clear();
  }
}

void test_runner::execute() noexcept {

  test_state state;
  std::vector<const char*> stringVec;
  std::span<const char* const> algSpan;

  if (globalOptions.algorithmCount == 0) {
    stringVec.reserve(registry.algorithms.size());
    for (auto&& [algName, alg] : registry.algorithms)
      stringVec.push_back(algName.c_str());
    algSpan = stringVec;
  }
  else {

    checkIfValidAlgorithms();

    algSpan = { globalOptions.algorithmIds, globalOptions.algorithmCount };
  }

  executeAlgorithms(state, algSpan);
}


void test_runner::invalidAlgorithmError(const std::string &str) const noexcept {
  fprintf(stderr, "Unknown algorithm ID: %s\n", str.c_str());
  exit(-1);
}

void test_runner::invalidDistributionError(const std::string &str) const noexcept {
  fprintf(stderr, "Unknown distribution ID: %s\n", str.c_str());
  exit(-1);
}


void test_runner::printResults() noexcept { }



void test_state::generateOpList(const test_runner &opt) noexcept {
  random_generator rng{opt.getDistribution().vtable, opt.getGlobalOptions()};

  auto&& options = opt.getGlobalOptions();

  std::vector<alloc_desc> allocs;

  for (uint32_t i = 0; i + allocs.size() < options.opCount - 1; ++i) {
    AllocTestOp op;

    if (allocs.size() == options.maxAllocCount) {
      do {
        op = rng.next();
      } while(op.kind == ALLOC_TEST_OP_ALLOC);
      assert(op.kind == ALLOC_TEST_OP_FREE);
    }
    else if (allocs.empty()) {
      do {
        op = rng.next();
      } while(op.kind == ALLOC_TEST_OP_FREE);
      assert(op.kind == ALLOC_TEST_OP_ALLOC);
    }
    else
      op = rng.next();

    if (op.kind == ALLOC_TEST_OP_ALLOC) {
      uint32_t size = std::clamp(op.size, options.minAllocSize, options.maxAllocSize);
      uint32_t offset = rawAlloc(size);
      struct op result;
      alloc_desc allocDesc;
      allocDesc.size = size;
      allocDesc.offset = offset;
      result.opKind = ALLOC_TEST_OP_ALLOC;
      result.count  = size;
      result.allocOffset = offset;

      opList.push_back(result);
      allocs.push_back(allocDesc);
    }
    else {
      assert(op.kind == ALLOC_TEST_OP_FREE);
      auto index = op.index % allocs.size();
      auto descPos = allocs.begin() + index;
      auto desc = *descPos;
      rawFree(desc.offset, desc.size);
      struct op result;
      result.opKind = ALLOC_TEST_OP_FREE;
      result.count  = desc.size;
      result.allocOffset = desc.offset;
      opList.push_back(result);
      allocs.erase(descPos);
    }
  }

  for (auto [offset, size] : allocs) {
    op freeOp{
        .opKind = free_op,
        .allocOffset = offset,
        .count = size
    };
    opList.push_back(freeOp);
    rawFree(offset, size);
  }

  assert( opList.size() == options.opCount || opList.size() == (options.opCount - 1) );

  resetAllocator(options);
}

void test_state::init(const test_runner &opt, std::string_view algorithm) noexcept {
  auto alg = opt.getAlgorithm(algorithm);
  vtable = alg.vtable;
  if (vtable.newAllocator)
    alloc = vtable.newAllocator(&opt.getGlobalOptions());
  generateOpList(opt);
}

void test_state::clear() noexcept {
  if (alloc)
    vtable.deleteAllocator(alloc);
  alloc = nullptr;
  vtable = {};
  opList.clear();
}

void test_state::executeTest(test_statistics &stats) noexcept {}

void test_state::executeDebugTest(test_statistics &stats, std::vector<std::string> &debugSteps) noexcept {}

void test_state::gatherTestStats(test_statistics &stats) noexcept {}







class naive_algorithm {
  uint64_t* bitmap;
  size_t    bitmapSize;
public:

  explicit naive_algorithm(const test_runner & opt) noexcept : bitmap(nullptr), bitmapSize(0) {

  }

  ~naive_algorithm() {
    delete[] bitmap;
  }

  static const char* name() noexcept {
    return "naive-bitmap";
  }

  static const char* description() noexcept {
    return "Naive implementation of a fixed size bitmap allocator";
  }

  uint32_t alloc(uint32_t count) noexcept {

  }

  void     free(uint32_t index, uint32_t count) noexcept {

  }

  bool     print(char* buffer, size_t bufSize, size_t* bytesWritten) noexcept {

  }
};

class naive_algorithm2 {
  uint64_t* bitmap;
  size_t    bitmapSize;
  uint32_t  firstFreeIndex;
  uint32_t  lastFreeIndex;
public:

  explicit naive_algorithm2(const test_runner & opt) noexcept {

  }

  ~naive_algorithm2() {
    delete[] bitmap;
  }


  static std::string_view name() noexcept {
    return "naive-with-hints";
  }

  uint32_t alloc(uint32_t count) noexcept {

  }

  void     free(uint32_t index, uint32_t count) noexcept {

  }

  bool     print(char* buffer, size_t bufSize, size_t* bytesWritten) noexcept {

  }
};

class chunked_algorithm {
  uint64_t* bitmap;
  size_t    bitmapSize;
  uint32_t  firstFreeIndex;

public:
  explicit chunked_algorithm(const test_runner & opt) noexcept : bitmap(nullptr), bitmapSize(0) {

  }

  ~chunked_algorithm() {
    delete[] bitmap;
  }

  static std::string_view name() noexcept {
    return "chunked";
  }

  uint32_t alloc(uint32_t count) noexcept {

  }

  void     free(uint32_t index, uint32_t count) noexcept {

  }

  bool     print(char* buffer, size_t bufSize, size_t* bytesWritten) noexcept {

  }
};



int main(int argc, char** argv) {
  test_runner runner{ };


  runner.init(argc, argv);

  if (runner.shouldRunTests()) {
    runner.execute();
    runner.printResults();
    /*state.init(options);
    state.execute();*/
  }
  else {
    runner.printList();
  }
}

