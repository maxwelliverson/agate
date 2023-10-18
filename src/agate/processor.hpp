//
// Created by maxwe on 2023-07-31.
//

#ifndef AGATE_INTERNAL_PROCESSOR_HPP
#define AGATE_INTERNAL_PROCESSOR_HPP

#include "config.hpp"

#include <span>

namespace agt {

  // TODO: Need to continue filling out with attribute getters

  class cpu {
  public:

    enum class cache_kind : uint8_t {
      invalid,
      data,
      instruction,
      unified
    };

    struct cache_info {
      uint64_t   size;
      // uint32_t   sizeKB;
      uint32_t   numberOfSets; // this is one less than the true number of sets

      uint8_t    cacheLevel : 4;
      cache_kind kind     : 4;
      bool       hasExtendedInfo;
      bool       isSelfInitializing;
      bool       isFullyAssociative;
      bool       cascadingWriteBack;
      bool       cacheIsInclusive;
      bool       isDirectMapped;
      uint16_t   maxLogicalProcessorID;

      uint16_t   maxProcessorCoreID;
      uint16_t   lineSize;
      uint16_t   physicalLinePartitions;
      uint16_t   setAssociativity;

      // uint16_t   lineSize;
    };

    enum xsave_feature : uint32_t {
      XSAVE_x87       = 0x0001,
      XSAVE_SSE       = 0x0002,
      XSAVE_AVX       = 0x0004,
      XSAVE_MPX_A     = 0x0008,
      XSAVE_MPX_B     = 0x0010,
      XSAVE_AVX_512_A = 0x0020,
      XSAVE_AVX_512_B = 0x0040,
      XSAVE_AVX_512_C = 0x0080,
      XSAVE_PT        = 0x0100,
      XSAVE_PKRU      = 0x0200,
      XSAVE_PASID     = 0x0400,
      XSAVE_CET       = 0x0800,
      XSAVE_CETS      = 0x1000,
      XSAVE_HDC       = 0x2000,
      XSAVE_UINTR     = 0x4000,
      XSAVE_LBR       = 0x8000,
      XSAVE_HWP       = 0x10000,
      XSAVE_TILECFG   = 0x20000,
      XSAVE_TILEDATA  = 0x40000,
      MAX_XSAVE_BIT_PLUS_ONE
    };

    struct xsave_feature_info {
      xsave_feature feature;
      uint32_t      dataSize;
      uint32_t      dataOffset;
      bool          alignInCompactData;
      bool          supportedInXSS_MSR;
    };


    static agt_string_t vendor() noexcept;
    static agt_string_t brand() noexcept;

    static bool hasSSE2() noexcept;
    static bool hasPCLMULQDQ() noexcept;
    static bool hasMONITOR() noexcept;
    static bool hasSSE3() noexcept;
    static bool hasSSSE3() noexcept;
    static bool hasFMA() noexcept;
    static bool hasCMPXCHG16B() noexcept;
    static bool hasSSE4_1() noexcept;
    static bool hasSSE4_2() noexcept;
    static bool hasMOVBE() noexcept;
    static bool hasPOPCNT() noexcept;
    static bool hasAES() noexcept;
    static bool hasXSAVE() noexcept;
    static bool hasOSXSAVE() noexcept;
    static bool hasFSGSBASE() noexcept;
    static bool hasBMI1() noexcept;
    static bool hasBMI2() noexcept;
    static bool hasAVX() noexcept;
    static bool hasAVX2() noexcept;
    static bool hasF16C() noexcept;
    static bool hasRDRAND() noexcept;

    static uint32_t getXSAVESupportedFeatures() noexcept;
    static std::span<const xsave_feature_info> getXSAVESupportedFeatureInfos() noexcept;
    static const xsave_feature_info* getXSAVEFeatureInfo(xsave_feature feature) noexcept;

    static const cache_info* getInfoL1D() noexcept;
    static const cache_info* getInfoL1I() noexcept;
    static const cache_info* getInfoL2()  noexcept;
    static const cache_info* getInfoL3()  noexcept;

    static std::span<const cache_info> getCacheInfos() noexcept;
  };

  class processor {
    class impl;
  public:
    processor() = delete;





  private:

    static impl* _get() noexcept;

    impl* pImpl;
  };


}

#endif//AGATE_INTERNAL_PROCESSOR_HPP
