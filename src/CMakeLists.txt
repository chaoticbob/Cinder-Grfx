cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

# Determine Vulkan's include directory
if (NOT VULKAN_INCLUDE_DIR)
    if (LINUX)
        if (GGP)
            set(VULKAN_INCLUDE_DIR "${GGP_SYSROOT_PATH}/usr/local/include")
        else()
            if (DEFINED ENV{VULKAN_SDK})
                set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/include")
            endif()
        endif()
    elseif (APPLE OR WIN32)
        if (DEFINED ENV{VULKAN_SDK})
            set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")
        endif()
    endif()
endif()

# Determine Vulkan's library directory
if (NOT VULKAN_LIBRARY_DIR)
    if (LINUX)
        if (GGP)
            set(VULKAN_LIBRARY_DIR "${GGP_SYSROOT_PATH}/usr/local/lib")
        else()
            if (DEFINED ENV{VULKAN_SDK})
                set(VULKAN_LIBRARY_DIR "$ENV{VULKAN_SDK}/lib")
            endif()
        endif()
    elseif (APPLE OR WIN32)
        if (DEFINED ENV{VULKAN_SDK})
            set(VULKAN_LIBRARY_DIR "$ENV{VULKAN_SDK}/Lib")
        endif()
    endif()
endif()

# Bail if Vulkan's include directory is not set
if (NOT VULKAN_INCLUDE_DIR)
    message(FATAL_ERROR "VULKAN_INCLUDE_DIR not specified and could not be determined using environment variable VULKAN_SDK")
endif()

# Bail if Vulkan's library directory is not set
if (NOT VULKAN_LIBRARY_DIR)
    message(FATAL_ERROR "VULKAN_LIBRARY_DIR not specified and could not be determined using environment variable VULKAN_SDK")
endif()

message("Found Vulkan: ${VULKAN_DIR}")
message("  Vulkan include directory: ${VULKAN_INCLUDE_DIR}")
message("  Vulkan library directory: ${VULKAN_LIBRARY_DIR}")

set(TARGET_NAME "cinder-grfx-renderers")

set(INC_PATH ${CINDER_GRFX_PATH}/include)
set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND APP_HDR_FILES
    ${INC_PATH}/cinder/app/GrfxRenderer.h
    ${INC_PATH}/cinder/app/RendererDx12.h
    ${INC_PATH}/cinder/app/RendererVk.h
)

list(APPEND APP_SRC_FILES
    ${SRC_PATH}/cinder/app/GrfxRenderer.cpp
    ${SRC_PATH}/cinder/app/RendererDx12.cpp
    ${SRC_PATH}/cinder/app/RendererVk.cpp
)

list(APPEND GRFX_HDR_FILES
    ${INC_PATH}/cinder/grfx/grfx.h
    ${INC_PATH}/cinder/grfx/grfx_config.h
    ${INC_PATH}/cinder/grfx/Constants.h
    ${INC_PATH}/cinder/grfx/Enums.h
    ${INC_PATH}/cinder/grfx/HashMap.h
)

list(APPEND GRFX_SRC_FILES
)

if(WIN32)
    list(APPEND DX12_HDR_FILES
        ${INC_PATH}/cinder/dx12/dx12.h
        ${INC_PATH}/cinder/dx12/Buffer.h
    )

    list(APPEND DX12_SRC_FILES
        ${SRC_PATH}/cinder/dx12/Buffer.cpp
    )
endif()

list(APPEND VK_HDR_FILES
    ${INC_PATH}/cinder/vk/vk.h
    ${INC_PATH}/cinder/vk/vk_config.h
    ${INC_PATH}/cinder/vk/Batch.h
    ${INC_PATH}/cinder/vk/Buffer.h
    ${INC_PATH}/cinder/vk/Command.h
    ${INC_PATH}/cinder/vk/Context.h
    ${INC_PATH}/cinder/vk/Descriptor.h
    ${INC_PATH}/cinder/vk/Device.h
    ${INC_PATH}/cinder/vk/draw.h
    ${INC_PATH}/cinder/vk/ChildObject.h
    ${INC_PATH}/cinder/vk/DeviceDispatchTable.h
    ${INC_PATH}/cinder/vk/Environment.h
    ${INC_PATH}/cinder/vk/GlslProg.h
    ${INC_PATH}/cinder/vk/HlslProg.h
    ${INC_PATH}/cinder/vk/Image.h
    ${INC_PATH}/cinder/vk/InstanceDispatchTable.h
    ${INC_PATH}/cinder/vk/HashKeys.h
    ${INC_PATH}/cinder/vk/Mesh.h
    ${INC_PATH}/cinder/vk/Pipeline.h
    ${INC_PATH}/cinder/vk/Query.h
    ${INC_PATH}/cinder/vk/RenderPass.h
    ${INC_PATH}/cinder/vk/Sampler.h
    ${INC_PATH}/cinder/vk/ShaderProg.h
    ${INC_PATH}/cinder/vk/Swapchain.h
    ${INC_PATH}/cinder/vk/Sync.h
    ${INC_PATH}/cinder/vk/Texture.h
    ${INC_PATH}/cinder/vk/Util.h
    ${INC_PATH}/cinder/vk/UniformBlock.h
    ${INC_PATH}/cinder/vk/UniformBuffer.h
    ${INC_PATH}/cinder/vk/scoped.h
    ${INC_PATH}/cinder/vk/wrapper.h
    ${CINDER_GRFX_PATH}/third_party/xxHash/xxhash.h
    ${CINDER_GRFX_PATH}/third_party/SPIRV-Reflect/spirv_reflect.h
)

list(APPEND VK_SRC_FILES
    ${SRC_PATH}/cinder/vk/Batch.cpp
    ${SRC_PATH}/cinder/vk/Buffer.cpp
    ${SRC_PATH}/cinder/vk/Command.cpp
    ${SRC_PATH}/cinder/vk/Context.cpp
    ${SRC_PATH}/cinder/vk/Environment.cpp
    ${SRC_PATH}/cinder/vk/Descriptor.cpp
    ${SRC_PATH}/cinder/vk/Device.cpp
    ${SRC_PATH}/cinder/vk/ChildObject.cpp
    ${SRC_PATH}/cinder/vk/DeviceDispatchTable.cpp
    ${SRC_PATH}/cinder/vk/Image.cpp
    ${SRC_PATH}/cinder/vk/InstanceDispatchTable.cpp
    ${SRC_PATH}/cinder/vk/Pipeline.cpp
    ${SRC_PATH}/cinder/vk/Mesh.cpp
    ${SRC_PATH}/cinder/vk/Query.cpp
    ${SRC_PATH}/cinder/vk/RenderPass.cpp
    ${SRC_PATH}/cinder/vk/Sampler.cpp
    ${SRC_PATH}/cinder/vk/ShaderProg.cpp
    ${SRC_PATH}/cinder/vk/Swapchain.cpp
    ${SRC_PATH}/cinder/vk/Sync.cpp
    ${SRC_PATH}/cinder/vk/Texture.cpp
    ${SRC_PATH}/cinder/vk/Util.cpp
    ${SRC_PATH}/cinder/vk/UniformBlock.cpp
    ${SRC_PATH}/cinder/vk/UniformBuffer.cpp
    ${SRC_PATH}/cinder/vk/scoped.cpp
    ${SRC_PATH}/cinder/vk/wrapper.cpp
    ${CINDER_GRFX_PATH}/third_party/glslang/glslang/CInterface/glslang_c_interface.cpp
    ${CINDER_GRFX_PATH}/third_party/glslang/SPIRV/CInterface/spirv_c_interface.cpp
    ${CINDER_GRFX_PATH}/third_party/glslang/StandAlone/resource_limits_c.cpp
    ${CINDER_GRFX_PATH}/third_party/glslang/StandAlone/ResourceLimits.cpp
    ${CINDER_GRFX_PATH}/third_party/xxHash/xxhash.c
    ${CINDER_GRFX_PATH}/third_party/SPIRV-Reflect/spirv_reflect.c
)

set_source_files_properties(
    ${CINDER_GRFX_PATH}/third_party/xxHash/xxhash.c
    ${CINDER_GRFX_PATH}/third_party/SPIRV-Reflect/spirv_reflect.c 
    PROPERTIES LANGUAGE C
)

add_library(
    ${TARGET_NAME}
    ${APP_HDR_FILES}
    ${APP_SRC_FILES}
    ${GRFX_HDR_FILES}
    ${GRFX_SRC_FILES}
    ${VK_HDR_FILES}
    ${VK_SRC_FILES}
)

source_group("Header Files\\cinder\\app"  FILES ${APP_HDR_FILES})
source_group("Source Files\\cinder\\app"  FILES ${APP_SRC_FILES})
source_group("Header Files\\cinder\\grfx" FILES ${GRFX_HDR_FILES})
source_group("Source Files\\cinder\\grfx" FILES ${GRFX_SRC_FILES})
source_group("Header Files\\cinder\\vk"   FILES ${VK_HDR_FILES})
source_group("Source Files\\cinder\\vk"   FILES ${VK_SRC_FILES})
    
set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER "cinder-grfx" )


target_include_directories(
    ${TARGET_NAME}
    PRIVATE ${CINDER_GRFX_PATH}/third_party/xxHash
    PUBLIC  ${CINDER_PATH}/include
            ${CINDER_GRFX_PATH}/include
            ${CINDER_GRFX_PATH}/third_party/VulkanMemoryAllocator/include
            ${VULKAN_INCLUDE_DIR}
            ${CINDER_GRFX_PATH}/third_party/glslang
            ${CINDER_GRFX_PATH}/third_party/SPIRV-Reflect

)

target_link_libraries(
    ${TARGET_NAME}
    PUBLIC cinder
           glslang
           SPIRV
)

# GGP
if ( CINDER_GGP )
    # Compile defintions
    target_compile_definitions(${TARGET_NAME} 
        PUBLIC VK_USE_PLATFORM_GGP
    )
    # Link directories
    target_link_directories(${TARGET_NAME} 
        PUBLIC ${VULKAN_LIBRARY_DIR}
    )
    # Link libraries
    target_link_libraries(${TARGET_NAME} 
        PUBLIC vulkan
    ) 
#Linux
elseif ( CINDER_LINUX )
    # Compile defintions
    target_compile_definitions(${TARGET_NAME} 
        PUBLIC VK_USE_PLATFORM_XCB_KHR
    )
    # Link directories
    target_link_directories(${TARGET_NAME} 
        PUBLIC ${VULKAN_LIBRARY_DIR}
    )
    # Link libraries
    target_link_libraries(${TARGET_NAME} 
        PUBLIC vulkan
    )  

    # Link properties
    set_target_properties(${TARGET_NAME}
        PROPERTIES OUTPUT_NAME HTK
                   LINKER_LANGUAGE CXX
    )
    # Link libraries
    target_link_libraries(${TARGET_NAME} 
        PUBLIC xcb X11-xcb
    )
# Windows
elseif ( CINDER_MSW )
    # Compile defintions
    target_compile_definitions(${TARGET_NAME} 
        PUBLIC VK_USE_PLATFORM_WIN32_KHR
    )
    # Link libraries
    target_link_libraries(${TARGET_NAME} 
        PUBLIC "${VULKAN_LIBRARY_DIR}/vulkan-1.lib"
               "${VULKAN_LIBRARY_DIR}/dxcompiler.lib"
    )
endif()