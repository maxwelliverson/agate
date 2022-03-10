# Agate
**A machine local message passing subsystem**

# Note: this is a work in progress

The motivation for Agate came initially from toying around with Vulkan, a modern graphics API.
One major benefit that Vulkan has over its predecessor, OpenGL, is simply that it has been
carefully designed with modern needs and capabilities in mind, as both the hardware and software
landscape have changed significantly since the first OpenGL specification was written. Some of the
core choices 

Initially, the design of Vulkan's command buffer API confused me a little. Almost none of the 
functions are thread safe with respect to both the command buffer *and* the pool from which the 
buffer was allocated. If I had to take a surface level guess, this likely so that command buffers 
can be allocated and reallocated with an optimized allocation algorithm, all while maintaining 
Vulkan's guarantee of maintaining no shared state by storing allocator state in the command pool 
object. For the sake of flexibility of use, simplicity of driver implementation, and efficiency, 
Vulkan also seeks to be as thread agnostic as possible. To my understanding, most (if not all) of 
Vulkan's thread safe API calls are safe by virtue of a lack of object state modification, rather 
than through the use of synchronization primitives. As such, synchronization of the command buffer 
allocator is left entirely up to callers. 

While this makes sense from a design perspective, when used with standard threading models, it 
makes for weird control flow. Ensuring synchronization of any given command buffer is simple; only 
allow it to be written to by a single thread. When working in a "job" or "task" oriented
environment, this model points one in the direction of having tasks that look something like the
following:

 - Allocate command buffer
 - Write appropriate commands to said buffer
 - Queue command buffer for execution
 - Close command buffer

This setup seems ideal, as it naturally allows for straightforward parallel command buffer creation. 
The one major hitch in this setup is the fact that the command buffer pool must also be synchronized,
and while one could simply create a command buffer pool unique to each thread, this would eliminate
many of the benefits the pooled allocator allows for. Another simple approach would be to control 
access to a given command buffer pool with a mutex, though this strips most of the benefits of working
in a multithreaded environment, as the command buffer creation tasks would have to be run sequentially
(this is assuming the mutex would be acquired for the duration of the entire task; acquiring the mutex
for the duration of individual commands would be even worse performance wise, as the command buffer 
functions are specifically meant to be on the fast path).

All these issues and more are solved however by use of message passing. Imagine we have a thread of
execution that has a non-blocking message queue attached to it. This thread hold unique ownership over 
the command buffer pool, and is the only thread that calls command buffer API functions directly. 
Consumers send messages to the thread to request a command buffer, forward commands with a reference to 
the command buffer in question, and then queue said command buffer for execution. In this scenario, 
tasks do not become blocked and the only overhead is that of writing messages. While latency would be 
added in the time between when a consumer thread finishes "writing" a buffer to when that buffer is 
queued (and thus executed), the inherent overhead of queuing/executing command buffers is significant 
enough that the difference in the vast majority of cases should be negligable.

The other motivating use case for Agate was to help deal with the Uncertainty Principle-esque problem
of logging performance sensitive programs. IO is very slow; this shouldn't be news to anybody. Even 
with buffering helping to speed things up on average, the latency that comes from the occasional
buffer flush is significant enough to have a disproportionate effect on the runtime of any performance
sensitive program. This is especially an issue for programs whose behaviour can change depending on 
timings. Being able to offload logging to a dedicated IO thread would, in some cases, lead to 
significant improvements in logging latency.

In looking around at existing message passing libraries, I found that almost all of them were either
 - Primarily designed for interprocess and/or cross-network use, and thus have a lot of overhead that is unnecessary in process local use
 - Very heavyweight, requiring one to "buy into" an entire framework



## Goals
 - Lightweight
 - Optimized for in-process use
 - Support interprocess use transparently 
 - Minimal dependencies


## Design Choices
 - Handle based API
 - Dedicated "async" type that can be attached to any asynchronous operaiton
 - Each object has a **scope**, which is one of the following:
    - **Private**: Not thread safe, process local.
    - **Local**:   Thread safe, process local.
    - **Shared**:  Thread safe, shared between processes.
 - Proper API use requires handles to be "attached" and "detatched".
   Handle users must attach successfully before use, and must detach when done. Scope/user limits are enforced only during attachment


## Undecided Design Choices

 - Scope of objects integrated as handles (ie. should users be able to implement/"install" their own object types that can then be used as handles? If so, how?)
 - Should 



