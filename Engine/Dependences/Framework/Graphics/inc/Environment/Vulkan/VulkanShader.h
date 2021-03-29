//
// Created by Nikita on 29.03.2021.
//

#ifndef GAMEENGINE_VULKANSHADERPROGRAM_H
#define GAMEENGINE_VULKANSHADERPROGRAM_H

#include <vulkan/vulkan.h>
#include <Environment/IShaderProgram.h>

namespace Framework::Graphics {
    struct VulkanShader : public IShaderProgram {
        VkShaderModule m_vertShaderModule = VK_NULL_HANDLE;
        VkShaderModule m_fragShaderModule = VK_NULL_HANDLE;

        [[nodiscard]] bool IsReady() const noexcept override {
            return m_vertShaderModule != VK_NULL_HANDLE && m_fragShaderModule != VK_NULL_HANDLE;
        }
    };
}

#endif //GAMEENGINE_VULKANSHADERPROGRAM_H
