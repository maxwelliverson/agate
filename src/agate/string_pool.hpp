//
// Created by maxwe on 2023-08-17.
//

#ifndef AGATE_SUPPORT_STRING_POOL_HPP
#define AGATE_SUPPORT_STRING_POOL_HPP

#include "dictionary.hpp"

namespace agt {

  class string_pool;

  class pooled_string {

    friend class string_pool;

    dictionary_entry<nothing_t>* m_entry = nullptr;

    explicit pooled_string(dictionary_entry<nothing_t>* entry) noexcept
        : m_entry(entry) { }
  public:

    pooled_string() = default;
    pooled_string(const pooled_string&) = default;
    pooled_string(pooled_string&&) noexcept = default;

    pooled_string& operator=(const pooled_string&) = default;
    pooled_string& operator=(pooled_string&&) noexcept = default;

    ~pooled_string() = default;

    [[nodiscard]] const char* data() const noexcept {
      return m_entry->get_key_data();
    }

    [[nodiscard]] size_t size() const noexcept {
      return m_entry->get_key_length();
    }

    [[nodiscard]] size_t length() const noexcept {
      return size();
    }

    [[nodiscard]] bool empty() const noexcept {
      return size() == 0;
    }

    [[nodiscard]] bool is_valid() const noexcept {
      return m_entry != nullptr;
    }

    [[nodiscard]] std::string_view view() const noexcept {
      return { data(), size() };
    }

    friend bool operator==(pooled_string a, pooled_string b) noexcept = default;
  };

  class string_pool {
    dictionary<nothing_t> m_dict;
  public:

    using entry_type = dictionary_entry<nothing_t>;

    string_pool() = default;

    [[nodiscard]] bool contains(std::string_view key) const noexcept {
      return m_dict.contains(key);
    }

    [[nodiscard]] pooled_string get(std::string_view key) noexcept {
      return pooled_string(&*(m_dict.try_emplace(key).first));
    }

    [[nodiscard]] bool empty() const noexcept {
      return m_dict.empty();
    }

    [[nodiscard]] size_t size() const noexcept {
      return m_dict.size();
    }

    void remove(pooled_string str) noexcept {
      if (str.is_valid()) {
        m_dict.remove(str.m_entry);
        str.m_entry->destroy(m_dict.get_allocator());
      }
    }

    bool remove(std::string_view key) noexcept {
      return m_dict.erase(key);
    }

    void clear() noexcept {
      m_dict.clear();
    }
  };
}

#endif//AGATE_SUPPORT_STRING_POOL_HPP
