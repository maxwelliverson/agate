//
// Created by maxwe on 2023-08-23.
//

#include "dynamic_code.hpp"

#include "dictionary.hpp"

namespace agt {
}// namespace agt


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


class agt::dc::module_builder::impl {
  module::impl* moduleImpl;

public:

};





class agt::dc::function_builder::impl {

public:

};