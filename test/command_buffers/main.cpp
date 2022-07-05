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

#define PP_VK_impl_MESSAGE_TAKE_NAME_I(name) struct msg_##name {
#define PP_VK_impl_MESSAGE_TAKE_NAME PP_VK_impl_MESSAGE_TAKE_NAME_I

#define VK_message(args) PP_VK_impl_MESSAGE_TAKE_NAME args


#define AGT_open_category namespace AGT_MESSAGE_CATEGORY { \
  template <int I>                                         \
  struct _AGT_dispatch_;



#define AGT_close_category }

namespace msg_utils {
  template <typename ...Types>
  struct typelist {};

  template <typename Obj>
  using dispatch_function = void(Obj::*)(agt_handle_t, void*, const agt_message_info_t*);

  template <template <size_t> typename Category, size_t N, typename ...Types>
  struct table_generator : table_generator<Category, N - 1, typename Category<N - 1>::type, Types...> { };

  template <template <size_t> typename Category, typename ...Types>
  struct table_generator<Category, 0, Types...> {
    using type = typelist<Types...>;
  };

  template <typename Obj, template <size_t> typename Category, size_t N>
  struct message_table {


    dispatch_function<Obj> table_[N];


    message_table() : message_table(typename table_generator<Category, N>::type{}) { }


    friend void dispatch() noexcept;

  private:
    template <typename ...Types>
    explicit message_table(typelist<Types...>) noexcept : table_{ &Obj::template handle_message<Types> ... } {}
  };
}

#define AGT_MESSAGE_CATEGORY cmd_pool_msgs

AGT_open_category



AGT_close_category




#define VK_MSG_ARGS(...) struct args_t { \
    vkMsg               msg;             \
    uint32_t            reserved;        \
    __VA_ARGS__ };                       \
  const auto args = static_cast<args_t*>(msg_)

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


  void do_allocate_command_buffers(void* msg_) noexcept {
    VK_MSG_ARGS(
        VkResult*                   pResult;
        VkCommandBuffer*            pCommandBuffers;
        VkCommandBufferAllocateInfo allocateInfo;
    );
    auto result = vk->allocateCommandBuffers(device, &args->allocateInfo, args->pCommandBuffers);
    if (args->pResult)
      *args->pResult = result;
  }

  void do_free_command_buffers(void* msg_) noexcept {
    VK_MSG_ARGS(
        uint32_t        commandBufferCount;
        VkCommandBuffer commandBuffers[];
    );
    vk->freeCommandBuffers(device, pool, args->commandBufferCount, args->commandBuffers);
  }

  void do_reset_command_buffers(void* msg_) noexcept {

  }

  void do_reset_command_pool(void* msg_) noexcept {}

  void do_bind_pipeline(void* msg_) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkPipelineBindPoint pipelineBindPoint;
        VkPipeline          pipeline;
    );
    vk->cmdBindPipeline(args->commandBuffer, args->pipelineBindPoint, args->pipeline);
  }

  void do_begin_query(void* msg_) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkQueryPool         queryPool;
        uint32_t            query;
        VkQueryControlFlags flags;
    );
    vk->cmdBeginQuery(args->commandBuffer, args->queryPool, args->query, args->flags);
  }

  void do_begin_query_indexed(void* msg_) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkQueryPool         queryPool;
        uint32_t            query;
        VkQueryControlFlags flags;
        uint32_t            index;
    );
    vk->cmdBeginQueryIndexedEXT(args->commandBuffer, args->queryPool, args->query, args->flags, args->index);
  }

  void do_end_query(void* msg_) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkQueryPool         queryPool;
        uint32_t            query;
    );
    vk->cmdEndQuery(args->commandBuffer, args->queryPool, args->query);
  }

  void do_end_query_indexed(void* msg_) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer     commandBuffer;
        VkQueryPool         queryPool;
        uint32_t            query;
        uint32_t            index;
    );
    vk->cmdEndQueryIndexedEXT(args->commandBuffer, args->queryPool, args->query, args->index);
  }

  void do_draw(void* msg_) noexcept {
    VK_MSG_ARGS(
        VkCommandBuffer commandBuffer;
        uint32_t        vertexCount;
        uint32_t        instanceCount;
        uint32_t        firstVertex;
        uint32_t        firstInstance;
    );
    vk->cmdDraw(args->commandBuffer, args->vertexCount, args->instanceCount, args->firstVertex, args->firstInstance);
  }

  void do_unknown_message(void* msg_) noexcept {}

  void handle_message(void* msg) noexcept {
    switch (*static_cast<const vkMsg*>(msg)) {
      case vkMsg::beginQuery:
        do_begin_query(msg);
        break;
      case vkMsg::beginQueryIndexedEXT:
        do_begin_query_indexed(msg);
        break;
      case vkMsg::endQuery:
        do_end_query(msg);
        break;
      case vkMsg::endQueryIndexedEXT:
        do_end_query_indexed(msg);
        break;
      case vkMsg::draw:
        do_draw(msg);
        break;
      default:
        do_unknown_message(msg);
    }
  }

  static void callback(void* userData, agt_handle_t thisHandle, void* messageData, const agt_message_info_t* messageInfo) noexcept {
    const auto pool = static_cast<command_pool*>(userData);

    pool->handle_message(messageData);
  }

public:


};

class command_buffer {
  const VkFunctions* vk;
  VkCommandPool      pool;
  VkCommandBuffer    handle;

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