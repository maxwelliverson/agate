//
// Created by maxwe on 2023-08-23.
//

#ifndef AGATE_DYNAMIC_CODE_HPP
#define AGATE_DYNAMIC_CODE_HPP

#include "config.hpp"

#include "vector.hpp"

#include <string_view>


#include <Zydis/Encoder.h>

namespace agt::dc {

  class module;
  class function;

  enum op_code {
    opLoad,
    opStore,
    opMov,
    opCondMov,

    opAdd,
    opSub,
    opAnd,
    opOr,
    opXor,
    opNot,
    opMul,
    opDiv,
    opShr,
    opShl,
    opPopCnt,

    opJmp,
    opCondJmp,
    opCall,
    opRet,
  };

  enum reg {
    r0_8,
    r1_8,
    r2_8,
    r3_8,
    r4_8,
    r5_8,
    r6_8,
    r7_8,
    r8_8,
    r9_8,
    r10_8,
    r11_8,
    r12_8,
    r13_8,
    r14_8,
    r15_8,
    r0_16,
    r1_16,
    r2_16,
    r3_16,
    r4_16,
    r5_16,
    r6_16,
    r7_16,
    r8_16,
    r9_16,
    r10_16,
    r11_16,
    r12_16,
    r13_16,
    r14_16,
    r15_16,
    r0_32,
    r1_32,
    r2_32,
    r3_32,
    r4_32,
    r5_32,
    r6_32,
    r7_32,
    r8_32,
    r9_32,
    r10_32,
    r11_32,
    r12_32,
    r13_32,
    r14_32,
    r15_32,
    r0_64,
    r1_64,
    r2_64,
    r3_64,
    r4_64,
    r5_64,
    r6_64,
    r7_64,
    r8_64,
    r9_64,
    r10_64,
    r11_64,
    r12_64,
    r13_64,
    r14_64,
    r15_64,

    no_register = 0x7FFFFFFF
  };

  enum unary_cmp_op {
    cmpIsZero           = 0x1,
    cmpIsPositive       = 0x2,
    cmpIsZeroOrPositive = 0x3,
    cmpIsNegative       = 0x4,
    cmpIsZeroOrNegative = 0x5,
    cmpIsNonZero        = 0x6,

  };

  enum binary_cmp_op {
    cmpEq        = 0x1,
    cmpLess      = 0x2,
    cmpLessEq    = 0x3,
    cmpGreater   = 0x4,
    cmpGreaterEq = 0x5,
    cmpNotEq     = 0x6
  };

  namespace x64 {
    enum gp_reg {
      al,
      bl,
      cl,
      dl,
      sil,
      dil,
      bpl,
      spl,
      r9b,
      r10b,
      r11b,
      r12b,
      r13b,
      r14b,
      r15b,

      ax,
      bx,
      cx,
      dx,
      si,
      di,
      bp,
      sp,
      r9w,
      r10w,
      r11w,
      r12w,
      r13w,
      r14w,
      r15w,

      eax,
      ebx,
      ecx,
      edx,
      esi,
      edi,
      ebp,
      esp,
      r9d,
      r10d,
      r11d,
      r12d,
      r13d,
      r14d,
      r15d,

      rax,
      rbx,
      rcx,
      rdx,
      rsi,
      rdi,
      rbp,
      rsp,
      r9,
      r10,
      r11,
      r12,
      r13,
      r14,
      r15,
    };

    enum avx_reg {

    };

  }


  class module;
  class function;
  class label;
  class module_builder;
  class function_builder;



  class module {
    friend class module_builder;
    class impl;
  public:


  private:
    impl* pImpl;
  };

  class label{ };

  class function {
    friend class module;
    class impl;
  public:


  private:
  };

  class function_builder {
    friend class module_builder;
    class impl;
  public:

    function_builder& load(reg dst, const void* address) noexcept {}

    function_builder& load(reg dst, reg srcAddress, size_t offset = 0) noexcept {}

    function_builder& loadIndexed(reg dst, const void* address, reg index, int stride) noexcept {}

    function_builder& loadIndexed(reg dst, const void* address, size_t offset, reg index, int stride) noexcept {}

    function_builder& store(void* address, reg src) noexcept {

    }

    function_builder& store(reg dstAddress, reg src, size_t offset = 0) noexcept {

    }

    function_builder& storeIndexed(void* address, reg src, reg index, int stride) noexcept {

    }

    function_builder& storeIndexed(reg dstAddress, reg src, size_t offset, reg index, int stride) noexcept {

    }


    function_builder& move(reg dst, reg src) noexcept { }


    function_builder& moveIf(reg dst, reg src, unary_cmp_op cmpOp, reg cmpReg) noexcept {

    }

    function_builder& moveIf(reg dst, reg src, binary_cmp_op cmpOp, reg leftCmpReg, reg rightCmpReg) noexcept {

    }




    function_builder& jmp(std::string_view label) noexcept {}
    function_builder& jmp(reg address) noexcept {}
    function_builder& jmp(const void* address) noexcept {}

    function_builder& jmpIf(std::string_view label, unary_cmp_op cmpOp, reg cmpReg) noexcept {

    }
    function_builder& jmpIf(reg address, unary_cmp_op cmpOp, reg cmpReg) noexcept {

    }
    function_builder& jmpIf(const void* address, unary_cmp_op cmpOp, reg cmpReg) noexcept {

    }

    function_builder& jmpIf(std::string_view label, binary_cmp_op cmpOp, reg leftCmpReg, reg rightCmpReg) noexcept {

    }
    function_builder& jmpIf(reg address, binary_cmp_op cmpOp, reg leftCmpReg, reg rightCmpReg) noexcept {

    }
    function_builder& jmpIf(const void* address, binary_cmp_op cmpOp, reg leftCmpReg, reg rightCmpReg) noexcept {

    }


    function_builder& call(std::string_view func) noexcept {

    }
    function_builder& call(reg r) noexcept {

    }
    function_builder& call(const void* address) noexcept {

    }

    function_builder& ret() noexcept {

    }


    function_builder& getStackPointer(reg dst) noexcept {}

    function_builder& putStackPointer(reg src) noexcept {}

    function_builder& getFramePointer(reg dst) noexcept {}

    function_builder& putFramePointer(reg src) noexcept {}



    function_builder& push(reg src) noexcept {

    }

    function_builder& pop(reg dst) noexcept {

    }

  };

  class asm_func {
  public:

  private:
    vector<ZyanU8, 0> m_buffer;
  };

  /*void test() {

    function_builder b;

    b.getFramePointer(r8_64)
     .push(r8_64)
     .getStackPointer(r8_64)
     .putFramePointer(r8_64);

    b.call(r1_64);
  }*/

  class module_builder {
    class impl;
  public:

  private:
    impl* pImpl;
  };




}// namespace agt

#endif//AGATE_DYNAMIC_CODE_HPP
