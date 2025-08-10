#include <ctime>
#include <vulkan/vulkan_core.h>
#include <chrono>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

constexpr int MAX_FRAMES_IN_FLIGHT = 4;

const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;

const std::vector<const char*> device_extensions
    = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const std::vector<const char*> validation_layers
    = { "VK_LAYER_KHRONOS_validation" };

/*
#ifndef NDEBUG
    const bool enable_validation_layers = false;
# else
    const bool enable_validation_layers = true;
# endif
*/

const bool enable_validation_layers = true;

class Application {
public:
    void run()
    {
        load_wad("assets/game.wad");
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

private:
    enum WadType {
        IWAD,
        PWAD
    };

    struct Lump {
        unsigned int filepos;
        unsigned int size;
        std::string name;
    };

    struct Thing {
        short x_position;
        short y_position;
        short direction;
        short type;
        short flags;

        void print() {
            std::cout   << "{   "
                        << "x_position: " << x_position
                        << ";y_position: " <<y_position
                        << ";direction: " << direction
                        << ";type: " << type 
                        << ";flags: " << flags
                        << "  }" 
                        <<std::endl;
        }
    };

    struct Linedef {
        short start_vertex;
        short end_vertex;
        short flags;
        short special;
        short tag;
        short sidenum_1;
        short sidenum_2;

        void print() {
            std::cout << "{"
            << ";start_vertex: " << start_vertex
            << ";end_vertex: " << end_vertex
            << ";flags: " << flags
            << ";special: " << special
            << ";tag: " << tag
            << ";sidenum_1: " << sidenum_1 
            << ";sidenum_2: " << sidenum_2
            << " }" 
            << std::endl;
        }
    };

    struct Sidedef {
        short x_offset;
        short y_offset;
        std::string upper_texture;
        std::string lower_texture;
        std::string middle_texture;
        short sector_facing;

        void print() {
            std::cout << "{"
            << "| x_offset: " << x_offset
            << "| y_offset: " << y_offset
            << "| upper_texture: " << upper_texture
            << "| lower_texture: " << lower_texture
            << "| middle_texture: " << middle_texture
            << "| sector_facing: " << sector_facing
            << " }"
            << std::endl;
        }
    };

    struct Vert {
        short x_position;
        short y_position;

        void print() {
            std::cout << "{"
            << "| x_position: " << x_position
            << "| y_position: " << y_position 
            << "}" << std::endl;
        }
    };

    struct Seg {
        short start_vertex;
        short end_vertex;
        short angle;
        short linedef_number;
        short direction;
        short offset;

        void print() {
            std::cout << "{"
            << "| start_vertex:" << start_vertex
            << "| end_vertex:" << end_vertex
            << "| angle:" << angle
            << "| linedef_number:" << linedef_number
            << "| direction:" << direction
            << "| offset:" << offset
            << "}" << std::endl;
        }
    };

    struct SSector {
        short seg_count;
        short seg_index;

        void print() {
            std::cout << "{"
            << "| seg_count" << seg_count 
            << "| seg_index" << seg_index
            << "}" << std::endl;
        }
    };

    struct BoundingBox {
        short top;
        short bottom;
        short left;
        short right;
    };

    struct Node {
        short partition_x;
        short partition_y;
        short partition_x_diff;
        short partition_y_diff;
        BoundingBox right_box;
        BoundingBox left_box;
        short right_child;
        short left_child;

        void print() {
            std::cout << "{"
            << "| partition_x: " << partition_x
            << "| partition_y: " << partition_y
            << "| partition_x_diff: " << partition_x_diff
            << "| partition_y_diff: " << partition_y_diff
            << "| right_child: " << right_child
            << "| left_child: " << left_child
            << "}" << std::endl;
        }
    };

    struct Sector {
        short floor_height;
        short ceiling_height;
        std::string floor_texture;
        std::string ceiling_texture;
        short light_level;
        short type;
        short tag_number;

        void print() {
            std::cout << "{"
            << "| floor_height: " << floor_height
            << "| ceiling_height: " << ceiling_height
            << "| floor_texture: " << floor_texture
            << "| ceiling_texture: " << ceiling_texture 
            << "| light_level: " << light_level
            << "| type: " << type
            << "| tag_number: " << tag_number
            << "}" << std::endl;
        }
    };



    struct Map {
        std::vector<Thing> things;
        std::vector<Linedef> linedefs;
        std::vector<Sidedef> sidedefs;
        std::vector<Vert> vertices;
        std::vector<Seg> segs;
        std::vector<SSector> ssectors;
        std::vector<Node> nodes;
        std::vector<Sector> sectors;
    };

    struct WAD {
        WadType wad_type;
        unsigned int num_lumps;
        unsigned int directory_offset;
        std::vector<Lump> lumps;
        std::vector<Map> maps;
    };

    WAD wad;

    char pop(std::vector<char> &data) {
        char value = data.front();
        return value;
    };

    int wad_pointer = 0;
    
    std::string parse_string(const std::vector<char> &data) {
        std::vector<char> buffer;
        for (int i =0; i < 8; i++) {
            buffer.push_back(data.at(wad_pointer));
            wad_pointer ++;
        }
        return std::string(buffer.begin(), buffer.end());
    }

    void parse_things(const std::vector<char> &data) {
        Map &current_map = wad.maps.back();
        Lump &current_lump = wad.lumps.back();
        
        wad_pointer = current_lump.filepos;
        while (wad_pointer < (current_lump.filepos + current_lump.size)) {
            current_map.things.push_back({});
            Thing &current_thing = current_map.things.back();
            current_thing.x_position = parse_short(data);
            current_thing.y_position = parse_short(data);
            current_thing.direction = parse_short(data);
            current_thing.type = parse_short(data);
            current_thing.flags = parse_short(data);
        }
    };

    void parse_linedefs(const std::vector<char> &data) {
        Map &current_map = wad.maps.back();
        Lump &current_lump = wad.lumps.back();
        
        wad_pointer = current_lump.filepos;
        while (wad_pointer < (current_lump.filepos + current_lump.size)) {
            current_map.linedefs.push_back({});
            Linedef &current_linedef = current_map.linedefs.back();
            current_linedef.start_vertex = parse_short(data);
            current_linedef.end_vertex = parse_short(data);
            current_linedef.flags = parse_short(data);
            current_linedef.special = parse_short(data);
            current_linedef.tag = parse_short(data);
            current_linedef.sidenum_1 = parse_short(data);
            current_linedef.sidenum_2 = parse_short(data);
        }
    };

    void parse_sidedefs(const std::vector<char> &data) {
        Map &current_map = wad.maps.back();
        Lump &current_lump = wad.lumps.back();
        wad_pointer = current_lump.filepos;
        while (wad_pointer < (current_lump.filepos + current_lump.size)) {
            current_map.sidedefs.push_back({});
            Sidedef &current_sidedef = current_map.sidedefs.back();
            current_sidedef.x_offset = parse_short(data);
            current_sidedef.y_offset = parse_short(data);
            current_sidedef.upper_texture = parse_string(data);
            current_sidedef.lower_texture = parse_string(data);
            current_sidedef.middle_texture = parse_string(data);
            current_sidedef.sector_facing = parse_short(data);
        }
    };

    void parse_vertices(const std::vector<char> &data) {
        Map &current_map = wad.maps.back();
        Lump &current_lump = wad.lumps.back();
        wad_pointer = current_lump.filepos;
        while (wad_pointer < (current_lump.filepos + current_lump.size)) {
            current_map.vertices.push_back({});
            Vert &current_vertex = current_map.vertices.back();
            current_vertex.x_position = parse_short(data);
            current_vertex.y_position = parse_short(data);
        }
    };

    void parse_segs(const std::vector<char> &data) {
        Map &current_map = wad.maps.back();
        Lump &current_lump = wad.lumps.back();
        wad_pointer = current_lump.filepos;
        while (wad_pointer < (current_lump.filepos + current_lump.size)) {
            current_map.segs.push_back({});
            Seg &current_seg = current_map.segs.back();
            current_seg.start_vertex = parse_short(data);
            current_seg.end_vertex = parse_short(data);
            current_seg.angle = parse_short(data);
            current_seg.linedef_number = parse_short(data);
            current_seg.direction = parse_short(data);
            current_seg.offset = parse_short(data);
        }
    };
    void parse_ssectors(const std::vector<char> &data) {
        Map &current_map = wad.maps.back();
        Lump &current_lump = wad.lumps.back();
        wad_pointer = current_lump.filepos;
        while (wad_pointer < (current_lump.filepos + current_lump.size)) {
            current_map.ssectors.push_back({});
            SSector current_ssector = current_map.ssectors.back();
            current_ssector.seg_count = parse_short(data);
            current_ssector.seg_index = parse_short(data);
        }
    };
    
    BoundingBox parse_bounding_box(const std::vector<char> &data, Node &current_node) {
        BoundingBox bounding_box;
        bounding_box.top = parse_short(data);
        bounding_box.bottom = parse_short(data);
        bounding_box.left = parse_short(data);
        bounding_box.right = parse_short(data);
        return bounding_box;
    };

    void parse_nodes(const std::vector<char> &data) {
        Map &current_map = wad.maps.back();
        Lump &current_lump = wad.lumps.back();
        wad_pointer = current_lump.filepos;
        while (wad_pointer < (current_lump.filepos + current_lump.size)) {
            current_map.nodes.push_back({});
            Node current_node = current_map.nodes.back();
            current_node.partition_x = parse_short(data); 
            current_node.partition_y = parse_short(data);
            current_node.partition_x_diff = parse_short(data);
            current_node.partition_y_diff = parse_short(data);
            current_node.right_box = parse_bounding_box(data, current_node);
            current_node.left_box = parse_bounding_box(data, current_node);
            current_node.right_child = parse_short(data);
            current_node.left_child = parse_short(data);
         }
    };

    void parse_sectors(const std::vector<char> &data) {
        Map &current_map = wad.maps.back();
        Lump &current_lump = wad.lumps.back();
        wad_pointer = current_lump.filepos;
        while (wad_pointer < (current_lump.filepos + current_lump.size)) {
            current_map.sectors.push_back({});
            Sector current_sector = current_map.sectors.back();
            current_sector.floor_height = parse_short(data);
            current_sector.ceiling_height = parse_short(data);
            current_sector.floor_texture = parse_string(data);
            current_sector.ceiling_texture = parse_string(data);
            current_sector.light_level = parse_short(data);
            current_sector.type = parse_short(data);
            current_sector.tag_number = parse_short(data);
        }
    };

    bool string_subset_equals(std::string x, std::string y) {
        int length = std::min(x.size(), y.size());
        bool equals = true;
        for (int i = 0 ; i < length ; i++) {
            equals &= x.at(i) == y.at(i);
        }
        return equals;
    }

    void parse_directory(const std::vector<char> &data) {
        wad.num_lumps = parse_int(data);
        wad.directory_offset = parse_int(data);
        for (int i = 0; i < wad.num_lumps; i++) {
            // Create an empty lump object
            wad.lumps.push_back({});
            Lump &lump = wad.lumps.back();

            wad_pointer = wad.directory_offset + i *16;
            lump.filepos = parse_int(data);
            lump.size = parse_int(data);
            lump.name = std::string(data.begin() + wad_pointer, data.begin() + wad_pointer + 8);

            if ((lump.name[0] == 'E') && (lump.name[2] == 'M')) {
                wad.maps.push_back({});
            }

            if (string_subset_equals(lump.name, "THINGS")) {
                parse_things(data);
            }

            if (string_subset_equals(lump.name, "LINEDEFS")) {
                parse_linedefs(data);
            }

            if (string_subset_equals(lump.name, "SIDEDEFS")) {
                parse_sidedefs(data);
            }

            if (string_subset_equals(lump.name, "VERTEXES")) {
                parse_vertices(data);
            }

            if (string_subset_equals(lump.name, "SEGS")) {
                parse_segs(data);
            }

            if (string_subset_equals(lump.name, "SSECTORS")) {
                parse_ssectors(data);
            }

            if (string_subset_equals(lump.name, "NODES")) {
                parse_nodes(data);
            }

            if (string_subset_equals(lump.name, "SECTORS")) {
                parse_sectors(data);
            }

        }
    }

    void parse_wad_type(const std::vector<char> &data) {
        std::string wad_type_string;
        for (int i = 0; i < 4; i++) {
            wad_type_string.push_back(data.at(wad_pointer));
            wad_pointer++;
        }
        if (wad_type_string == "IWAD") {
            wad.wad_type = WadType::IWAD;
        }
        else if (wad_type_string == "PWAD") {
            wad.wad_type = WadType::PWAD;
        }
        else {
            throw std::runtime_error("Couldn't parse wad");
        }
    }

    short parse_short(const std::vector<char> &data) {
        char buffer[2];
        for (int i = 0; i < 2; i++) {
            buffer[i] = data.at(wad_pointer);
            wad_pointer ++;
        }

        return * (short *) &buffer;
    }

    int parse_int(const std::vector<char> &data) {
        char buffer[4];
        for (int i = 0; i < 4; i++) {
            buffer[i] = data.at(wad_pointer);
            wad_pointer ++;
        }
        return * (int *) &buffer;
    }

    void print_vector_slice(const std::vector<char> &data, int start, int end) {
        std::string slice;
        for (int i = start; i < end; i++) {
            slice.push_back(data.at(i));
        }
        std::cout << slice << std::endl;
    }

    void print_lumps() {
        for (Lump lump : wad.lumps) {
            std::cout << lump.name << std::endl;
        }
    }

    void parse_header(const std::vector<char> &data) {
        parse_wad_type(data);
        parse_directory(data);
    }

    void load_wad(const char * path) {
        std::vector<char> data = read_file(path);
        parse_header(data);
        throw std::runtime_error("cool beanz");
    };

    struct UniformBufferObject {
        alignas(16) glm::mat4x4 model;
        alignas(16) glm::mat4x4 view;
        alignas(16) glm::mat4x4 proj;
    };

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;
        static VkVertexInputBindingDescription get_binding_description() {
            VkVertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(Vertex);
            binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return binding_description;
        }
        static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};
            attribute_descriptions[0].binding = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attribute_descriptions[0].offset = offsetof(Vertex, pos);
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attribute_descriptions[1].offset = offsetof(Vertex, color);
            return attribute_descriptions;
        }
    };
    VkPhysicalDeviceMemoryProperties mem_properties;
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };    
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;
    std::vector<VkBuffer> uniform_buffers;
    std::vector<VkDeviceMemory> uniform_buffers_memory;
    std::vector<void *> uniform_buffers_mapped;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    uint32_t image_count;
    uint32_t current_frame = 0;
    VkRenderPassCreateInfo render_pass_info {};
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
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;
    GLFWwindow* window;
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

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
        bool is_complete()
        {
            return (graphics_family.has_value() && present_family.has_value());
        }
    };

    static std::vector<char> read_file(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
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
        const std::vector<VkSurfaceFormatKHR>& available_formats)
    {
        // Selects the best format
        for (const auto& available_format : available_formats) {
            if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB
                && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return available_format;
            }
        }

        return available_formats.at(0);
    }

    VkPresentModeKHR choose_swap_present_mode(
        const std::vector<VkPresentModeKHR>& available_present_modes)
    {
        for (const auto& available_present_mode : available_present_modes) {
            if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return available_present_mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width
            != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actual_extent
            = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

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
        if (format_count != 0) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                details.formats.data());
        }
        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
            &present_mode_count, nullptr);
        if (present_mode_count != 0) {
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
        for (int i = 0; i < queue_family_count; i++) {
            VkQueueFamilyProperties queue_family = queue_families.at(i);
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics_family = i;
            }

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                &present_support);
            if (present_support) {
                indices.present_family = i;
            }

            if (indices.is_complete()) {
                break;
            }
        }
        return indices;
    }

    void populate_debug_messenger_create_info(
        VkDebugUtilsMessengerCreateInfoEXT& create_info)
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
        const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
        const VkAllocationCallbacks* p_allocator,
        VkDebugUtilsMessengerEXT* p_debug_messenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");

        if (func != nullptr) {
            return func(instance, p_create_info, p_allocator, p_debug_messenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void
    destroy_debug_utils_messenger_ext(VkInstance instance,
        VkDebugUtilsMessengerEXT debug_messenger,
        const VkAllocationCallbacks* p_allocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debug_messenger, p_allocator);
        }
    }

    void setup_debug_messenger()
    {
        if (!enable_validation_layers) {
            return;
        }
        VkDebugUtilsMessengerCreateInfoEXT create_info {};
        populate_debug_messenger_create_info(create_info);
        if (create_debug_utils_messenger_ext(instance, &create_info, nullptr,
                &debug_messenger)
            != VK_SUCCESS) {
            throw std::runtime_error("Failed to set up debug messenger.");
        }
    }

    bool check_validation_layer_support()
    {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
        for (const char* layer_name : validation_layers) {
            // std::cout << layer_name << std::endl;
            bool layer_found = false;
            for (const auto& layer_properties : available_layers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found) {
                return false;
            }
        }
        return true;
    }

    void init_window()
    {
        if (!glfwInit()) {
            throw std::runtime_error("GLFW Initialisation failed.");
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "jubaengine", nullptr, nullptr);
    }

    void create_surface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface)
            != VK_SUCCESS) {
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
        create_descriptor_set_layout();
        create_graphics_pipeline();
        create_framebuffers();
        create_command_pool();
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
        create_vertex_buffer();
        create_index_buffer();
        create_uniform_buffers();
        create_descriptor_pool();
        create_descriptor_sets();
        create_command_buffers();
        create_sync_objects();
    }
    
    void create_descriptor_pool() {
        VkDescriptorPoolSize pool_size{};
        pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_size.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = &pool_size;
        pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        if (vkCreateDescriptorPool(device,&pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool.");
        }
    }

    void create_descriptor_sets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = descriptor_pool;
        alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        alloc_info.pSetLayouts = layouts.data();
        descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor sets.");
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo buffer_info{};
            buffer_info.buffer = uniform_buffers.at(i);
            buffer_info.offset = 0;
            buffer_info.range = sizeof(UniformBufferObject);
            VkWriteDescriptorSet descriptor_write{};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = descriptor_sets.at(i);
            descriptor_write.dstBinding = 0;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pBufferInfo = &buffer_info;
            descriptor_write.pImageInfo = nullptr;
            descriptor_write.pTexelBufferView = nullptr;
            vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
        }
    }

    void create_uniform_buffers() {
        VkDeviceSize buffer_size = sizeof(UniformBufferObject);
        uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
        uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffers.at(i), uniform_buffers_memory.at(i));
            vkMapMemory(device, uniform_buffers_memory.at(i), 0, buffer_size, 0, &uniform_buffers_mapped.at(i));
        }

    }

    void create_descriptor_set_layout() {
        VkDescriptorSetLayoutBinding ubo_layout_binding{};
        ubo_layout_binding.binding = 0;
        ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_layout_binding.descriptorCount = 1;
        ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        ubo_layout_binding.pImmutableSamplers = nullptr;
        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = 1;
        layout_info.pBindings = &ubo_layout_binding;
        if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create UBO descriptor set layout");
        }
    }

    void create_index_buffer() {
        VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
        void *data;
        vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
        memcpy(data, indices.data(), (size_t) buffer_size);
        vkUnmapMemory(device, staging_buffer_memory);
        create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_memory);
        copy_buffer(staging_buffer, index_buffer, buffer_size);
        vkDestroyBuffer(device, staging_buffer, nullptr);
        vkFreeMemory(device, staging_buffer_memory, nullptr);
    }

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory) {
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = sizeof(vertices[0]) * vertices.size();
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vertex buffer.");
        }
        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);
        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate vertex buffer memory.");
        }
        vkBindBufferMemory(device, buffer, buffer_memory, 0);
    }

    void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool = command_pool;
        alloc_info.commandBufferCount = 1;
        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(command_buffer, &begin_info);
        VkBufferCopy copy_region{};
        copy_region.srcOffset = 0;
        copy_region.dstOffset = 0;
        copy_region.size = size;
        vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
        vkEndCommandBuffer(command_buffer);
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphics_queue);
        vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    }

    void create_vertex_buffer() {
        VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;
        create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
        void* data;
        vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
        memcpy(data, vertices.data(), (size_t) buffer_size);
        vkUnmapMemory(device, staging_buffer_memory);
        create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory);
        copy_buffer(staging_buffer, vertex_buffer,buffer_size);
        vkDestroyBuffer(device, staging_buffer, nullptr);
        vkFreeMemory(device, staging_buffer_memory, nullptr);
    }

    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties){
                return i;
            }
        }
        return UINT32_MAX;
    }

    void create_sync_objects()
    {
        image_available_semphs.resize(image_count);
        render_finished_semphs.resize(image_count);
        in_flight_fences.resize(image_count);
        VkSemaphoreCreateInfo semaphore_info {};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fence_info {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (int i = 0; i < image_count; i++) {
            if (vkCreateSemaphore(device, &semaphore_info, nullptr,
                    &image_available_semphs.at(i))
                != VK_SUCCESS) {
                throw std::runtime_error(
                    "Failed to create the image_available_semph");
            }

            if (vkCreateSemaphore(device, &semaphore_info, nullptr,
                    &render_finished_semphs.at(i))
                != VK_SUCCESS) {
                throw std::runtime_error("Failed to create render_finished_semph");
            }

            if (vkCreateFence(device, &fence_info, nullptr, &in_flight_fences.at(i))
                != VK_SUCCESS) {
                throw std::runtime_error("Failed to create in flight fence.");
            }
        }
    }

    void
    record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index)
    {
        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;
        if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer.");
        }
        VkRenderPassBeginInfo render_pass_info {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass;
        render_pass_info.framebuffer = swapchain_framebuffers[image_index];
        render_pass_info.renderArea.offset = { 0, 0 };
        render_pass_info.renderArea.extent = swapchain_extent;
        VkClearValue clear_color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clear_color;
        vkCmdBeginRenderPass(command_buffer, &render_pass_info,
            VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphics_pipeline);
        VkBuffer vertex_buffers[] = {vertex_buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);
        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchain_extent.width);
        viewport.height = static_cast<float>(swapchain_extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        VkRect2D scissor {};
        scissor.offset = { 0, 0 };
        scissor.extent = swapchain_extent;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets.at(current_frame), 0, nullptr);
        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        // vkCmdDraw(command_buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        vkCmdEndRenderPass(command_buffer);
        if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer.");
        }
    }

    void acquire_next_image();
    void draw();
    void present();

    void update_uniform_buffer(uint32_t current_frame) {
        static auto start_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapchain_extent.width / (float) swapchain_extent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;
        memcpy(uniform_buffers_mapped.at(current_frame), &ubo, sizeof(ubo));
    }

    void draw_frame()
    {
        update_uniform_buffer(current_frame);
        vkWaitForFences(device, 1, &in_flight_fences.at(current_frame), VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &in_flight_fences.at(current_frame));
        uint32_t image_index;
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
            image_available_semphs.at(current_frame),
            VK_NULL_HANDLE, &image_index);
        vkResetCommandBuffer(command_buffers.at(current_frame), 0);
        record_command_buffer(command_buffers.at(current_frame), image_index);

        VkPipelineStageFlags wait_stages[]
            = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &image_available_semphs.at(current_frame);
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffers.at(current_frame);
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &render_finished_semphs.at(image_index);
        if (vkQueueSubmit(graphics_queue, 1, &submit_info,
                in_flight_fences.at(current_frame))
            != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer");
        }
        VkSubpassDependency dependency {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;
        VkPresentInfoKHR present_info {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &render_finished_semphs.at(image_index);
        VkSwapchainKHR swapchains[] = { swapchain };
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &image_index;
        present_info.pResults = nullptr;
        vkQueuePresentKHR(present_queue, &present_info);
        current_frame++;
        current_frame *= (current_frame < image_count);
    }

    void create_command_buffers()
    {
        VkCommandBufferAllocateInfo alloc_info {};
        command_buffers.resize(image_count);
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = (uint32_t)command_buffers.size();
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data())
            != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command buffers.");
        }
    }

    void create_command_pool()
    {
        QueueFamilyIndices queue_family_indices
            = find_queue_families(physical_device);
        VkCommandPoolCreateInfo pool_info {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
        if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool)
            != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a command pool.");
        }
    }

    void create_framebuffers()
    {
        swapchain_framebuffers.resize(swapchain_image_views.size());

        for (size_t i = 0; i < swapchain_image_views.size(); i++) {
            VkImageView attachments[] = { swapchain_image_views[i] };
            VkFramebufferCreateInfo framebuffer_info {};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = render_pass;
            framebuffer_info.attachmentCount = 1;
            framebuffer_info.pAttachments = attachments;
            framebuffer_info.width = swapchain_extent.width;
            framebuffer_info.height = swapchain_extent.height;
            framebuffer_info.layers = 1;
            if (vkCreateFramebuffer(device, &framebuffer_info, nullptr,
                    &swapchain_framebuffers[i])
                != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer");
            }
        }
    }

    void create_render_pass()
    {
        VkAttachmentDescription color_attachment {};
        color_attachment.format = swapchain_image_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_ref {};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;

        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments = &color_attachment;
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;

        if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass)
            != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass.");
        }
    }

    VkShaderModule create_shader_module(const std::vector<char>& src)
    {
        VkShaderModuleCreateInfo create_info {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = src.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(src.data());

        VkShaderModule shader_module {};
        if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module)) {
            throw std::runtime_error("Failed to create shader module.");
        }
        return shader_module;
    }

    void create_graphics_pipeline()
    {
        // TODO: Seperate source shaders from compiled shaders.
        auto vert_src = read_file("shaders/vert.spv");
        auto frag_src = read_file("shaders/frag.spv");

        vert_module = create_shader_module(vert_src);
        frag_module = create_shader_module(frag_src);

        VkPipelineShaderStageCreateInfo vert_create_info {};
        vert_create_info.sType
            = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_create_info.module = vert_module;
        vert_create_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_create_info {};
        frag_create_info.sType
            = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_create_info.module = frag_module;
        frag_create_info.pName = "main";

        VkPipelineShaderStageCreateInfo shader_stages[]
            = { vert_create_info, frag_create_info };

        VkPipelineVertexInputStateCreateInfo vert_input_info {};
        auto binding_description = Vertex::get_binding_description();
        auto attribute_descriptions = Vertex::get_attribute_descriptions();
        vert_input_info.sType
            = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vert_input_info.vertexBindingDescriptionCount = 1;
        vert_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
        vert_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();
        vert_input_info.pVertexBindingDescriptions = &binding_description;

        VkPipelineInputAssemblyStateCreateInfo input_assm_info {};
        input_assm_info.sType
            = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assm_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assm_info.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchain_extent.width;
        viewport.height = (float)swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor {};
        scissor.offset = { 0, 0 };
        scissor.extent = swapchain_extent;

        std::vector<VkDynamicState> dynamic_states
            = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamic_state_ci {};
        dynamic_state_ci.sType
            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_ci.dynamicStateCount
            = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state_ci.pDynamicStates = dynamic_states.data();

        VkPipelineViewportStateCreateInfo viewport_state_ci {};
        viewport_state_ci.sType
            = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_ci.viewportCount = 1;
        viewport_state_ci.scissorCount = 1;
        viewport_state_ci.pViewports = &viewport;
        viewport_state_ci.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer {};
        rasterizer.sType
            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling {};
        multisampling.sType
            = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState color_blend_attach {};
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

        VkPipelineColorBlendStateCreateInfo color_blending {};
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

        VkPipelineLayoutCreateInfo pipeline_layout_ci {};
        pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_ci.setLayoutCount = 1;
        pipeline_layout_ci.pSetLayouts = &descriptor_set_layout;
        pipeline_layout_ci.pushConstantRangeCount = 0;
        pipeline_layout_ci.pPushConstantRanges = nullptr;
        if (vkCreatePipelineLayout(device, &pipeline_layout_ci, nullptr,
                &pipeline_layout)
            != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a pipeline layout.");
        }

        VkGraphicsPipelineCreateInfo graphics_pipeline_info {};
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

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                &graphics_pipeline_info, nullptr,
                &graphics_pipeline)
            != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline.");
        }
    }

    void create_image_views()
    {
        swapchain_image_views.resize(swapchain_images.size());
        for (int i = 0; i < swapchain_images.size(); i++) {
            VkImageViewCreateInfo create_info {};
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

            if (vkCreateImageView(device, &create_info, nullptr,
                    &swapchain_image_views.at(i))
                != VK_SUCCESS) {

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
        image_count = swapchain_support.capabilities.minImageCount + 1;
        if (swapchain_support.capabilities.maxImageCount > 0) {
            image_count = std::min(image_count,
                swapchain_support.capabilities.maxImageCount);
        }
        VkSwapchainCreateInfoKHR create_info {};
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
            = { indices.graphics_family.value(), indices.present_family.value() };
        if (indices.graphics_family.value() != indices.present_family.value()) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_family_indices;
        } else {
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

    std::vector<const char*> get_required_extensions()
    {
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        std::vector<const char*> extensions(
            glfw_extensions, glfw_extensions + glfw_extension_count);
        if (enable_validation_layers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
        void* p_user_data)
    {
        if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "Validation layer: " << p_callback_data->pMessage
                      << std::endl;
        }
        return VK_FALSE;
    }

    void create_instance()
    {
        if (enable_validation_layers && !check_validation_layer_support()) {
            throw std::runtime_error(
                "Validation layers requested, but not available.");
        }
        // A VKInstance is the bridge between our application and the
        // Vulkan driver.
        // To create it, we need to specify some information in a
        // VkInstanceCreateInfo struct.
        VkInstanceCreateInfo create_info {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        // When creating an instance, providing some extra information
        // about our application is useful, so that the Vulkan driver
        // can perform some use-specific optimizations.
        // We provide this information through VkApplicationInfo.
        // #ref:
        // https://registry.khronos.org/vulkan/specs/latest/man/html/VkApplicationInfo.html
        VkApplicationInfo app_info {};
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
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        auto extensions = get_required_extensions();
        create_info.enabledExtensionCount
            = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
        // Global validation layer
        // Works in a similar way to extensions.
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info {};
        if (enable_validation_layers) {
            create_info.enabledLayerCount
                = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
            populate_debug_messenger_create_info(debug_create_info);
            create_info.pNext
                = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
        } else {
            create_info.enabledLayerCount = 0;
            create_info.pNext = nullptr;
        }
        if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a VkInstance");
        }
    }

    void main_loop()
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            
            auto time_1 = std::chrono::high_resolution_clock::now();
            draw_frame();
            auto time_2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> frame_time = time_2 - time_1;
            float fps = 1.0f / frame_time.count();
            std::stringstream ss;
            ss << "fps: " << fps << std::endl;
            glfwSetWindowTitle(window, ss.str().c_str());
        }
    } 
    void cleanup()
    {
        for (size_t i = 0; i < image_count; i++) {
            vkDestroyBuffer(device, uniform_buffers.at(i), nullptr);
            vkFreeMemory(device, uniform_buffers_memory.at(i), nullptr);
        }
        vkDeviceWaitIdle(device);
        vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
        vkDestroyBuffer(device, index_buffer, nullptr);
        vkFreeMemory(device, index_buffer_memory, nullptr);
        vkDestroyBuffer(device, vertex_buffer, nullptr);
        vkFreeMemory(device, vertex_buffer_memory, nullptr);
        for (int i = 0; i < image_count; i++) {
            vkDestroySemaphore(device, image_available_semphs.at(i), nullptr);
            vkDestroySemaphore(device, render_finished_semphs.at(i), nullptr);
            vkDestroyFence(device, in_flight_fences.at(i), nullptr);
        }
        vkDestroyCommandPool(device, command_pool, nullptr);
        for (auto framebuffer : swapchain_framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        vkDestroyPipeline(device, graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyRenderPass(device, render_pass, nullptr);

        for (auto image_view : swapchain_image_views) {
            vkDestroyImageView(device, image_view, nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);
        destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyShaderModule(device, vert_module, nullptr);
        vkDestroyShaderModule(device, frag_module, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
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
            = { indices.graphics_family.value(), indices.present_family.value() };

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

        float queue_priority = 1.0f;
        for (uint32_t queue_family : unique_queue_families) {
            VkDeviceQueueCreateInfo queue_create_info {};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = indices.graphics_family.value();
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features {};
        VkDeviceCreateInfo create_info {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.queueCreateInfoCount
            = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledExtensionCount
            = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();
        if (vkCreateDevice(physical_device, &create_info, nullptr, &device)
            != VK_SUCCESS) {
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
        if (device_count == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
        for (const auto& device : devices) {
            if (is_device_suitable(device)) {
                std::cout << "Found suitable device." << std::endl;
                physical_device = device;
                break;
            }
        }

        if (physical_device == VK_NULL_HANDLE) {
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
        for (const auto& available_extension : available_extensions) {
            required_extensions.erase(available_extension.extensionName);
        }
        return required_extensions.empty();
    }

    bool is_device_suitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = find_queue_families(device);
        bool extensions_supported = check_device_extension_support(device);
        bool swapchain_adequate = false;
        if (extensions_supported) {
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
    try {
        Application app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
