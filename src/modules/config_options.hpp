//
// Created by Maxwell on 2024-02-01.
//

#ifndef AGATE_INTERNAL_CONFIG_OPTIONS_HPP
#define AGATE_INTERNAL_CONFIG_OPTIONS_HPP

#include "agate/map.hpp"
#include "agate/parse.hpp"
#include "agate/sys_string.hpp"
#include "agate/vector.hpp"
#include "init.hpp"

#include <cstdint>

namespace agt::init {

  // bit0: equals
  // bit1: less
  // bit2: greater
  enum cmp_op {
    CMP_UNKNOWN    = 0,
    CMP_EQ         = 1,
    CMP_LESS       = 2,
    CMP_LESS_EQ    = 3,
    CMP_GREATER    = 4,
    CMP_GREATER_EQ = 5,
    CMP_NEQ        = 6
  };

  template <typename T>
  struct constraint {
    T                          value;
    cmp_op                     cmpOp = CMP_UNKNOWN;
    agt_init_necessity_t       necessity;
    const agt_config_option_t* option;
  };

  template <typename T>
  struct constraint_range {
    std::optional<constraint<T>> strictLowerBound; // strictLowerBound <= tightLowerBound <= exactBound <= tightUpperBound <= strictUpperBound
    std::optional<constraint<T>> tightLowerBound;
    std::optional<constraint<T>> strictUpperBound;
    std::optional<constraint<T>> tightUpperBound;
    std::optional<constraint<T>> exactBound;
    vector<constraint<T>, 0>     bannedValues; // neq
  };

  template <typename T>
  struct constraint_set {
    std::optional<constraint<T>> desiredValue;
    vector<constraint<T>, 0>     bannedValues; // neq
  };

  template <typename T>
  struct option_traits {

    // static std::optional<T> extractValue(const agt_config_option_t& option, init_manager& manager) noexcept;

    // static void             resolveConflict(any_vector<constraint<T>>& vec, const constraint<T>& newConstraint, bool allowOverride, init_manager& manager) noexcept;

    // static std::optional<T> resolveConstraints(const any_vector<constraint<T>>& constraints, init_manager& manager) noexcept;

    // static std::optional<T> getEnvVar(std::string_view name, environment& env, init_manager& manager) noexcept;
  };

  template <std::integral I>
  struct option_traits<I> {

    using optional = std::optional<I>;
    using cons     = constraint<I>;

    using set_type = constraint_range<I>;

    inline constexpr static auto TypeValue = dtl::value_type_enum_v<I>;
    inline constexpr static auto IsUnsigned = std::is_unsigned_v<I>;
    inline constexpr static auto Is32Bit = sizeof(I) == sizeof(uint32_t);

    static optional extractValue(const agt_config_option_t& option, init_manager& manager) noexcept {
      switch (option.type) {
        case AGT_TYPE_INT32:
          if constexpr (IsUnsigned) {
            if (option.value.int32 < 0) {
              manager.reportBadNegativeValue(option);
              return std::nullopt;
            }
          }
          return static_cast<I>(option.value.int32);
        case AGT_TYPE_INT64:
          if constexpr (IsUnsigned) {
            if (option.value.int64 < 0) {
              manager.reportBadNegativeValue(option);
              return std::nullopt;
            }
            if constexpr (Is32Bit) {
              if (UINT32_MAX < option.value.int64) {
                manager.reportOptionTypeOutOfRange(option, TypeValue);
                return std::nullopt;
              }
            }
          }
          else if (TypeValue == AGT_TYPE_INT32) {
            if (option.value.int64 < INT32_MIN || INT32_MAX < option.value.int64) {
              manager.reportOptionTypeOutOfRange(option, TypeValue);
              return std::nullopt;
            }
          }
          return static_cast<I>(option.value.int64);
        case AGT_TYPE_UINT32:
          if constexpr (TypeValue == AGT_TYPE_INT32) {
            if (0x7FFFFFFFu < option.value.uint32) {
              manager.reportOptionTypeOutOfRange(option, TypeValue);
              return std::nullopt;
            }
          }
          return static_cast<I>(option.value.uint32);
        case AGT_TYPE_UINT64:
          if constexpr (TypeValue == AGT_TYPE_INT32) {
            if (0x7FFFFFFFull < option.value.uint64) {
              manager.reportOptionTypeOutOfRange(option, TypeValue);
              return std::nullopt;
            }
          }
          else if constexpr (TypeValue == AGT_TYPE_UINT32) {
            if (0xFFFFFFFFull < option.value.uint64) {
              manager.reportOptionTypeOutOfRange(option, TypeValue);
              return std::nullopt;
            }
          }
          else if constexpr (TypeValue == AGT_TYPE_INT64) {
            if (0x7FFFFFFFFFFFFFFFull < option.value.uint64) {
              manager.reportOptionTypeOutOfRange(option, TypeValue);
              return std::nullopt;
            }
          }
          return static_cast<I>(option.value.uint64);
        default:
          manager.reportOptionBadType(option, { TypeValue });
          return std::nullopt;
      }
    }

    static void     addConstraintToSet(set_type& set, const cons& newCons, bool allowOverride, init_manager& manager) noexcept {

      const auto isWithinExactBound = [&] (const cons& bound) {
        switch (newCons.cmpOp) {
          case CMP_EQ:
            return newCons.value == bound.value;
          case CMP_LESS_EQ:
            return newCons.value <= bound.value;
          case CMP_LESS:
            return newCons.value < bound.value;
          case CMP_GREATER_EQ:
            return newCons.value >= bound.value;
          case CMP_GREATER:
            return newCons.value > bound.value;
          case CMP_NEQ:
            return newCons.value != bound.value;
          AGT_no_default;
        }
      };

      const auto isWithinLowerBound = [&] (const cons& bound) {
        switch (newCons.cmpOp) {
          case CMP_EQ: [[fallthrough]];
          case CMP_LESS_EQ:
            return bound.value < newCons.value || ((bound.cmpOp & CMP_EQ) && bound.value == newCons.value);
          case CMP_LESS:
            return bound.value < newCons.value;


          case CMP_GREATER_EQ:
            return bound.value >= newCons.value; // Might seem a little weird, but this is essentially saying that the new lower bound is no tighter than the existing one.
          case CMP_GREATER:
            return bound.value > newCons.value || (!(bound.cmpOp & CMP_EQ) && bound.value == newCons.value);
          case CMP_NEQ:
            return true;
          AGT_no_default;
        }
      };

      const auto isWithinUpperBound = [&] (const cons& bound) {
        switch (newCons.cmpOp) {
          case CMP_EQ: [[fallthrough]];
          case CMP_GREATER_EQ:
            return bound.value > newCons.value || ((bound.cmpOp & CMP_EQ) && bound.value == newCons.value);
          case CMP_GREATER:
            return bound.value > newCons.value;
          case CMP_LESS_EQ:
            return bound.value <= newCons.value; // Might seem a little weird, but this is essentially saying that the new lower bound is no tighter than the existing one.
          case CMP_LESS:
            return bound.value < newCons.value || (!(bound.cmpOp & CMP_EQ) && bound.value == newCons.value);
          case CMP_NEQ:
            return true;
          AGT_no_default;
        }
      };

      const auto valueIsBanned = [&](const cons& c) {
        if (c.cmpOp == CMP_EQ) {
          for (size_t i = 0; i < set.bannedValues.size();) {
            auto&& bannedVal = set.bannedValues[i];
            bool removedValue = false;
            if (bannedVal.value == c.value) {
              if (c.necessity < bannedVal.necessity) {
                manager.reportOptionIgnored(*bannedVal.option);
                set.bannedValues.erase(&bannedVal);
                removedValue = true;
              }
              else
                return true;
            }

            if (!removedValue)
              ++i;
          }
        }

        return false;
      };

      const auto addBannedValue = [&](const cons& c) {
        for (auto&& val : set.bannedValues) {
          if (val.value == c.value) {
            if (c.necessity < val.necessity) {
              val = c;
            }
            return;
          }
        }

        set.bannedValues.push_back(c);
      };

      const auto mayOverride = [&](const cons& c) {
        return allowOverride && newCons.necessity <= c.necessity;
      };

      constexpr static auto isLowerBound = [](const cons& c) {
        return c.cmpOp == CMP_GREATER || c.cmpOp == CMP_GREATER_EQ;
      };

      constexpr static auto isUpperBound = [](const cons& c) {
        return c.cmpOp == CMP_LESS || c.cmpOp == CMP_LESS_EQ;
      };

      bool shouldSetExactBound = false;
      bool shouldSetLowerBound = false;
      bool shouldSetUpperBound = false;

      if (set.exactBound) {
        if (!isWithinExactBound(*set.exactBound)) {
          if (mayOverride(*set.exactBound)) {
            manager.reportOptionIgnored(*set.exactBound.value().option);
            set.exactBound.reset();
          }
          else {
            manager.reportOptionIgnored(*newCons.option);
            return;
          }
        }
      }
      else if (newCons.cmpOp == CMP_EQ)
        shouldSetExactBound = true;

      do {
        if (set.tightLowerBound) {
          if (!isWithinLowerBound(*set.tightLowerBound)) {
            if (mayOverride(*set.tightLowerBound)) {
              if (isLowerBound(newCons)) {
                set.tightLowerBound.emplace(newCons);
                if (set.strictLowerBound && newCons.necessity <= set.strictLowerBound.value().necessity)
                  set.strictLowerBound.reset();
              }
              else {
                manager.reportOptionIgnored(*set.tightLowerBound.value().option);
                set.tightLowerBound = std::exchange(set.strictLowerBound, std::nullopt);
                continue; // go again, having replaced the tight bound with the strict bound.
              }
            }
            else {
              manager.reportOptionIgnored(*newCons.option);
              return;
            }

          }
          else if (isLowerBound(newCons) && newCons.necessity < set.tightLowerBound.value().necessity) {

            if (set.strictLowerBound) {
              if (!isWithinLowerBound(*set.strictLowerBound)) {
                if (newCons.necessity <= set.strictLowerBound.value().necessity)
                  set.strictLowerBound.emplace(newCons);
              }
            }
            else
              set.strictLowerBound.emplace(newCons);
          }
        }
        else if (isLowerBound(newCons))
          shouldSetLowerBound = true;
        break;
      } while(true);

      do {
        if (set.tightUpperBound) {
          if (!isWithinUpperBound(*set.tightUpperBound)) {
            if (mayOverride(*set.tightUpperBound)) {
              if (isUpperBound(newCons)) {
                set.tightUpperBound.emplace(newCons);
                if (set.strictUpperBound && newCons.necessity <= set.strictUpperBound.value().necessity)
                  set.strictUpperBound.reset();
              }
              else {
                manager.reportOptionIgnored(*set.tightUpperBound.value().option);
                set.tightUpperBound = std::exchange(set.strictUpperBound, std::nullopt);
                // if there was a distinct strict upper bound, set the tight upper bound to the strict upper bound, set the strict upper bound to nothing, and try again.
                continue;
              }
            }
            else {
              manager.reportOptionIgnored(*newCons.option);
              return;
            }

          }
          else if (isUpperBound(newCons) && newCons.necessity < set.tightUpperBound.value().necessity) {

            if (set.strictUpperBound) {
              if (!isWithinUpperBound(*set.strictUpperBound)) {
                if (newCons.necessity <= set.strictUpperBound.value().necessity)
                  set.strictUpperBound.emplace(newCons);
              }
            }
            else
              set.strictUpperBound.emplace(newCons);
          }
        }
        else if (isUpperBound(newCons))
          shouldSetUpperBound = true;
        break;
      } while(true);


      if (shouldSetExactBound) {
        if (!valueIsBanned(newCons))
          set.exactBound.emplace(newCons);
        else
          manager.reportOptionIgnored(*newCons.option);
      }

      if (shouldSetLowerBound)
        set.tightLowerBound.emplace(newCons);

      if (shouldSetUpperBound)
        set.tightUpperBound.emplace(newCons);

      if (newCons.cmpOp == CMP_NEQ)
        addBannedValue(newCons);
    }

    static void     mergeConstraintSets(set_type& dstSet, const set_type& srcSet, bool allowOverride, init_manager& manager) noexcept {
      // Definitely a more efficient way to do this, but I'll do it the lazy way for now


      if (srcSet.tightLowerBound)
        addConstraintToSet(dstSet, srcSet.tightLowerBound.value(), allowOverride, manager);
      if (srcSet.tightUpperBound)
        addConstraintToSet(dstSet, srcSet.tightUpperBound.value(), allowOverride, manager);
      if (srcSet.exactBound)
        addConstraintToSet(dstSet, srcSet.exactBound.value(), allowOverride, manager);
      if (srcSet.strictLowerBound)
        addConstraintToSet(dstSet, srcSet.strictLowerBound.value(), allowOverride, manager);
      if (srcSet.strictUpperBound)
        addConstraintToSet(dstSet, srcSet.strictLowerBound.value(), allowOverride, manager);

      for (auto&& bannedVal : srcSet.bannedValues)
        addConstraintToSet(dstSet, bannedVal, allowOverride, manager);

    }

    // If it is determined that the default must be used, don't return it!
    static optional resolveConstraintSet(const set_type& set, I defaultValue, init_manager& manager) noexcept {
      return set.exactBound
                .or_else([&] {
                  return set.tightUpperBound;
                })
                .or_else([&] {
                  return set.tightLowerBound;
                })
                .or_else([&] {
                  return set.strictUpperBound;
                })
                .or_else([&] {
                  return set.strictLowerBound;
                })
            .transform([](const cons& c) {
              if (c.cmpOp & CMP_EQ)
                return c.value;
              if (c.cmpOp & CMP_GREATER)
                return c.value + 1;
              return c.value - 1;
            })
            .or_else([&] () -> optional {
              if (!std::ranges::none_of(set.bannedValues, std::bind_back(std::equal_to<I>{}, defaultValue), &cons::value))
                manager.reportFatalError("No valid value for option. Default value has been disallowed.");
              return std::nullopt;
            });
    }

    /*static void     resolveConflict(any_vector<cons>& vec, const cons& newConstraint, bool allowOverride, init_manager& manager) noexcept {

      if (newConstraint.necessity == AGT_INIT_UNNECESSARY)
        return; // this effectively ignores the constraint

      bool isAccountedFor = false;

      // We cache all the changes we wish to make to the constraint list until after fully iterating through.
      // This way, if it turns out that the constraint is already accounted for, no changes need to be made at all,
      // even if some of the first couple constraints checked against might have seemed like they'd conflict.

      std::optional<size_t> newIndex;
      vector<size_t, 0> removeIndices;

      for (size_t i = 0, j = 0; i < vec.size(); ++i) {
        auto& c = vec[i];

        if (c.cmpOp == CMP_EQ) {
          if (c.value == newConstraint.value)
            return; // this constraint is already handled :)

          if (allowOverride && newConstraint.necessity <= c.necessity) {
            manager.reportOptionIgnored(*c.option);
            if (!newIndex)
              newIndex.emplace(j);
          }
          else {
            manager.reportOptionIgnored(*newConstraint.option);
            return;
          }
        }
        else if (c.cmpOp & AGT_VALUE_GREATER_THAN && c.cmpOp != AGT_VALUE_NOT_EQUALS) {
          if (c.value < newConstraint.value || (c.cmpOp == CMP_GREATER_EQ && c.value == newConstraint.value)) {

          }
        }
        else {

        }
      }

      if (newConstraint.cmpOp == CMP_EQ) { // most common case, and also the most restrictive. Handle totally seperately.
        for (auto&& c : vec) {
          if (c.cmpOp == CMP_EQ) {
            if (c.value == newConstraint.value)
              return; // this constraint is already handled :)

            if (allowOverride && newConstraint.necessity <= c.necessity) {
              manager.reportOptionIgnored(*c.option);
              c = newConstraint;
            }
            else
              manager.reportOptionIgnored(*newConstraint.option);
          }

        }
      }
      else {

      }


    }

    static optional resolveConstraints(const any_vector<cons>& constraints, init_manager& manager) noexcept {

    }*/

    static optional getEnvVar(std::string_view name, environment& env, init_manager& manager) noexcept {
      return env.get(name).and_then([&](std::string_view value) {
        auto result = parse_integer<I>(value);
        if (!result)
          manager.reportBadEnvVarValue(name, value, TypeValue);
        return result;
      });
    }
  };

  template <>
  struct option_traits<version> {

    using base_traits = option_traits<int32_t>;

    using optional = std::optional<version>;
    using cons     = constraint<version>;
    using set_type = base_traits::set_type;


    static optional extractValue(const agt_config_option_t& option, init_manager& manager) noexcept {
      return base_traits::extractValue(option, manager).transform(&version::from_integer);
    }

    static void     addConstraintToSet(set_type& set, const cons& newCons, bool allowOverride, init_manager& manager) noexcept {
      base_traits::addConstraintToSet(set, reinterpret_cast<const base_traits::cons&>(newCons), allowOverride, manager);
    }

    static void     mergeConstraintSets(set_type& dstSet, const set_type& srcSet, bool allowOverride, init_manager& manager) noexcept {
      base_traits::mergeConstraintSets(dstSet, srcSet, allowOverride, manager);
    }

    static optional resolveConstraintSet(const set_type& set, version defaultValue, init_manager& manager) noexcept {
      return base_traits::resolveConstraintSet(set, defaultValue.to_i32(), manager).transform(&version::from_integer);
    }

    static optional getEnvVar(std::string_view name, environment& env, init_manager& manager) noexcept {
      return env.get(name).and_then([&](std::string_view value) {
        auto result = parse_version(value);
        if (!result)
          manager.reportBadVersionString(value);
        return result;
      });
    }
  };

  template <>
  struct option_traits<bool> {
    using optional = std::optional<bool>;
    using cons     = constraint<bool>;

    using set_type = constraint_set<bool>;

    static optional extractValue(const agt_config_option_t& option, init_manager& manager) noexcept {
      return std::nullopt;
    }

    static void     addConstraintToSet(set_type& set, const cons& newCons, bool allowOverride, init_manager& manager) noexcept {

    }

    static void     mergeConstraintSets(set_type& dstSet, const set_type& srcSet, bool allowOverride, init_manager& manager) noexcept {

    }

    static optional resolveConstraintSet(const set_type& set, bool defaultValue, init_manager& manager) noexcept {
      return std::nullopt;
    }

    static optional getEnvVar(std::string_view name, environment& env, init_manager& manager) noexcept {
      return std::nullopt;
    }
  };

  template <typename Char> requires requires { typename std::char_traits<Char>::char_type; }
  struct option_traits<const Char*> {
    using optional = std::optional<const Char*>;
    using cons     = constraint<const Char*>;

    using set_type = constraint_set<const Char*>;

    static optional extractValue(const agt_config_option_t& option, init_manager& manager) noexcept {
      return std::nullopt;
    }

    static void     addConstraintToSet(set_type& set, const cons& newCons, bool allowOverride, init_manager& manager) noexcept {

    }

    static void     mergeConstraintSets(set_type& dstSet, const set_type& srcSet, bool allowOverride, init_manager& manager) noexcept {

    }

    static optional resolveConstraintSet(const set_type& set, const Char* defaultValue, init_manager& manager) noexcept {
      return std::nullopt;
    }

    static optional getEnvVar(std::string_view name, environment& env, init_manager& manager) noexcept {
      return std::nullopt;
    }
  };



  template <typename T>
  class opt {

    using traits = option_traits<T>;

    struct node {
      node*                     parent = nullptr;
      vector<node*, 0>          children;
      typename traits::set_type constraints;
      bool                      allowsEnvironmentOverride = true;
      bool                      allowsSubmoduleOverride = true;
    };





    T                                 m_default = {};
    bool                              m_usedDefault = false;
    std::optional<T>                  m_resolvedValue;
    map<nonnull<agt_config_t>, node*> m_nodeMap;
    node*                             m_rootNode = nullptr;
    std::string_view                  m_envVarName;
    agt_config_id_t                   m_configId;

    static cmp_op getCmpOp(agt_config_flags_t flags) noexcept {
      constexpr static auto FlagMask = AGT_VALUE_EQUALS | AGT_VALUE_LESS_THAN | AGT_VALUE_GREATER_THAN;
      const auto result = flags & FlagMask;
      return result ? static_cast<cmp_op>(result) : CMP_EQ;
    }

    static std::optional<constraint<T>> toConstraint(const agt_config_option_t& opt, init_manager& initManager) noexcept {
      return traits::extractValue(opt, initManager).transform([&](T val){ return constraint<T>{ val, getCmpOp(opt.flags), opt.necessity, &opt }; });
    }


    void attachConstraintToNode(node* n, const agt_config_option_t& opt, init_manager& initManager) noexcept {
      // Any config_option that disables environment/submodule-overrides overrules any and all that allow it.
      // This is because allowing potentially user-loaded modules/runtime environment variables override
      // any and all config variables may expose security issues for applications that depend on
      // any given configuration.

      // By default, environment/submodule-overrides are allowed, so this logic is simply checking whether it's already enabled before checking the flags.
      if (n->allowsEnvironmentOverride)
        n->allowsEnvironmentOverride = opt.flags & AGT_ALLOW_ENVIRONMENT_OVERRIDE;
      if (n->allowsSubmoduleOverride)
        n->allowsSubmoduleOverride = opt.flags & AGT_ALLOW_SUBMODULE_OVERRIDE;


      if (auto maybeConstraint = toConstraint(opt, initManager)) {
        traits::addConstraintToSet(n->constraints, *maybeConstraint, true, initManager);
      }
    }

    [[nodiscard]] node* newNode(agt_config_t config, const agt_config_option_t* opt, init_manager& initManager) noexcept {
      node* parentNode = nullptr;

      auto pNode = new node;

      if (config->parent != AGT_ROOT_CONFIG) {
        auto [parentIter, parentIsNew] = m_nodeMap.try_emplace(config->parent);
        if (parentIsNew)
          parentIter->value() = newNode(config->parent, nullptr, initManager);
        parentNode = parentIter->value();
        parentNode->children.push_back(pNode);
      }
      else {
        assert( m_rootNode == nullptr );
        m_rootNode = pNode;
      }

      pNode->parent = parentNode;

      if (opt != nullptr)
        attachConstraintToNode(pNode, *opt, initManager);

      return pNode;
    }

    void _destroyNode(node* n) noexcept {
      for (auto child : n->children)
        _destroyNode(child);
      delete n;
    }

    void resolveNodeTree(node* root, init_manager& manager) noexcept {
      for (auto child : root->children) {
        resolveNodeTree(child, manager);
        if (root->allowsSubmoduleOverride && root->allowsEnvironmentOverride)
          root->allowsEnvironmentOverride = child->allowsEnvironmentOverride;

        traits::mergeConstraintSets(root->constraints, child->constraints, root->allowsSubmoduleOverride, manager);
      }
    }

    [[nodiscard]] node* resolveNodeTree(init_manager& initManager) noexcept {
      if (m_rootNode) {
        resolveNodeTree(m_rootNode, initManager);
        return m_rootNode;
      }

      return nullptr;
    }

  public:

    explicit opt(std::string_view envVarName) noexcept : m_envVarName(envVarName) { }

    ~opt() {

      assert( m_nodeMap.empty() || m_rootNode ); // the node map having entries implies that the root node has been set

      if (m_rootNode)
        _destroyNode(m_rootNode);

      /*for (auto pNode : m_nodeMap | std::views::values)
        delete pNode;*/
    }


    void addConstraint(agt_config_t config, const agt_config_option_t& opt, init_manager& initManager) noexcept {
      m_configId = opt.id; // this can be set every time, it should be the same every time past the first...
      auto [iter, isNew] = m_nodeMap.try_emplace(config);
      if (isNew)
        iter->value() = newNode(config, &opt, initManager);
      else
        attachConstraintToNode(iter->value(), opt, initManager);
    }

    void setDefault(T value) noexcept {
      m_default = value;
    }

    [[nodiscard]] bool selectedDefault() const noexcept {
      assert( m_resolvedValue.has_value() );
      return m_usedDefault;
    }

    [[nodiscard]] T resolve(environment& env, init_manager& initManager) noexcept {

      if ( !m_resolvedValue ) {
        bool allowingEnvironmentOverride = true;
        node* finalNode = resolveNodeTree(initManager);

        if (finalNode) {
          m_resolvedValue = traits::resolveConstraintSet(finalNode->constraints, m_default, initManager);
          allowingEnvironmentOverride = finalNode->allowsEnvironmentOverride;
        }

        if (allowingEnvironmentOverride) {
          if (auto envVarValue = traits::getEnvVar(m_envVarName, env, initManager)) {
            if (m_resolvedValue) {
              assert( finalNode != nullptr );
              agt_config_option_t envVarOption;
              envVarOption.value = dtl::value_cast(*envVarValue);
              envVarOption.id = m_configId; // we know this has been set cause there is a resolved value
              envVarOption.necessity = AGT_INIT_REQUIRED;
              envVarOption.type = dtl::value_type_enum_v<T>;
              envVarOption.flags = AGT_OPTION_IS_ENVVAR_OVERRIDE;
              auto constraintValue = toConstraint(envVarOption, initManager);
              assert( constraintValue );
              if (constraintValue) { // JUST TO BE SAFE, given this is dealing with user input...
                traits::addConstraintToSet(finalNode->constraints, *constraintValue, true, initManager);
                m_resolvedValue = traits::resolveConstraintSet(finalNode->constraints, m_default, initManager);
                initManager.reportEnvVarOverride(m_envVarName);
              }
            }
            else
              m_resolvedValue = envVarValue;
          }
        }

        if (!m_resolvedValue) {
          m_resolvedValue.emplace(m_default);
          m_usedDefault = true;
        }
      }

      return *m_resolvedValue;
    }
  };



  /*template <typename T, typename Cmp = std::less<T>>
  class opt : public Cmp {
    std::optional<T> m_defaultValue;
    std::optional<T> m_setValue;
    bool             m_allowEnvOverride = false;
    bool             m_allowSubmoduleOverride = false;
    std::string_view m_envVarName;
  public:

    explicit opt(std::string_view envVarName) noexcept : m_envVarName(envVarName) { }

    template <typename ...Args>
    [[nodiscard]] T& emplace(Args&& ...args) noexcept {
      return m_setValue.emplace(std::forward<Args>(args)...);
    }

    void setDefault(const T& value) noexcept {
      m_defaultValue.emplace(value);
    }

    void setFlags(agt_init_flags_t flags) noexcept {
      if (flags & AGT_ALLOW_SUBMODULE_OVERRIDE)
        m_allowSubmoduleOverride = true;
      if (flags & AGT_ALLOW_ENVIRONMENT_OVERRIDE)
        m_allowEnvOverride = true;
    }

    [[nodiscard]] bool allowsEnvironmentOverride() const noexcept {
      return m_allowEnvOverride;
    }

    [[nodiscard]] bool allowsSubmoduleOverride() const noexcept {
      return m_allowSubmoduleOverride;
    }

    [[nodiscard]] T resolve(environment& env, init_manager& initManager) const noexcept;
  };*/


  struct config_options {
    const uint32_t    internalAsyncSize;
    opt<uint32_t>     threadCount{"AGATE_MAX_THREAD_COUNT"};
    opt<version>      libraryVersion{"AGATE_LIBRARY_VERSION"};
    opt<bool>         sharedContext{"AGATE_INSTANCE_IS_SHARED"};
    opt<bool>         cxxExceptionsEnabled{"AGATE_CXX_EXCEPTIONS_ENABLED"};
    const sys_cstring libraryPath;
    opt<sys_cstring>  sharedNamespace{"AGATE_SHARED_NAMESPACE"};
    const sys_cstring sharedNamespaceDynPtr = nullptr;
    opt<uint32_t>     channelDefaultCapacity{"AGATE_CHANNEL_DEFAULT_CAPACITY"};
    opt<uint32_t>     channelDefaultMessageSize{"AGATE_CHANNEL_DEFAULT_MESSAGE_SIZE"};
    opt<uint32_t>     channelDefaultTimeoutMs{"AGATE_CHANNEL_DEFAULT_TIMEOUT_MS"};
    opt<uint64_t>     durationUnitSizeNs{"AGATE_DURATION_UNIT_SIZE_NS"};
    const uint64_t    nativeDurationUnitSizeNs;
    opt<uint64_t>     fixedChannelSizeGranularity{"AGATE_FIXED_CHANNEL_SIZE_GRANULARITY"};
    const size_t      stackSizeAlignment;
    opt<size_t>       defaultThreadStackSize{"AGATE_DEFAULT_THREAD_STACK_SIZE"};
    opt<size_t>       defaultFiberStackSize{"AGATE_DEFAULT_FIBER_STACK_SIZE"};
    const uint64_t    fullStateSaveMask;
    const size_t      stateSaveSize;


    explicit config_options(init_manager& initManager) noexcept;

    void compileOptions(attributes& attr, environment& env, init_manager& initManager) noexcept;

  private:
    config_options(init_manager& initManager, std::pair<uint64_t, size_t>) noexcept;
  };
}

#endif //AGATE_INTERNAL_CONFIG_OPTIONS_HPP
