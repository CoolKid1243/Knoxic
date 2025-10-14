#include "knoxic_vk_directional_light_system.hpp"
#include "../../core/vulkan/knoxic_vk_device.hpp"
#include "../../graphics/knoxic_frame_info.hpp"
#include "../../core/ecs/components.hpp"
#include "../../core/ecs/coordinator_instance.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <stdexcept>
#include <map>

namespace knoxic {

    struct DirectionalLightPushConstants {
        glm::vec4 position{}; // position for visualization
        glm::vec4 direction{}; // ignore w
        glm::vec4 color{}; // w is intensity
        float radius;
    };

    DirectionalLightSystem::DirectionalLightSystem(
        KnoxicDevice &device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout,
        std::shared_ptr<DirectionalLightECSSystem> ecsDirectionalLightSystem
    ) : knoxicDevice{device}, directionalLightSystem{std::move(ecsDirectionalLightSystem)} {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    DirectionalLightSystem::~DirectionalLightSystem() {
        vkDestroyPipelineLayout(knoxicDevice.device(), pipelineLayout, nullptr);
    }

    void DirectionalLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(DirectionalLightPushConstants);

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

    void DirectionalLightSystem::createPipeline(VkRenderPass renderPass) {
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
            "shaders/vk_directional_light.vert.spv",
            "shaders/vk_directional_light.frag.spv",
            pipelineConfig
        );
    }

    void DirectionalLightSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo) {
        int lightIndex = 0;
        for (auto entity : directionalLightSystem->mEntities) {
            auto &transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto &light = gCoordinator.GetComponent<DirectionalLightComponent>(entity);

            assert(lightIndex < MAX_LIGHTS && "Directional lights exceed maximum specified limit");

            glm::vec3 color = glm::vec3(1.0f);
            if (gCoordinator.HasComponent<ColorComponent>(entity)) {
                color = gCoordinator.GetComponent<ColorComponent>(entity).color;
            }

            // Calculate direction from rotation (forward is -Z in our coordinate system)
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            rotationMatrix = glm::rotate(rotationMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            rotationMatrix = glm::rotate(rotationMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::vec3 direction = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

            ubo.directionalLights[lightIndex].direction = glm::vec4(direction, 1.0f);
            ubo.directionalLights[lightIndex].color = glm::vec4(color, light.lightIntensity);
            lightIndex += 1;
        }

        ubo.numDirectionalLights = lightIndex;
    }

    void DirectionalLightSystem::render(FrameInfo &frameInfo) {
        // Sort lights by distance to camera (furthest first)
        std::map<float, Entity> sorted;
        for (auto entity : directionalLightSystem->mEntities) {
            auto &transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto offset = frameInfo.camera.getPosition() - transform.translation;
            float disSquared = glm::dot(offset, offset);
            sorted[disSquared] = entity;
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

        // Iterate through the lights in reverse order
        for (auto it = sorted.rbegin(); it != sorted.rend(); it++) {
            Entity entity = it->second;
            auto &transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto &light = gCoordinator.GetComponent<DirectionalLightComponent>(entity);

            glm::vec3 color = glm::vec3(1.0f);
            if (gCoordinator.HasComponent<ColorComponent>(entity)) {
                color = gCoordinator.GetComponent<ColorComponent>(entity).color;
            }

            // Calculate direction from rotation
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            rotationMatrix = glm::rotate(rotationMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            rotationMatrix = glm::rotate(rotationMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::vec3 direction = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

            DirectionalLightPushConstants push{};
            push.position = glm::vec4(transform.translation, 1.0f);
            push.direction = glm::vec4(direction, 1.0f);
            push.color = glm::vec4(color, light.lightIntensity);
            push.radius = transform.scale.x;

            vkCmdPushConstants(
                frameInfo.commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(DirectionalLightPushConstants),
                &push
            );

            vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
        }
    }
}