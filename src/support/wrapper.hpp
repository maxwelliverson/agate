//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_WRAPPER_HPP
#define JEMSYS_AGATE2_WRAPPER_HPP

#include <agate.h>

namespace Agt {
  
  template <typename WrapperType, typename WrappedType, typename StoredType = WrappedType>
  class Wrapper {
  protected:
    
    StoredType value;
    
    Wrapper() = default;
    
  public:

    AGT_forceinline explicit operator bool() const noexcept {
      return static_cast<bool>(value);
    }
    
    AGT_forceinline WrappedType unwrap() const noexcept {
      return (WrappedType)value;
    }
      
      
    static WrapperType wrap(WrappedType value) noexcept {
      Wrapper w;
      w.value = (StoredType)value;
      return static_cast<WrapperType>(w);
    }
  };
}

#endif//JEMSYS_AGATE2_WRAPPER_HPP
