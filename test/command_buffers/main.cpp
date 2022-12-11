//
// Created by maxwe on 2022-05-26.
//


#include <cstring>

#include "agate.h"


#include <vulkan/vulkan.h>

#define PP_VLK_impl_CONCAT_(a, b) a##b
#define PP_VLK_impl_CONCAT PP_VLK_impl_CONCAT_
#define VLK_concat(...) PP_VLK_impl_CONCAT(__VA_ARGS__)

#define VLK_eat_front(arg)



struct VkFunctions {

  PFN_vkAllocateCommandBuffers   allocateCommandBuffers;
  PFN_vkFreeCommandBuffers       freeCommandBuffers;
  PFN_vkResetCommandBuffer       resetCommandBuffer;
  PFN_vkResetCommandPool         resetCommandPool;

  PFN_vkCmdBeginQuery            cmdBeginQuery;
  PFN_vkCmdEndQuery              cmdEndQuery;
  PFN_vkCmdBeginQueryIndexedEXT  cmdBeginQueryIndexedEXT;
  PFN_vkCmdEndQueryIndexedEXT    cmdEndQueryIndexedEXT;

  PFN_vkCmdBindPipeline          cmdBindPipeline;
  PFN_vkCmdBindIndexBuffer       cmdBindIndexBuffer;
  PFN_vkCmdBindDescriptorSets    cmdBindDescriptorSets;
  PFN_vkCmdBindTransformFeedbackBuffersEXT cmdBindTransformFeedbackBuffersEXT;
  PFN_vkCmdBindVertexBuffers  cmdBindVertexBuffers;
  PFN_vkCmdBindVertexBuffers2 cmdBindVertexBuffers2;

  PFN_vkCmdDraw                  cmdDraw;
};

// #define PP_VK_impl_MESSAGE_TAKE_NAME_I(name) struct msg_##name {
// #define PP_VK_impl_MESSAGE_TAKE_NAME PP_VK_impl_MESSAGE_TAKE_NAME_I

// #define VK_message(args) PP_VK_impl_MESSAGE_TAKE_NAME args

#define VK_message(name) \
  struct name; \
  template <> struct _AGT_msg_info_<name> { inline constexpr static int id = __COUNTER__; }; \
  template <> struct _AGT_dispatch_<_AGT_msg_info_<name>::id> { using type = name; };


#define AGT_open_category namespace AGT_MESSAGE_CATEGORY { \
  template <typename T>                                    \
  struct _AGT_msg_info_;                                     \
  template <int I>                                         \
  struct _AGT_dispatch_;



#define AGT_close_category inline constexpr static size_t _AGT_msg_count_ = __COUNTER__; }

namespace msg_utils {
  template <typename ...Types>
  struct typelist {};

  template <typename Obj>
  using dispatch_function = void(*)(Obj*, const void* msg, size_t msgSize);

  template <template <size_t> typename Category, size_t N, typename ...Types>
  struct table_generator : table_generator<Category, N - 1, typename Category<N - 1>::type, Types...> { };

  template <template <size_t> typename Category, typename ...Types>
  struct table_generator<Category, 0, Types...> {
    using type = typelist<Types...>;
  };

  template <typename Obj, typename Type>
  concept has_sized_handle = requires(Obj* object, const Type* msg, size_t msgSize){
                               object->handle(msg, msgSize);
                             };
  template <typename Obj, typename Type>
  concept has_handle = requires(Obj* object, const Type* msg, size_t msgSize){
                               object->handle(msg);
                             };

  template <typename Obj>
  concept has_catch_unknown = requires(Obj* object, const void* msg, size_t msgSize){
                                object->catch_unknown(msg, msgSize);
                              };

  template <typename Obj, template <size_t> typename Category, size_t N>
  struct message_table {

    template <typename Type>
    inline constexpr static auto dispatch_func_ = [](Obj* obj, const void* msg, size_t msgSize){
      if constexpr(has_sized_handle<Obj, Type>)
        obj->handle(static_cast<Type*>(msg), msgSize);
      else if (has_handle<Obj, Type>)
        obj->handle(static_cast<Type*>(msg));
      else if (has_catch_unknown<Obj>)
        obj->catch_unknown(msg, msgSize);
    };


    dispatch_function<Obj> table_[N];


    message_table() : message_table(typename table_generator<Category, N>::type{}) { }


    friend void dispatch(const message_table& table, void* object, agt_message_id_t msgId, const void* msg, size_t msgSize) noexcept {
      (table.table_[msgId])(static_cast<Obj*>(object), msgId, msg, msgSize);
    }

  private:
    template <typename ...Types>
    explicit message_table(typelist<Types...>) noexcept
        : table_{ dispatch_func_<Types> ... } { }
  };
}


#define AGT_message_table(obj, category)

#define AGT_MESSAGE_CATEGORY cmd_pool_msgs

AGT_open_category

  VK_message(allocate_command_buffers)() {

  };


AGT_close_category




#define VK_MSG_ARGS(...) struct args_t { \
    __VA_ARGS__ };                       \
  const auto args = static_cast<const args_t*>(msg)

class command_pool {
  const VkFunctions* vk;
  VkDevice           device;
  VkCommandPool      pool;

  enum class vkMsg {
    beginQuery,
    beginQueryIndexedEXT,
    endQuery,
    endQueryIndexedEXT,
    draw
  };

  void do_allocate_command_buffers(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        VkResult*                   pResult;
        VkCommandBuffer*            pCommandBuffers;
        VkCommandBufferAllocateInfo allocateInfo;
    );
    auto result = vk->allocateCommandBuffers(device, &args->allocateInfo, args->pCommandBuffers);
    if (args->pResult)
      *args->pResult = result;
  }

  void do_free_command_buffers(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        uint32_t        commandBufferCount;
        VkCommandBuffer commandBuffers[];
    );
    vk->freeCommandBuffers(device, pool, args->commandBufferCount, args->commandBuffers);
  }

  void do_reset_command_buffer(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        VkResult*       pResult;
        VkCommandBuffer commandBuffer;
        VkCommandBufferResetFlags flags;
    );
    VkResult result = vk->resetCommandBuffer(args->commandBuffer, args->flags);
    if (args->pResult)
      *args->pResult = result;
  }

  void do_reset_command_pool(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        VkResult* pResult;
        VkCommandPoolResetFlags flags;
    );
    VkResult result = vk->resetCommandPool(device, pool, args->flags);
    if (args->pResult)
      *args->pResult = result;
  }

  void do_bind_pipeline(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkPipelineBindPoint pipelineBindPoint;
        VkPipeline          pipeline;
    );
    vk->cmdBindPipeline(args->commandBuffer, args->pipelineBindPoint, args->pipeline);
  }

  void do_begin_query(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkQueryPool         queryPool;
        uint32_t            query;
        VkQueryControlFlags flags;
    );
    vk->cmdBeginQuery(args->commandBuffer, args->queryPool, args->query, args->flags);
  }

  void do_begin_query_indexed(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkQueryPool         queryPool;
        uint32_t            query;
        VkQueryControlFlags flags;
        uint32_t            index;
    );
    vk->cmdBeginQueryIndexedEXT(args->commandBuffer, args->queryPool, args->query, args->flags, args->index);
  }

  void do_end_query(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkQueryPool         queryPool;
        uint32_t            query;
    );
    vk->cmdEndQuery(args->commandBuffer, args->queryPool, args->query);
  }

  void do_end_query_indexed(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkQueryPool         queryPool;
        uint32_t            query;
        uint32_t            index;
    );
    vk->cmdEndQueryIndexedEXT(args->commandBuffer, args->queryPool, args->query, args->index);
  }

  void do_draw(const void* msg, size_t msgSize) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer commandBuffer;
        uint32_t        vertexCount;
        uint32_t        instanceCount;
        uint32_t        firstVertex;
        uint32_t        firstInstance;
    );
    vk->cmdDraw(args->commandBuffer, args->vertexCount, args->instanceCount, args->firstVertex, args->firstInstance);
  }

  void do_unknown_message(const void* msg, size_t msgSize) noexcept {}

  void handle_message(agt_message_id_t msgId, const void* msg, size_t msgSize) noexcept {
    switch (static_cast<vkMsg>(msgId)) {
      case vkMsg::beginQuery:
        do_begin_query(msg, msgSize);
        break;
      case vkMsg::beginQueryIndexedEXT:
        do_begin_query_indexed(msg, msgSize);
        break;
      case vkMsg::endQuery:
        do_end_query(msg, msgSize);
        break;
      case vkMsg::endQueryIndexedEXT:
        do_end_query_indexed(msg, msgSize);
        break;
      case vkMsg::draw:
        do_draw(msg, msgSize);
        break;
      default:
        do_unknown_message(msg, msgSize);
    }
  }

  static void callback(void* userData, agt_message_id_t msgId, const void* msg, size_t msgSize) noexcept {
    static_cast<command_pool*>(userData)->handle_message(msgId, msg, msgSize);
  }

public:


};

class command_buffer {
  agt_agent_t     poolAgent;
  VkCommandBuffer handle;

public:

  void gloss() {
    vkCmdBeginQuery();
  }


  void begin_query(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags controlFlags) noexcept {
    vk->cmdBeginQuery();
  }

};



int main() {



}