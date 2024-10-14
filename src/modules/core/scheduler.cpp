//
// Created by maxwe on 2024-10-13.
//

#include "scheduler.hpp"

enum class task_state_value {
  running,
  suspended,
  ready
};