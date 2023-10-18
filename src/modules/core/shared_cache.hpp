//
// Created by maxwe on 2023-01-02.
//

#ifndef AGATE_INTERNAL_SHM_CACHE_HPP
#define AGATE_INTERNAL_SHM_CACHE_HPP

#include "config.hpp"
#include "object.hpp"
// #include "object_pool.hpp"

namespace agt {

  // The intent here is to use static polymorphism to provide a shared interface for dealing with
  // shared agate objects, where the basic operations are (undecided)
  //  - makeLocal(shared_object) -> import_object
  //  - updateShared(import_object)
  //  - updateLocal(import_object)
  // The idea here is to have a unified interface that supports local caching, and tries to
  // reduce the number of synchronized operations as possible. This probably needs to be broken
  // down a bit more.
  //

  template <typename T>
  struct shared_traits {
    // using shared_type = T;
    // using local_type = ...;

    // static local_type* make_local(shared_type& type) noexcept;
    // static void        update_shared(local_type& type) noexcept;
    // static void        update_local(local_type& type) noexcept;
  };




  struct shared_cache {

  };




}

#endif//AGATE_INTERNAL_SHM_CACHE_HPP
