#include "knoxic_vk_point_light_system.hpp"
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

    struct PointLightPushConstants {
        glm::vec4 position{}; // ignore w
        glm::vec4 color{}; // w is intensity
        float radius;
    };

    PointLightSystem::PointLightSystem(
        KnoxicDevice &device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout,
        std::shared_ptr<PointLightECSSystem> ecsPointLightSystem
    ) : knoxicDevice{device}, pointLightSystem{std::move(ecsPointLightSystem)} {
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
            "shaders/vk_point_light.vert.spv",
            "shaders/vk_point_light.frag.spv",
            pipelineConfig
        );
    }

    void PointLightSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo) {
        static float time = 0.0f;
        time += frameInfo.frameTime;

        int lightIndex = 0;
        for (auto entity : pointLightSystem->mEntities) {
            auto &transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto &light = gCoordinator.GetComponent<PointLightComponent>(entity);

            assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified limit");

            // Animate the point lights
            if (transform.translation.x > -3.0f && transform.translation.x < 3.0f) {
                auto rotateLight = glm::rotate(
                    glm::mat4(1.0f),
                    frameInfo.frameTime * 0.6f,
                    glm::vec3{0.0f, -1.0f, 0.0f}
                );
                transform.translation = glm::vec3(rotateLight * glm::vec4(transform.translation, 1.0f));
            } else if (transform.translation.x > 7.0f && transform.translation.x < 13.0f) {
                transform.translation.x = 10.0f + sin(time * 1.0f) * 2.0f;
            } else if (transform.translation.x > -13.0f && transform.translation.x < -7.0f) {
                float pendulumAngle = sin(time * 0.6f) * (glm::pi<float>() / 2.0f);
                glm::vec3 helmetCenter = glm::vec3(-10.0f, 0.5f, 0.0f);
                float radius = 2.0f;
                transform.translation.x = helmetCenter.x - radius * sin(pendulumAngle);
                transform.translation.y = helmetCenter.y - 1.0f;
                transform.translation.z = helmetCenter.z - radius * cos(pendulumAngle) - 0.3f;
            }

            glm::vec3 color = glm::vec3(1.0f);
            if (gCoordinator.HasComponent<ColorComponent>(entity)) {
                color = gCoordinator.GetComponent<ColorComponent>(entity).color;
            }

            ubo.pointLights[lightIndex].position = glm::vec4(transform.translation, 1.0f);
            ubo.pointLights[lightIndex].color = glm::vec4(color, light.lightIntensity);
            lightIndex += 1;
        }

        ubo.numLights = lightIndex;
    }

    void PointLightSystem::render(FrameInfo &frameInfo) {
        // Sort lights by distance to camera (furthest first)
        std::map<float, Entity> sorted;
        for (auto entity : pointLightSystem->mEntities) {
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
            auto &light = gCoordinator.GetComponent<PointLightComponent>(entity);

            glm::vec3 color = glm::vec3(1.0f);
            if (gCoordinator.HasComponent<ColorComponent>(entity)) {
                color = gCoordinator.GetComponent<ColorComponent>(entity).color;
            }

            PointLightPushConstants push{};
            push.position = glm::vec4(transform.translation, 1.0f);
            push.color = glm::vec4(color, light.lightIntensity);
            push.radius = transform.scale.x;

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