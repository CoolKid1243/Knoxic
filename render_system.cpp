#include "render_system.hpp"
#include "knoxic_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <stdexcept>

namespace knoxic {

    struct SimplePushConstantData {
        glm::mat4 transform{1.f};
        alignas(16) glm::vec3 color;
    };

    RenderSystem::RenderSystem(KnoxicDevice &device, VkRenderPass renderPass) : knoxicDevice{device} {
        createPipelineLayout();
        createPipeline(renderPass);
    }

    RenderSystem::~RenderSystem() {
        vkDestroyPipelineLayout(knoxicDevice.device(), pipelineLayout, nullptr);
    }

    void RenderSystem::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(knoxicDevice.device(), &pipelineLayoutInfo, nullptr, 
        &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void RenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        KnoxicPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        knoxicPipeline = std::make_unique<KnoxicPipeline>(
            knoxicDevice,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig
        );
    }

    void RenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<KnoxicGameObject> &gameObjects) {
        knoxicPipeline->bind(commandBuffer);

        for (auto &obj: gameObjects) {
            obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.01f, glm::two_pi<float>()); // rotate the model evey frame
            obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.005f, glm::two_pi<float>()); // rotate the model evey frame

            SimplePushConstantData push{};
            push.color = obj.color;
            push.transform = obj.transform.mat4();

            vkCmdPushConstants(
                commandBuffer,
                pipelineLayout, 
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                0, 
                sizeof(SimplePushConstantData), 
                &push
            );
            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }
    }
}