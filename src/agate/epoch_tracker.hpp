//
// Created by maxwe on 2022-08-09.
//

#ifndef AGATE_SUPPORT_EPOCH_TRACKER_HPP
#define AGATE_SUPPORT_EPOCH_TRACKER_HPP

#include <cstdint>
#include <limits>

namespace agt {

#if defined(AGT_ENABLE_ABI_BREAKING_DEBUGGING)

  /// LLVM Style epoch tracking container debugging
  class debug_epoch_base {
    uint64_t epoch;

  public:
    constexpr debug_epoch_base() : epoch(0) { }

    /// Calling incrementEpoch invalidates all handles pointing into the
    /// calling instance.
    void increment_epoch() { ++epoch; }

    /// The destructor calls incrementEpoch to make use-after-free bugs
    /// more likely to crash deterministically.
    ~debug_epoch_base() { increment_epoch(); }

    /// A base class for iterator classes ("handles") that wish to poll for
    /// iterator invalidating modifications in the underlying data structure.
    /// When LLVM is built without asserts, this class is empty and does nothing.
    ///
    /// HandleBase does not track the parent data structure by itself.  It expects
    /// the routines modifying the data structure to call incrementEpoch when they
    /// make an iterator-invalidating modification.
    ///
    class handle_base {
      const uint64_t* epochAddress;
      uint64_t        epochAtCreation;

    public:
      constexpr handle_base()
          : epochAddress(nullptr),
            epochAtCreation(std::numeric_limits<uint64_t>::max()) { }

      explicit constexpr handle_base(const debug_epoch_base* parent)
          : epochAddress(&parent->epoch),
            epochAtCreation(parent->epoch) {}

      /// Returns true if the DebugEpochBase this Handle is linked to has
      /// not called incrementEpoch on itself since the creation of this
      /// HandleBase instance.
      bool is_handle_in_sync() const noexcept { return *epochAddress == epochAtCreation; }

      /// Returns a pointer to the epoch word stored in the data structure
      /// this handle points into.  Can be used to check if two iterators point
      /// into the same data structure.
      const void* get_epoch_address() const noexcept { return epochAddress; }
    };
  };


#else

  class debug_epoch_base {
  public:
    void increment_epoch() {}

    class handle_base {
    public:
      constexpr handle_base() noexcept = default;
      explicit constexpr handle_base(const debug_epoch_base *) noexcept {}
      bool is_handle_in_sync() const noexcept { return true; }
      const void *get_epoch_address() const noexcept { return nullptr; }
    };
  };

#endif
}

#endif//AGATE_SUPPORT_EPOCH_TRACKER_HPP
