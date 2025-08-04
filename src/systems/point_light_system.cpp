#include "point_light_system.hpp"
#include "../core/knoxic_device.hpp"
#include "../graphics/knoxic_frame_info.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <stdexcept>
#include <map>

namespace knoxic {

    struct PointLightPushConstants {
        glm::vec4 position{}; // ignore w
        glm::vec4 color{}; // w is intensity
        float radius;
    };

    PointLightSystem::PointLightSystem(KnoxicDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : knoxicDevice{device} {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    PointLightSystem::~PointLightSystem() {
        vkDestroyPipelineLayout(knoxicDevice.device(), pipelineLayout, nullptr);
    }

    void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PointLightPushConstants);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(knoxicDevice.device(), &pipelineLayoutInfo, nullptr, 
        &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void PointLightSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        KnoxicPipeline::defaultPipelineConfigInfo(pipelineConfig);
        KnoxicPipeline::enableAlphaBlending(pipelineConfig);
        pipelineConfig.attributeDescriptions.clear();
        pipelineConfig.bindingDescriptions.clear();
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        knoxicPipeline = std::make_unique<KnoxicPipeline>(
            knoxicDevice,
            "shaders/point_light.vert.spv",
            "shaders/point_light.frag.spv",
            pipelineConfig
        );
    }

    void PointLightSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo) {
        auto rotateLight = glm::rotate(glm::mat4(1.0f), frameInfo.frameTime, {0.0f, -1.0f, 0.0f});

        int lightIndex = 0;
        for (auto &keyValue : frameInfo.gameObjects) {
            auto &obj = keyValue.second;
            if (obj.pointLight == nullptr) continue;

            assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified limit");

            // Update light position
            obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.0f));

            // Copy light to ubo
            ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.0f);
            ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);

            lightIndex += 1;
        }

        ubo.numLights = lightIndex;
    }

    void PointLightSystem::render(FrameInfo &frameInfo) {
        // Sort lights
        std::map<float, KnoxicGameObject::id_t> sorted;
        for (auto &keyValue : frameInfo.gameObjects) {
            auto &obj = keyValue.second;
            if (obj.pointLight == nullptr) continue;

            // Calculate distance
            auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
            float disSquared = glm::dot(offset, offset);
            sorted[disSquared] = obj.getId();
        }

        knoxicPipeline->bind(frameInfo.commandBuffer);

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 1,
            &frameInfo.globalDescriptorSet,
            0, nullptr
        );

        // Itterate through the lights in reverse order
        for (auto it = sorted.rbegin(); it != sorted.rend(); it++) {
            // Use game obj id to find light object
            auto &obj = frameInfo.gameObjects.at(it->second);

            PointLightPushConstants push{};
            push.position = glm::vec4(obj.transform.translation, 1.0f);
            push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
            push.radius = obj.transform.scale.x;

            vkCmdPushConstants(
                frameInfo.commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PointLightPushConstants),
                &push
            );

            vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
        }

    }
}