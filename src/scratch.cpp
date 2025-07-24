#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#include <fstream>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;

const std::vector<const char *> device_extensions
  = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const std::vector<const char *> validation_layers
  = {"VK_LAYER_KHRONOS_validation"};

/*
#ifndef NDEBUG
    const bool enable_validation_layers = false;
# else
    const bool enable_validation_layers = true;
# endif
*/

const bool enable_validation_layers = true;

class Application
{
public:
  void run()
  {
    init_window();
    init_vulkan();
    main_loop();
    cleanup();
  }

private:
  uint32_t current_frame = 0;
  VkRenderPassCreateInfo render_pass_info{};
  VkSemaphore image_available_semph;
  std::vector<VkCommandBuffer> command_buffers;
  std::vector<VkSemaphore> image_available_semphs;
  std::vector<VkSemaphore> render_finished_semphs;
  std::vector<VkFence> in_flight_fences;
  VkSemaphore render_finished_semph;
  VkFence in_flight_fence;
  VkCommandBuffer command_buffer;
  VkCommandPool command_pool;
  std::vector<VkFramebuffer> swapchain_framebuffers;
  VkPipeline graphics_pipeline;
  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_image_views;
  VkFormat swapchain_image_format;
  VkExtent2D swapchain_extent;
  GLFWwindow *window;
  VkInstance instance;
  VkDevice device;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice physical_device;
  VkSurfaceKHR surface;
  VkQueue present_queue;
  VkQueue graphics_queue;
  VkSwapchainKHR swapchain;
  VkShaderModule vert_module;
  VkShaderModule frag_module;

  struct SwapchainSupportDetails
  {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
  };

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    bool is_complete()
    {
      return (graphics_family.has_value() && present_family.has_value());
    }
  };

  static std::vector<char> read_file(const std::string &filename)
  {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if(!file.is_open())
      {
        throw std::runtime_error("Failed to open file.");
      }
    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();
    return buffer;
  }

  VkSurfaceFormatKHR choose_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR> &available_formats)
  {
    // Selects the best format

    for(const auto &available_format : available_formats)
      {
        if(available_format.format == VK_FORMAT_B8G8R8A8_SRGB
           && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
          {
            return available_format;
          }
      }

    return available_formats.at(0);
  }

  VkPresentModeKHR choose_swap_present_mode(
    const std::vector<VkPresentModeKHR> &available_present_modes)
  {
    for(const auto &available_present_mode : available_present_modes)
      {
        if(available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
          {
            return available_present_mode;
          }
      }
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities)
  {
    if(capabilities.currentExtent.width
       != std::numeric_limits<uint32_t>::max())
      {
        return capabilities.currentExtent;
      }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D actual_extent
      = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actual_extent.width
      = std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);

    actual_extent.height
      = std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actual_extent;
  }

  SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device)
  {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                         nullptr);
    if(format_count != 0)
      {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                             details.formats.data());
      }
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &present_mode_count, nullptr);
    if(present_mode_count != 0)
      {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, surface, &present_mode_count, details.present_modes.data());
      }
    return details;
  }

  QueueFamilyIndices find_queue_families(VkPhysicalDevice device)
  {
    // Given a device, what queues can it support?
    // Queues are where we submit commands, and certain queues
    // are used for certain types of commands.
    QueueFamilyIndices indices;
    uint32_t queue_family_count = 0;
    // This function populates the third argument with a set of
    // queue family properties for the chosen GPU.
    // We don't know how many there are so we fire one call to get
    // the number of queue families, resize a vector according to
    // that number, and populate that vector with the second call.
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             queue_families.data());
    // We can through all the queue families, and find the first
    // queue that can support graphics.
    for(int i = 0; i < queue_family_count; i++)
      {
        VkQueueFamilyProperties queue_family = queue_families.at(i);
        if(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
          {
            indices.graphics_family = i;
          }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &present_support);
        if(present_support)
          {
            indices.present_family = i;
          }

        if(indices.is_complete())
          {
            break;
          }
      }
    return indices;
  }

  void populate_debug_messenger_create_info(
    VkDebugUtilsMessengerCreateInfoEXT &create_info)
  {
    create_info.sType
      = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity
      = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType
      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = nullptr;
  }

  VkResult create_debug_utils_messenger_ext(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
    const VkAllocationCallbacks *p_allocator,
    VkDebugUtilsMessengerEXT *p_debug_messenger)
  {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");

    if(func != nullptr)
      {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
      }
    else
      {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
      }
  }

  void
  destroy_debug_utils_messenger_ext(VkInstance instance,
                                    VkDebugUtilsMessengerEXT debug_messenger,
                                    const VkAllocationCallbacks *p_allocator)
  {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr)
      {
        func(instance, debug_messenger, p_allocator);
      }
  }

  void setup_debug_messenger()
  {
    if(!enable_validation_layers)
      {
        return;
      }
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    populate_debug_messenger_create_info(create_info);
    if(create_debug_utils_messenger_ext(instance, &create_info, nullptr,
                                        &debug_messenger)
       != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to set up debug messenger.");
      }
  }

  bool check_validation_layer_support()
  {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
    for(const char *layer_name : validation_layers)
      {
        std::cout << layer_name << std::endl;
        bool layer_found = false;
        for(const auto &layer_properties : available_layers)
          {
            if(strcmp(layer_name, layer_properties.layerName) == 0)
              {
                layer_found = true;
                break;
              }
          }

        if(!layer_found)
          {
            return false;
          }
      }
    return true;
  }

  void init_window()
  {
    if(!glfwInit())
      {
        throw std::runtime_error("GLFW Initialisation failed.");
      }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "jubaengine", nullptr, nullptr);
  }

  void create_surface()
  {
    if(glfwCreateWindowSurface(instance, window, nullptr, &surface)
       != VK_SUCCESS)
      {
        throw std::runtime_error("Unable to initialise window surface.");
      }
  }

  void init_vulkan()
  {
    std::cout << "Initialising Vulkan." << std::endl;
    create_instance();
    setup_debug_messenger();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_swapchain();
    create_image_views();
    create_render_pass();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_pool();
    create_command_buffers();
    create_sync_objects();
  }

  void create_sync_objects()
  {
    image_available_semphs.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semphs.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
      {
        if(vkCreateSemaphore(device, &semaphore_info, nullptr,
                             &image_available_semphs.at(i))
           != VK_SUCCESS)
          {
            throw std::runtime_error(
              "Failed to create the image_available_semph");
          }

        if(vkCreateSemaphore(device, &semaphore_info, nullptr,
                             &render_finished_semphs.at(i))
           != VK_SUCCESS)
          {
            throw std::runtime_error("Failed to create render_finished_semph");
          }

        if(vkCreateFence(device, &fence_info, nullptr, &in_flight_fences.at(i))
           != VK_SUCCESS)
          {
            throw std::runtime_error("Failed to create in flight fence.");
          }
      }
  }

  void
  record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index)
  {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;
    if(vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to begin recording command buffer.");
      }
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swapchain_framebuffers[image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain_extent;
    VkClearValue clear_color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;
    vkCmdBeginRenderPass(command_buffer, &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphics_pipeline);
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain_extent.width);
    viewport.height = static_cast<float>(swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);
    if(vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to record command buffer.");
      }
  }

  void draw_frame()
  {
    vkWaitForFences(device, 1, &in_flight_fences.at(current_frame), VK_TRUE,
                    UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fences.at(current_frame));
    uint32_t image_index;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                          image_available_semphs.at(current_frame),
                          VK_NULL_HANDLE, &image_index);
    vkResetCommandBuffer(command_buffers.at(current_frame), 0);
    record_command_buffer(command_buffers.at(current_frame), image_index);
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags wait_stages[]
      = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semphs.at(current_frame);
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers.at(current_frame);
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_finished_semphs.at(current_frame);
    if(vkQueueSubmit(graphics_queue, 1, &submit_info,
                     in_flight_fences.at(current_frame))
       != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to submit draw command buffer");
      }
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished_semphs.at(current_frame);
    VkSwapchainKHR swapchains[] = {swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;
    vkQueuePresentKHR(present_queue, &present_info);
    std::cout << current_frame << std::endl;
    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void create_command_buffers()
  {
    VkCommandBufferAllocateInfo alloc_info{};
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = (uint32_t)command_buffers.size();
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    if(vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data())
       != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create command buffers.");
      }
  }

  void create_command_pool()
  {
    QueueFamilyIndices queue_family_indices
      = find_queue_families(physical_device);
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
    if(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool)
       != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create a command pool.");
      }
  }

  void create_framebuffers()
  {
    swapchain_framebuffers.resize(swapchain_image_views.size());

    for(size_t i = 0; i < swapchain_image_views.size(); i++)
      {
        VkImageView attachments[] = {swapchain_image_views[i]};
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swapchain_extent.width;
        framebuffer_info.height = swapchain_extent.height;
        framebuffer_info.layers = 1;
        if(vkCreateFramebuffer(device, &framebuffer_info, nullptr,
                               &swapchain_framebuffers[i])
           != VK_SUCCESS)
          {
            throw std::runtime_error("Failed to create framebuffer");
          }
      }
  }

  void create_render_pass()
  {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    if(vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass)
       != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create render pass.");
      }
  }

  VkShaderModule create_shader_module(const std::vector<char> &src)
  {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = src.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(src.data());

    VkShaderModule shader_module{};
    if(vkCreateShaderModule(device, &create_info, nullptr, &shader_module))
      {
        throw std::runtime_error("Failed to create shader module.");
      }
    return shader_module;
  }

  void create_graphics_pipeline()
  {
    auto vert_src = read_file("shaders/vert.spv");
    auto frag_src = read_file("shaders/frag.spv");

    vert_module = create_shader_module(vert_src);
    frag_module = create_shader_module(frag_src);

    VkPipelineShaderStageCreateInfo vert_create_info{};
    vert_create_info.sType
      = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_create_info.module = vert_module;
    vert_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_create_info{};
    frag_create_info.sType
      = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_create_info.module = frag_module;
    frag_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[]
      = {vert_create_info, frag_create_info};

    VkPipelineVertexInputStateCreateInfo vert_input_info{};
    vert_input_info.sType
      = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_input_info.vertexBindingDescriptionCount = 0;
    vert_input_info.vertexAttributeDescriptionCount = 0;
    vert_input_info.pVertexAttributeDescriptions = nullptr;
    vert_input_info.pVertexBindingDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_assm_info{};
    input_assm_info.sType
      = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assm_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assm_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain_extent.width;
    viewport.height = (float)swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent;

    std::vector<VkDynamicState> dynamic_states
      = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state_ci{};
    dynamic_state_ci.sType
      = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_ci.dynamicStateCount
      = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_ci.pDynamicStates = dynamic_states.data();

    VkPipelineViewportStateCreateInfo viewport_state_ci{};
    viewport_state_ci.sType
      = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_ci.viewportCount = 1;
    viewport_state_ci.scissorCount = 1;
    viewport_state_ci.pViewports = &viewport;
    viewport_state_ci.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType
      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType
      = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attach{};
    color_blend_attach.colorWriteMask
      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attach.blendEnable = VK_FALSE;
    color_blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attach.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attach.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType
      = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attach;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_ci{};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 0;
    pipeline_layout_ci.pSetLayouts = nullptr;
    pipeline_layout_ci.pushConstantRangeCount = 0;
    pipeline_layout_ci.pPushConstantRanges = nullptr;
    if(vkCreatePipelineLayout(device, &pipeline_layout_ci, nullptr,
                              &pipeline_layout)
       != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create a pipeline layout.");
      }

    VkGraphicsPipelineCreateInfo graphics_pipeline_info{};
    graphics_pipeline_info.sType
      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_info.stageCount = 2;
    graphics_pipeline_info.pStages = shader_stages;
    graphics_pipeline_info.pVertexInputState = &vert_input_info;
    graphics_pipeline_info.pInputAssemblyState = &input_assm_info;
    graphics_pipeline_info.pViewportState = &viewport_state_ci;
    graphics_pipeline_info.pRasterizationState = &rasterizer;
    graphics_pipeline_info.pMultisampleState = &multisampling;
    graphics_pipeline_info.pDepthStencilState = nullptr;
    graphics_pipeline_info.pColorBlendState = &color_blending;
    graphics_pipeline_info.pDynamicState = &dynamic_state_ci;
    graphics_pipeline_info.layout = pipeline_layout;
    graphics_pipeline_info.renderPass = render_pass;
    graphics_pipeline_info.subpass = 0;
    graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_info.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                 &graphics_pipeline_info, nullptr,
                                 &graphics_pipeline)
       != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create graphics pipeline.");
      }
  }

  void create_image_views()
  {
    swapchain_image_views.resize(swapchain_images.size());
    for(int i = 0; i < swapchain_images.size(); i++)
      {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchain_images.at(i);
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if(vkCreateImageView(device, &create_info, nullptr,
                             &swapchain_image_views.at(i))
           != VK_SUCCESS)
          {
            throw std::runtime_error("Failed to create image views");
          }
      }
  }

  void create_swapchain()
  {
    SwapchainSupportDetails swapchain_support
      = query_swapchain_support(physical_device);
    VkSurfaceFormatKHR surface_format
      = choose_swap_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode
      = choose_swap_present_mode(swapchain_support.present_modes);
    VkExtent2D extent = choose_swap_extent(swapchain_support.capabilities);
    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if(swapchain_support.capabilities.maxImageCount > 0)
      {
        image_count = std::min(image_count,
                               swapchain_support.capabilities.maxImageCount);
      }
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    QueueFamilyIndices indices = find_queue_families(physical_device);
    uint32_t queue_family_indices[]
      = {indices.graphics_family.value(), indices.present_family.value()};
    if(indices.graphics_family.value() != indices.present_family.value())
      {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
      }
    else
      {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
      }
    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count,
                            swapchain_images.data());
    swapchain_image_format = surface_format.format;
    swapchain_extent = extent;
  }

  std::vector<const char *> get_required_extensions()
  {
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char *> extensions(
      glfw_extensions, glfw_extensions + glfw_extension_count);
    if(enable_validation_layers)
      {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }
    return extensions;
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                 VkDebugUtilsMessageTypeFlagsEXT message_type,
                 const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
                 void *p_user_data)
  {
    if(message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
      {
        std::cerr << "Validation layer: " << p_callback_data->pMessage
                  << std::endl;
      }
    return VK_FALSE;
  }

  void create_instance()
  {
    if(enable_validation_layers && !check_validation_layer_support())
      {
        throw std::runtime_error(
          "Validation layers requested, but not available.");
      }
    // A VKInstance is the bridge between our application and the
    // Vulkan driver.
    // To create it, we need to specify some information in a
    // VkInstanceCreateInfo struct.
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    // When creating an instance, providing some extra information
    // about our application is useful, so that the Vulkan driver
    // can perform some use-specific optimizations.
    // We provide this information through VkApplicationInfo.
    // #ref:
    // https://registry.khronos.org/vulkan/specs/latest/man/html/VkApplicationInfo.html
    VkApplicationInfo app_info{};
    // The structure type maps the object type into a unqiue
    // unsigned int, so that the Vulkan driver knows how to
    // interpret the data structure correctly.
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "jubajam";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "No engine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_4;
    create_info.pApplicationInfo = &app_info;
    // VkInstanceCreateInfo also needs extra information regarding
    // any global extensions. For example, we need to a means
    // for Vulkan to talk to GLFW windowing system.
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    auto extensions = get_required_extensions();
    create_info.enabledExtensionCount
      = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    // Global validation layer
    // Works in a similar way to extensions.
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if(enable_validation_layers)
      {
        create_info.enabledLayerCount
          = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
        populate_debug_messenger_create_info(debug_create_info);
        create_info.pNext
          = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
      }
    else
      {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
      }
    if(vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create a VkInstance");
      }
  }
  void main_loop()
  {
    while(!glfwWindowShouldClose(window))
      {
        glfwPollEvents();
        draw_frame();
      }
  }
  void cleanup()
  {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, image_available_semphs.at(i), nullptr);
        vkDestroySemaphore(device, render_finished_semphs.at(i), nullptr);
        vkDestroyFence(device, in_flight_fences.at(i), nullptr);
    }
    vkDestroyCommandPool(device, command_pool, nullptr);
    for(auto framebuffer : swapchain_framebuffers)
      {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
      }
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);

    for(auto image_view : swapchain_image_views)
      {
        vkDestroyImageView(device, image_view, nullptr);
      }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyShaderModule(device, vert_module, nullptr);
    vkDestroyShaderModule(device, frag_module, nullptr);
    vkDestroyInstance(instance, nullptr);
    vkDestroyDevice(device, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void create_logical_device()
  {
    // Logical devices are a way to interface with their associated
    // physical devices.
    //
    // We look for the first index that supports graphics and present queues.
    QueueFamilyIndices indices = find_queue_families(physical_device);
    // Sometimes, both present and graphics are in the same queue family,
    // other times they may be seperate.
    // So let's handle this with a set, to get all the unique indices of
    // queue families needed for device initialisation.
    std::set<uint32_t> unique_queue_families
      = {indices.graphics_family.value(), indices.present_family.value()};

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

    float queue_priority = 1.0f;
    for(uint32_t queue_family : unique_queue_families)
      {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = indices.graphics_family.value();
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
      }

    VkPhysicalDeviceFeatures device_features{};
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount
      = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount
      = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();
    if(vkCreateDevice(physical_device, &create_info, nullptr, &device)
       != VK_SUCCESS)
      {
        throw std::runtime_error("Error creating a logical device.");
      }
    vkGetDeviceQueue(device, indices.present_family.value(), 0,
                     &present_queue);
    vkGetDeviceQueue(device, indices.present_family.value(), 0,
                     &graphics_queue);
    std::cout << "Initialised logical device." << std::endl;
  }

  void pick_physical_device()
  {
    physical_device = VK_NULL_HANDLE;
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if(device_count == 0)
      {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
      }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    for(const auto &device : devices)
      {
        if(is_device_suitable(device))
          {
            std::cout << "Found suitable device." << std::endl;
            physical_device = device;
            break;
          }
      }

    if(physical_device == VK_NULL_HANDLE)
      {
        throw std::runtime_error("Couldn't find a compatible GPU.");
      }
    std::cout << "Warning, using iGPU" << std::endl;
  }

  bool check_device_extension_support(VkPhysicalDevice device)
  {
    uint32_t extension_count;
    // Grab all of the available extensions from the device.
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                         nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                         available_extensions.data());
    std::set<std::string> required_extensions(device_extensions.begin(),
                                              device_extensions.end());
    // For every available extension of the device, if it is a required one,
    // erase from required extensions.
    for(const auto &available_extension : available_extensions)
      {
        required_extensions.erase(available_extension.extensionName);
      }
    return required_extensions.empty();
  }

  bool is_device_suitable(VkPhysicalDevice device)
  {
    QueueFamilyIndices indices = find_queue_families(device);
    bool extensions_supported = check_device_extension_support(device);
    bool swapchain_adequate = false;
    if(extensions_supported)
      {
        SwapchainSupportDetails swapchain_support
          = query_swapchain_support(device);
        swapchain_adequate = !swapchain_support.formats.empty()
                             && !swapchain_support.present_modes.empty();
      }
    return indices.is_complete() && extensions_supported && swapchain_adequate;
  }
};

int main()
{
  try
    {
      Application app;
      app.run();
    }
  catch(const std::exception &e)
    {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
