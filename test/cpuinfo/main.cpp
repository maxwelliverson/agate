//
// Created by maxwe on 2023-08-01.
//

#define AGT_BUILD_STATIC

#include "agate/processor.hpp"

#include <string_view>
#include <iostream>


std::ostream& operator<<(std::ostream& os, agt_string_t str) noexcept {
  return (os << std::string_view{ str.data, str.length });
}


int main() {
  using agt::cpu;

  auto brandStr = cpu::brand();
  auto vendorStr = cpu::vendor();
  std::cout << "CPU Vendor String: " << vendorStr << std::endl;
  std::cout << "CPU Brand String:  " << brandStr << std::endl;
}