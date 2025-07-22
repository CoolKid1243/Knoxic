#pragma once

#include "knoxic_device.hpp"

#include <string>
#include <vector>

namespace knoxic {

    struct PipelineConfigInfo {
        VkViewport viewport;
        VkRect2D scissor;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };

    class KnoxicPipeline {
        public:
            KnoxicPipeline(
                KnoxicDevice &device, 
                const std::string &vertexFilePath, 
                const std::string &fragmentFilePath, 
                const PipelineConfigInfo &configInfo
            );
            ~KnoxicPipeline();

            KnoxicPipeline(const KnoxicPipeline&) = delete;
            KnoxicPipeline &operator=(const KnoxicPipeline&) = delete;

            void bind(VkCommandBuffer commandBuffer);
            
            static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);

        private:
            static std::vector<char> readFile(const std::string &filepath);

            void createGraphicsPipeline(
                const std::string &vertexFilePath, 
                const std::string &fragmentFilePath, 
                const PipelineConfigInfo &configInfo
            );

            void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);

            KnoxicDevice &knoxicDevice;
            VkPipeline graphicsPipeline;
            VkShaderModule vertShaderModule;
            VkShaderModule fragShaderModule;
    };
}