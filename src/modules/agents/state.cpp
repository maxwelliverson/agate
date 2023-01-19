//
// Created by maxwe on 2022-07-13.
//

#include "state.hpp"
#include "agate/agents.h"


constinit thread_local agt::thread_cache_data agt::tl_state{
    nullptr,
    nullptr
};