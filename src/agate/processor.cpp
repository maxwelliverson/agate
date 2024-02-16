//
// Created by maxwe on 2023-07-31.
//

#define AGT_BUILD_STATIC

#include "processor.hpp"

#include <array>
#include <bit>

#include <intrin.h>


using agt::cpu;


class cpu_register {
  union {
    char     characters[4];
    uint8_t  bytes[4];
    uint16_t words[2];
    uint32_t u32;
  };
public:

  [[nodiscard]] uint32_t value() const noexcept {
    return u32;
  }

  [[nodiscard]] uint32_t range(uint32_t fromBit, uint32_t toBit) const noexcept {
    uint32_t bitCount = (toBit - fromBit) + 1;
    uint32_t mask     = (0x1 << bitCount) - 1;
    return (u32 >> fromBit) & mask;
  }

  [[nodiscard]] const char* chars() const noexcept {
    return characters;
  }

  [[nodiscard]] uint16_t word(uint32_t index) const noexcept {
    return words[index];
  }

  [[nodiscard]] uint8_t  byte(uint32_t index) const noexcept {
    return bytes[index];
  }

  [[nodiscard]] uint8_t  nibble(uint32_t index) const noexcept {
    return static_cast<uint8_t>((u32 >> (index * 4)) & 0xFu);
  }

  [[nodiscard]] uint8_t  crumb(uint32_t index) const noexcept {
    return static_cast<uint8_t>((u32 >> (index * 2)) & 0x3u);
  }

  [[nodiscard]] bool     bit(uint32_t bitNo) const noexcept {
    return (u32 & bitNo) != 0;
  }
};

template <size_t N>
class static_string {
  uint32_t m_length;
  char     m_data[N];
public:

  static_string() = default;

  [[nodiscard]] char*        data() noexcept {
    return m_data;
  }

  [[nodiscard]] const char*  data() const noexcept {
    return m_data;
  }

  [[nodiscard]] size_t       size() const noexcept {
    return m_length;
  }

  void                       set_size() noexcept {
    const uint32_t end = (uint32_t)N;
    for (uint32_t i = 0; i < end; i += 1) {
      if (m_data[i] == '\0') {
        m_length = i;
        return;
      }
    }
    m_length = end;
  }

  void                       assign(uint32_t index, cpu_register reg) noexcept {
    std::memcpy(&m_data[index * sizeof(cpu_register)], reg.chars(), sizeof(cpu_register));
  }

  [[nodiscard]] agt_string_t view() const noexcept {
    return { m_data, m_length };
  }
};

using vendor_string_t = static_string<12>;
using brand_string_t  = static_string<32>;

using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;


enum {
  CACHE_L1D,
  CACHE_L1I,
  CACHE_L2,
  CACHE_L3,
  MAX_CACHE_COUNT
};



inline constexpr static size_t MaximumCacheInfoCount = 8;
inline constexpr static size_t CStateCount = 8;

inline constexpr static size_t MaxXSAVEFeatureCount = std::countr_zero(cpu::MAX_XSAVE_BIT_PLUS_ONE - 1) + 1;

enum {
  LEAF_01_ecx,
  LEAF_01_edx,
  LEAF_07_0_ebx,
  LEAF_0D_eax
};


struct alignas(AGT_CACHE_LINE) static_cpuinfo {
  vendor_string_t vendorString;
  brand_string_t  brandString;


  u8  steppingID    : 4;
  u8  processorType : 2;
  // u8                : 2;
  u8  modelID;
  u16 familyID;

  u8  brandIndex;
  u8  cacheLineSize;
  u16 maxLogicalProcessorID;

  u32 has_SSE3       : 1;
  u32 has_PCLMULQDQ  : 1;
  u32 has_DTES64     : 1;
  u32 has_MONITOR    : 1;
  u32 has_DS_CPL     : 1;
  u32 has_VMX        : 1;
  u32 has_SMX        : 1;
  u32 has_EIST       : 1;
  u32 has_TM2        : 1;
  u32 has_SSSE3      : 1;
  u32 has_CNTX_ID    : 1;
  u32 has_SDBG       : 1;
  u32 has_FMA        : 1;
  u32 has_CMPXCHG16B : 1;
  u32 has_xTPR       : 1;
  u32 has_PDCM       : 1;
  u32 _reserved_1    : 1;
  u32 has_PCID       : 1;
  u32 has_DCA        : 1;
  u32 has_SSE4_1     : 1;
  u32 has_SSE4_2     : 1;
  u32 has_x2APIC     : 1;
  u32 has_MOVBE      : 1;
  u32 has_POPCNT     : 1;
  u32 has_TSC_Deadline : 1;
  u32 has_AES        : 1;
  u32 has_XSAVE      : 1;
  u32 has_OSXSAVE    : 1;
  u32 has_AVX        : 1;
  u32 has_F16C       : 1;
  u32 has_RDRAND     : 1;
  u32 _reserved_2    : 1;

  u32 has_x87        : 1;
  u32 has_VME        : 1;
  u32 has_DebugExtensions : 1;
  u32 has_PSE        : 1;
  u32 has_TSC        : 1;
  u32 has_MSR        : 1;
  u32 has_PAE        : 1;
  u32 has_MCE        : 1;
  u32 has_CX8        : 1;
  u32 has_APIC       : 1;
  u32 _reserved_3    : 1;
  u32 has_SEP        : 1;
  u32 has_MTRR       : 1;
  u32 has_PGE        : 1;
  u32 has_MCA        : 1;
  u32 has_CMOV       : 1;
  u32 supports_PAT   : 1;
  u32 supports_36bit_PageSize : 1;
  u32 has_ProcessorSerialNumber : 1;
  u32 has_CLFLUSH    : 1;
  u32 _reserved_4    : 1;
  u32 has_DebugStore : 1;
  u32 has_ACPI       : 1;
  u32 has_MMX        : 1;
  u32 has_FXSR       : 1;
  u32 has_SSE        : 1;
  u32 has_SSE2       : 1;
  u32 can_SelfSnoop  : 1;
  u32 supports_maxAPICField : 1;
  u32 has_TermalMonitoring : 1;
  u32 _reserved_5    : 1;
  u32 supports_PBE   : 1;

  u32 has_FSGSBASE : 1;
  u32 supports_TSC_ADJUST_MSR : 1;
  u32 has_SGX : 1;
  u32 has_BMI1 : 1;
  u32 has_HLE : 1;
  u32 has_AVX2 : 1;
  u32 fdpExceptionOnly : 1;
  u32 supports_SMEP : 1;
  u32 has_BMI2 : 1;
  u32 supports_enhancedMOVSB : 1;
  u32 has_INVPCID : 1;
  u32 has_RTM : 1;
  u32 supports_RDTMonitoring : 1;
  u32 deprecated_CS_DS : 1;
  u32 supports_MPX : 1;
  u32 supports_RDTAllocation : 1;
  u32 has_AVX512F : 1;
  u32 has_AVX512DQ : 1;
  u32 has_RDSEED   : 1;
  u32 has_ADX : 1;
  u32 supports_SMAP : 1;
  u32 has_AVX512_IFMA : 1;
  u32 _reserved_: 1;
  u32 has_CLFLUSHOPT : 1;
  u32 has_CLWB : 1;
  u32 supports_IntelProcessorTrace : 1;
  u32 has_AVX512PF : 1;
  u32 has_AVX512ER : 1;
  u32 has_AVX512CD : 1;
  u32 has_SHA : 1;
  u32 has_AVX512BW : 1;
  u32 has_AVX512VL : 1;

  u32 has_XSAVEOPT : 1;
  u32 has_XSAVEC   : 1;
  u32 supports_XGETBV_withECX_equals_1 : 1;
  u32 has_XSAVES   : 1;
  u32 supports_extendedFeatureDisable : 1;
  u32 _reserved_6 : 27;

  /*union {
    struct {

    };
    u32 _leaf_1_ecx;
  };*/

  /*union {
    struct {
      u32 has_x87        : 1;
      u32 has_VME        : 1;
      u32 has_DebugExtensions : 1;
      u32 has_PSE        : 1;
      u32 has_TSC        : 1;
      u32 has_MSR        : 1;
      u32 has_PAE        : 1;
      u32 has_MCE        : 1;
      u32 has_CX8        : 1;
      u32 has_APIC       : 1;
      u32 _reserved_3    : 1;
      u32 has_SEP        : 1;
      u32 has_MTRR       : 1;
      u32 has_PGE        : 1;
      u32 has_MCA        : 1;
      u32 has_CMOV       : 1;
      u32 supports_PAT   : 1;
      u32 supports_36bit_PageSize : 1;
      u32 has_ProcessorSerialNumber : 1;
      u32 has_CLFLUSH    : 1;
      u32 _reserved_4    : 1;
      u32 has_DebugStore : 1;
      u32 has_ACPI       : 1;
      u32 has_MMX        : 1;
      u32 has_FXSR       : 1;
      u32 has_SSE        : 1;
      u32 has_SSE2       : 1;
      u32 can_SelfSnoop  : 1;
      u32 supports_maxAPICField : 1;
      u32 has_TermalMonitoring : 1;
      u32 _reserved_5    : 1;
      u32 supports_PBE   : 1;
    };
    u32 _leaf_1_edx;
  };*/

  /*union {
    struct {
      u32 has_FSGSBASE : 1;
      u32 supports_TSC_ADJUST_MSR : 1;
      u32 has_SGX : 1;
      u32 has_BMI1 : 1;
      u32 has_HLE : 1;
      u32 has_AVX2 : 1;
      u32 fdpExceptionOnly : 1;
      u32 supports_SMEP : 1;
      u32 has_BMI2 : 1;
      u32 supports_enhancedMOVSB : 1;
      u32 has_INVPCID : 1;
      u32 has_RTM : 1;
      u32 supports_RDTMonitoring : 1;
      u32 deprecated_CS_DS : 1;
      u32 supports_MPX : 1;
      u32 supports_RDTAllocation : 1;
      u32 has_AVX512F : 1;
      u32 has_AVX512DQ : 1;
      u32 has_RDSEED   : 1;
      u32 has_ADX : 1;
      u32 supports_SMAP : 1;
      u32 has_AVX512_IFMA : 1;
      u32 _reserved_: 1;
      u32 has_CLFLUSHOPT : 1;
      u32 has_CLWB : 1;
      u32 supports_IntelProcessorTrace : 1;
      u32 has_AVX512PF : 1;
      u32 has_AVX512ER : 1;
      u32 has_AVX512CD : 1;
      u32 has_SHA : 1;
      u32 has_AVX512BW : 1;
      u32 has_AVX512VL : 1;
    };
    u32 _leaf_07_0_ebx;
  };*/

  u32 initial_APIC_ID;

  u32 processorSerialNumber[3];

  // cpu::cache_info cacheInfo[MAX_CACHE_COUNT];

  u32             cacheInfoCount;
  u32             cacheInfoIndices[MAX_CACHE_COUNT];
  cpu::cache_info cacheInfo[MaximumCacheInfoCount];


  uint16_t minMonitorLineSize;
  uint16_t maxMonitorLineSize;
  bool     supports_monitorWaitEnumeration;
  bool     supports_mwaitInteruptBreak;
  uint16_t reserved1;
  uint8_t  supportedMWAITSubCStates[CStateCount];

  uint64_t supportedBitsXCR0;
  uint32_t maxXSAVEDataRequiredSize;
  uint32_t maxXSAVEDataXCR0RequiredSize;

  uint64_t supportedBitsXSS;

  u32      sizeOfXSAVEData;

  u32      mask_XSAVEFeatures;

  /*union {
    struct {
      u32 has_XSAVEOPT : 1;
      u32 has_XSAVEC   : 1;
      u32 supports_XGETBV_withECX_equals_1 : 1;
      u32 has_XSAVES   : 1;
      u32 supports_extendedFeatureDisable : 1;
      u32 : 0;
    };
    u32 _leaf_0D_eax;
  };*/

  u32      xsaveFeatureCount;
  const cpu::xsave_feature_info* xsaveFeatureInfos;
  u8       xsaveFeatureIndices[MaxXSAVEFeatureCount];

  uint16_t baseFreqMHz;
  uint16_t maxFreqMHz;
  uint16_t busFreqMHz;
};


namespace {



  template <typename T>
  inline u32* getBitFields(static_cpuinfo& info, T static_cpuinfo::* preField) noexcept {
    uint64_t offset = reinterpret_cast<uintptr_t>(&(((static_cpuinfo*)nullptr)->*preField));
    offset += sizeof(T);
    offset = ((offset - 1) | (alignof(u32) - 1)) + 1;
    return reinterpret_cast<u32*>(reinterpret_cast<std::byte*>(&info) + offset);
  }


}

inline void parseEncodedCacheTLBInfo(static_cpuinfo & info, u8 value) noexcept {
  cpu::cache_info* cache;

  uint8_t lowNibble = value & 0xF;
  uint8_t hiNibble = value >> 4;


  switch ( hiNibble ) {
    case 0xD:
    case 0xE:
      cache = &info.cacheInfo[CACHE_L3];
      cache->lineSize = 64;

  }



  switch (value) {
    case 0x00:
      return;
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x0E:

    case 0x1D:

    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x29:
    case 0x2C:

    case 0x30:

    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:

    case 0x50:


    case 0xA0:

    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0xB6:
    case 0xBA:

    case 0xC0:
    case 0xC1:
    case 0xC2:
    case 0xC3:
    case 0xC4:
    case 0xCA:

    case 0xD0:
      cache = &info.cacheInfo[CACHE_L3];

    case 0xD1:
    case 0xD2:
    case 0xD6:
    case 0xD7:
    case 0xD8:
    case 0xDC:
    case 0xDD:
    case 0xDE:

    case 0xE2:
    case 0xE3:
    case 0xE4:
    case 0xEA:
    case 0xEB:
    case 0xEC:

    case 0xF0:
    case 0xF1:
    case 0xFE:
    case 0xFF:
      break;
  }
}


static_cpuinfo getCpuInfo() noexcept {

  static_cpuinfo info { };
  uint32_t maxLeaf = 0;

  u32*     bitfields = getBitFields(info, &static_cpuinfo::maxLogicalProcessorID);

  bitfields[LEAF_01_ecx] = 0;
  bitfields[LEAF_01_edx] = 0;
  bitfields[LEAF_07_0_ebx] = 0;
  bitfields[LEAF_0D_eax] = 0;

  std::array<int, 4> regs = {};
  auto& eax = (cpu_register&)regs[0];
  auto& ebx = (cpu_register&)regs[1];
  auto& ecx = (cpu_register&)regs[2];
  auto& edx = (cpu_register&)regs[3];

  auto cpuid = [&](uint32_t leaf, uint32_t subleaf = (uint32_t)-1) mutable {
    if (leaf > maxLeaf)
      return false;
    if (subleaf == (uint32_t)-1)
      __cpuid(regs.data(), (int)leaf);
    else
      __cpuidex(regs.data(), (int)leaf, (int)subleaf);
    return !(regs[0] == 0 && regs[1] == 0 && regs[2] == 0 && regs[3] == 0);
  };

  cpuid(0x0);

  maxLeaf = eax.value();

  info.vendorString.assign(0, ebx);
  info.vendorString.assign(1, edx);
  info.vendorString.assign(2, ecx);
  info.vendorString.set_size();


  uint8_t initialAPIC_ID_8bit;

  if (cpuid(1)) {
    info.processorType = eax.crumb(6);
    info.steppingID    = eax.nibble(0);
    uint32_t modelId   = eax.nibble(1);
    uint32_t familyId  = eax.nibble(2);

    if (familyId == 0xF) {
      info.familyID = familyId + eax.range(20, 27);
      info.modelID  = modelId | (eax.nibble(4) << 4);
    }
    else {
      if (familyId == 0x6)
        info.modelID = modelId | (eax.nibble(4) << 4);
      else
        info.modelID = modelId;
      info.familyID = familyId;
    }

    info.brandIndex = ebx.byte(0);
    info.cacheLineSize = ebx.byte(1) * 8;
    info.maxLogicalProcessorID = std::bit_ceil((u16)ebx.byte(2));
    initialAPIC_ID_8bit = ebx.byte(3);

    bitfields[LEAF_01_ecx] = ecx.value();
    bitfields[LEAF_01_edx] = edx.value();
    // info._leaf_1_ecx = ecx.value();
    // info._leaf_1_edx = edx.value();


  }
  else {
    info.processorType = 0;
    info.steppingID = 0;
    info.familyID = 0;
    info.modelID = 0;
    info.brandIndex = 0;
    info.cacheLineSize = 0;
    info.maxLogicalProcessorID = 0;
    info.initial_APIC_ID = 0;
    // info._leaf_1_ecx = 0;
    // info._leaf_1_edx = 0;
  }

  if (cpuid(2)) {
    // assert( eax.byte(0) == 0x01 );
    if (!eax.bit(31)) {
      parseEncodedCacheTLBInfo(info, eax.byte(1));
      parseEncodedCacheTLBInfo(info, eax.byte(2));
      parseEncodedCacheTLBInfo(info, eax.byte(3));
    }

    if (!ebx.bit(31)) {
      parseEncodedCacheTLBInfo(info, ebx.byte(0));
      parseEncodedCacheTLBInfo(info, ebx.byte(1));
      parseEncodedCacheTLBInfo(info, ebx.byte(2));
      parseEncodedCacheTLBInfo(info, ebx.byte(3));
    }

    if (!ecx.bit(31)) {
      parseEncodedCacheTLBInfo(info, ecx.byte(0));
      parseEncodedCacheTLBInfo(info, ecx.byte(1));
      parseEncodedCacheTLBInfo(info, ecx.byte(2));
      parseEncodedCacheTLBInfo(info, ecx.byte(3));
    }

    if (!edx.bit(31)) {
      parseEncodedCacheTLBInfo(info, edx.byte(0));
      parseEncodedCacheTLBInfo(info, edx.byte(1));
      parseEncodedCacheTLBInfo(info, edx.byte(2));
      parseEncodedCacheTLBInfo(info, edx.byte(3));
    }
  }

  if (cpuid(3)) {
    if (info.has_ProcessorSerialNumber) {
      info.processorSerialNumber[0] = ecx.value();
      info.processorSerialNumber[1] = edx.value();
    }
  }


  std::memset(info.cacheInfoIndices, 0xFF, sizeof(info.cacheInfoIndices));

  for (uint32_t i = 0; ; ++i) {
    cpuid(4, i);
    if (eax.nibble(0) == 0) {
      info.cacheInfoCount = i;
      break;
    }

    auto& cache = info.cacheInfo[i];

    cache.kind                    = static_cast<cpu::cache_kind>(eax.nibble(0));
    cache.cacheLevel              = eax.nibble(1);
    cache.isSelfInitializing      = eax.bit(8);
    cache.isFullyAssociative      = eax.bit(9);
    cache.maxLogicalProcessorID   = std::bit_ceil(static_cast<uint16_t>(eax.range(14, 25) + 1));
    cache.maxProcessorCoreID      = std::bit_ceil(static_cast<uint16_t>(eax.range(26, 31) + 1));
    cache.lineSize                = ebx.range(0, 11) + 1;
    cache.physicalLinePartitions  = ebx.range(12, 21) + 1;
    cache.setAssociativity        = ebx.range(22, 31) + 1;
    cache.numberOfSets            = ecx.value();
    cache.cascadingWriteBack      = !edx.bit(0);
    cache.cacheIsInclusive        = edx.bit(1);
    cache.isDirectMapped          = !edx.bit(2);

    cache.size = static_cast<uint64_t>(cache.lineSize) *
                 static_cast<uint64_t>(cache.setAssociativity) *
                 static_cast<uint64_t>(cache.lineSize) *
                 (static_cast<uint64_t>(ecx.value()) + 1);

    if (cache.cacheLevel == 1) {
      if (cache.kind == cpu::cache_kind::data)
        info.cacheInfoIndices[CACHE_L1D] = i;
      else if (cache.kind == cpu::cache_kind::instruction)
        info.cacheInfoIndices[CACHE_L1I] = i;
    }
    else if (cache.cacheLevel == 2 && cache.kind == cpu::cache_kind::unified)
      info.cacheInfoIndices[CACHE_L2] = i;
    else if (cache.cacheLevel == 3 && cache.kind == cpu::cache_kind::unified)
      info.cacheInfoIndices[CACHE_L3] = i;
  }

  if (cpuid(5)) {
    info.minMonitorLineSize = eax.word(0);
    info.maxMonitorLineSize = ebx.word(0);
    info.supports_monitorWaitEnumeration = ecx.bit(0);
    info.supports_mwaitInteruptBreak = ecx.bit(1);
    for (uint32_t i = 0; i < CStateCount; ++i)
      info.supportedMWAITSubCStates[i] = edx.nibble(i);
  }


  if (cpuid(0x7, 0)) {
    uint32_t maxSubLeaf = eax.value();
    bitfields[LEAF_07_0_ebx] = ebx.value();
    // info._leaf_07_0_ebx = ebx.value();
  }


  std::memset(&info.xsaveFeatureIndices, 0xFF, sizeof(info.xsaveFeatureIndices));

  if (cpuid(0x0D, 0)) {
    u32 featureMask;
    u32 xcr0Mask;
    u32 xssMask;

    xcr0Mask = eax.value();
    info.maxXSAVEDataXCR0RequiredSize = ebx.value();
    info.maxXSAVEDataRequiredSize = ecx.value();
    info.supportedBitsXCR0 = xcr0Mask;

    cpuid(0x0D, 1);

    bitfields[LEAF_0D_eax] = eax.value();
    // info._leaf_0D_eax = eax.value();

    info.sizeOfXSAVEData = ebx.value();
    xssMask = ecx.value();
    info.supportedBitsXSS = xssMask;

    featureMask = xcr0Mask | xssMask;

    info.mask_XSAVEFeatures = featureMask;


    if (u32 featureCount = std::popcount(featureMask)) {

      info.xsaveFeatureCount = featureCount;
      auto featureInfos = new cpu::xsave_feature_info[featureCount];

      for (u32 i = 0, j = 0; j < featureCount; ++i) {
        u32 bit = (0x1 << i);
        if (featureMask & bit) {
          if (i >= 2) {
            cpuid(0x0D, i);
            auto& featureInfo = featureInfos[j];
            featureInfo.feature            = static_cast<cpu::xsave_feature>(bit);
            featureInfo.dataSize           = eax.value();
            featureInfo.dataOffset         = ebx.value();
            featureInfo.supportedInXSS_MSR = ecx.bit(0);
            featureInfo.alignInCompactData = ecx.bit(1);
            info.xsaveFeatureIndices[i]    = static_cast<u8>(j);
          }
          ++j;
        }
      }

      info.xsaveFeatureInfos = featureInfos;
    }
    else {
      info.xsaveFeatureCount = 0;
      info.xsaveFeatureInfos = nullptr;
    }
  }
  else {
    info.xsaveFeatureCount = 0;
    info.xsaveFeatureInfos = nullptr;
  }

  if (cpuid(0x16)) {
    if (eax.word(0) != 0)
      info.baseFreqMHz = eax.word(0);
    if (ebx.word(0) != 0)
      info.maxFreqMHz = ebx.word(0);
    if (ecx.word(0) != 0)
      info.busFreqMHz = ecx.word(0);
  }


  u32 x2APIC = 0;


  if (info.supports_maxAPICField) {
    if (info.has_x2APIC)
      info.initial_APIC_ID = x2APIC;
    else
      info.initial_APIC_ID = initialAPIC_ID_8bit;

  }
  else
    info.initial_APIC_ID = 1;


  return info;
}

const static static_cpuinfo g_cpuInfo = getCpuInfo();

const static_cpuinfo & info() noexcept {
  // const static static_cpuinfo cached_cpuInfo = getCpuInfo();
  // return cached_cpuInfo;
    return g_cpuInfo;
}

agt_string_t cpu::brand() noexcept {
  return info().brandString.view();
}

agt_string_t cpu::vendor() noexcept {
  return info().vendorString.view();
}




bool agt::cpu::hasSSE2() noexcept {
  return info().has_SSE2;
}
bool agt::cpu::hasPCLMULQDQ() noexcept {
  return info().has_PCLMULQDQ;
}
bool agt::cpu::hasMONITOR() noexcept {
  return info().has_MONITOR;
}
bool agt::cpu::hasSSE3() noexcept {
  return info().has_SSE3;
}
bool agt::cpu::hasSSSE3() noexcept {
  return info().has_SSSE3;
}
bool agt::cpu::hasFMA() noexcept {
  return info().has_FMA;
}
bool agt::cpu::hasCMPXCHG16B() noexcept {
  return info().has_CMPXCHG16B;
}
bool agt::cpu::hasSSE4_1() noexcept {
  return info().has_SSE4_1;
}
bool agt::cpu::hasSSE4_2() noexcept {
  return info().has_SSE4_2;
}
bool agt::cpu::hasMOVBE() noexcept {
  return info().has_MOVBE;
}
bool agt::cpu::hasPOPCNT() noexcept {
  return info().has_POPCNT;
}
bool agt::cpu::hasAES() noexcept {
  return info().has_AES;
}
bool agt::cpu::hasXSAVE() noexcept {
  return info().has_XSAVE;
}
bool agt::cpu::hasOSXSAVE() noexcept {
  return info().has_OSXSAVE;
}
bool agt::cpu::hasFSGSBASE() noexcept {
  return info().has_FSGSBASE;
}
bool agt::cpu::hasBMI1() noexcept {
  return info().has_BMI1;
}
bool agt::cpu::hasBMI2() noexcept {
  return info().has_BMI2;
}
bool agt::cpu::hasAVX() noexcept {
  return info().has_AVX;
}
bool agt::cpu::hasAVX2() noexcept {
  return info().has_AVX2;
}
bool agt::cpu::hasF16C() noexcept {
  return info().has_F16C;
}
bool agt::cpu::hasRDRAND() noexcept {
  return info().has_RDRAND;
}



uint32_t cpu::getXSAVESupportedFeatures() noexcept {
  return info().mask_XSAVEFeatures;
}
std::span<const cpu::xsave_feature_info> cpu::getXSAVESupportedFeatureInfos() noexcept {
  auto&& cpu = info();
  return { cpu.xsaveFeatureInfos, cpu.xsaveFeatureCount };
}
const cpu::xsave_feature_info* cpu::getXSAVEFeatureInfo(agt::cpu::xsave_feature feature) noexcept {
  u32 bit = feature;
  if (!std::has_single_bit(bit) || (bit > MAX_XSAVE_BIT_PLUS_ONE))
    return nullptr;
  u32 index = std::countr_zero(bit);
  auto&& cpu = info();
  u8 indirectIndex = cpu.xsaveFeatureIndices[index];
  if (indirectIndex == 0xFF)
    return nullptr;
  return cpu.xsaveFeatureInfos + indirectIndex;
}


const cpu::cache_info *agt::cpu::getInfoL1D() noexcept {
  auto&& cpu = info();
  if (cpu.cacheInfoIndices[CACHE_L1D] == (uint32_t)-1)
    return nullptr;
  return &cpu.cacheInfo[cpu.cacheInfoIndices[CACHE_L1D]];
}
const cpu::cache_info *agt::cpu::getInfoL1I() noexcept {
  auto&& cpu = info();
  if (cpu.cacheInfoIndices[CACHE_L1I] == (uint32_t)-1)
    return nullptr;
  return &cpu.cacheInfo[cpu.cacheInfoIndices[CACHE_L1I]];
}
const cpu::cache_info *agt::cpu::getInfoL2() noexcept {
  auto&& cpu = info();
  if (cpu.cacheInfoIndices[CACHE_L2] == (uint32_t)-1)
    return nullptr;
  return &cpu.cacheInfo[cpu.cacheInfoIndices[CACHE_L2]];
}
const cpu::cache_info *agt::cpu::getInfoL3() noexcept {
  auto&& cpu = info();
  if (cpu.cacheInfoIndices[CACHE_L3] == (uint32_t)-1)
    return nullptr;
  return &cpu.cacheInfo[cpu.cacheInfoIndices[CACHE_L3]];
}



uint16_t agt::cpu::getMinMonitorLineSize() noexcept {
  return info().minMonitorLineSize;
}
uint16_t agt::cpu::getMaxMonitorLineSize() noexcept {
  return info().maxMonitorLineSize;
}
bool     agt::cpu::supportsMonitorExtensionEnumeration() noexcept {
  return info().supports_monitorWaitEnumeration;
}
bool     agt::cpu::supportsMWaitInteruptAlwaysBreak() noexcept {
  return info().supports_mwaitInteruptBreak;
}

std::span<const uint8_t> agt::cpu::mwaitSubCStates() noexcept {
  return info().supportedMWAITSubCStates;
}




std::span<const cpu::cache_info> agt::cpu::getCacheInfos() noexcept {
  auto&& cpu = info();
  return { &cpu.cacheInfo[0], cpu.cacheInfoCount };
}
