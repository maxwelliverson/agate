# Agate
**A machine-local agent subsystem**

THIS README IS A WORK IN PROGRESS.

Agate was borne of a desire for a low-overhead message passing framework optimized for
in-process use. In looking around at existing message passing libraries, I found that almost all of them were either
- Primarily designed for interprocess and/or cross-network use, and thus have a lot of overhead that is unnecessary in process local use
- Very heavyweight, requiring users to "buy into" an entire framework

Agate was created to provide an alternative.

## Library Structure

The library is split into two primary components: the *static loader*, and the *modules*.
The majority of the library's functionality is organized into modules, while a select few routines
are a part of the static loader (most notably the library initialization/configuration routines). 

### Modules
The modules contain implementations of the various library routines, seperated into distinct, 
self contained, well, modules. Clients should never link directly to any given module.


### Static Loader
The library component with which clients should link. Can be found as `agate.lib`.


### Initialization
Initialization is straightforward, but highly customizable. The basic pattern is to first acquire a *configuration object* 
by calling `agt_get_config`, subsequently using the `agt_config_*` functions to declare desired parameters, before finally
calling `agt_init` with the given *configuration object*. 

#### Default Initialization
If customized parameters are not required, and you are not linking with a agate-enabled DLL,
`agt_default_init` may simply be called instead. This bypasses the need for *configuration objects*
entirely. This is ideal for small, proof-of-concept type projects.


### Initializing from a DLL
Initializing from within a DLL works in much the same way, but with a couple key differences.
The DLL should export an initialization function, named `dll_init` here for the sake of example,
that takes a *configuration object*. `dll_init` should perform initialization as described above, 
with the except of using the passed `agt_config_t` 
object rather than the `AGT_ROOT_CONFIG` constant in the call to `agt_get_config`. Furthermore, 
the pointer to `agt_ctx_t` passed to `agt_init` must point to memory that will remain valid at 
least until `agt_init` is called on the *root configuration object*. 



### Internals
Effectively, while the static loader exports entry points for every function in the library 
(given the loader's version, at least), it only contains proper implementations of a few functions. \
While functions found in the core module *may* be implemented by the static loader, only the 
following functions are guaranteed to be:
- `agt_get_config`  
- `agt_config_init_modules`  
- `agt_config_init_user_modules`  
- `agt_config_set_options`  
- `agt_init`  
- `agt_default_init`  
- `agt_get_proc_address`  

The intended usage is for a client to either simply call `agt_default_init`, or to
first acquire a *configuration object* (an object of type `agt_config_t`) with `agt_get_config`, 
use the `agt_config_*` functions that control how the library is initialized, and once finished, 
to call `agt_init` with the *configuration object* as an argument. 

When called, `agt_init` does the following:
 1. Determines the *builtin attribute* values based on the *configuration object*.
 2. Determines which ***modules*** are required based on the *configuration object*.
 3. Locates those ***modules*** from the path denoted by the `AGT_ATTR_LIBRARY_PATH` *builtin attribute*
 4. Each ***module*** file exports a single function, henceforth called the *export hook* that takes the
    *builtin attribute* values and declares a set of function descriptors that contain the following:
    - Canonical name of a function
    - Address of an implementation of the named function, selected based on the given *builtin attributes*.
    - Canonical *binding index* for the named function.
 5. The *export hook* from each loaded ***module*** is called, and the function descriptor sets returned by 
    each *export hook* are all merged into one.
 6. The complete descriptor set is then used to populate the *dispatch table* of each statically linked copy
    of the loader.


The above described scheme allows for a library that is very flexible, extensible, optimizable, and future and 
backwards compatible.
The static loader should, as per its name, only ever be statically linked to.


# Agents
Agate *agents* are an implementation of the actor model of concurrent computation. *Agents* are organized into two 
distinct components: *state* and *processes*. An *agent*'s state is private, and may only be accessed from within
an agent's *process*.
Individual *agents* may be bound to an *executor*. Each *agent* may only be bound to a single *executor* at any 
given time, though they may be unbound and subsequently rebound to a different *executor*.

Each `agt_agent_t` object serves as a reference to a given *agent*.  
*Agents* may be *detached* or *reference counted*. They may likewise be *named* or *anonymous*. If an agent is *named*,
handles to it may be found by name lookup. If an *agent* is *detached*, this means that its lifetime is not tied in any
way tied to handles, and rather persists indefinitely until `agt_exit` is called from within one of its *processes*.
*Agents* may only be *detached* if they are *named* (this is because in theory, it does not make sense for an *agent* to 
persist without any open handles and no way to open new handles; *named agents* may still have new handles opened by
name lookup, even if no other handles exist. This restriction may loosen if a reasonable use case is found).

The lifetime of *reference counted agents* depend on open handles. A given handle may represent a *strong reference* or 
a *weak reference*. The primary difference between the two is that *weak references* do not count towards an *agent*'s 
*reference count*; as such, *strong handles* always refer to a *living agent*, whereas *weak handles* may not. *Weak handles*
should primarily be used for breaking reference cycles, and clients should note that operations on *weak handles* have a
non-negligible overhead, as the *handles* must be checked to ensure they refer to a *living agent* before each use. This 
overhead may be front-loaded by converting a *weak handle* into a *strong handle* upfront, subsequently closing the *strong
handle* when done.

 



# Glossary
- ***agent***:
- ***anonymous***:
- ***configuration object***: An object of type `agt_config_t` obtained from a call to `agt_get_config`. It must eventually be used as a parameter to `agt_init`, wherein it is consumed.
- ***detached agent***:
- ***parent configuration object***: A *configuration object*'s *parent* is the *configuration object* passed to the call to `agt_get_config` from which it was acquired.
- ***root configuration object***: A *configuration object* obtained from a call to `agt_get_config` to which 
  `AGT_ROOT_CONFIG` was passed. The *root* of a given *configuration object* is defined recursively as follows: 
  if the given *configuration object* is a *root configuration object*, its *root* is itself. Otherwise, its *root* is the *root* of its *parent configuration object*.
- ***strong reference***:
- ***weak reference***:

## Goals
 - Optimized for in-process use
 - Support interprocess use transparently, though at minimal overhead for those who do not need interprocess capabilities
 - Minimal dependencies
 - Modular


## Design Choices
 - Object based API
 - Dedicated "async" type that abstracts any given asynchronous operation
 - Each object has a **scope**, which is one of the following:
    - **Private**: Thread local, process local. Requires no inter-thread synchronization.
    - **Local**:   Process local, may require synchronization with atomics.
    - **Shared**:  Shared across processes.
 - 

## Undecided Design Choices

 - 



