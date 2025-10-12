#include "knoxic_vk_render_system.hpp"
#include "../../core/vulkan/knoxic_vk_device.hpp"
#include "../../graphics/knoxic_frame_info.hpp"
#include "../../core/ecs/components.hpp"
#include "../../core/ecs/coordinator_instance.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <stdexcept>

namespace knoxic {

    struct PushConstantData {
        glm::mat4 modelMatrix{1.0f};
        glm::mat4 normalMatrix{1.0f};
        
        glm::vec3 albedo{1.0f, 1.0f, 1.0f};
        float metallic{0.0f};
        float roughness{0.5f};
        float ao{1.0f};
        glm::vec2 textureOffset{0.0f, 0.0f};
        glm::vec2 textureScale{1.0f, 1.0f};

        glm::vec3 emissionColor; 
        float emissionStrength;
    };

    RenderSystem::RenderSystem(
        KnoxicDevice &device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout,
        VkDescriptorSetLayout materialSetLayout,
        std::shared_ptr<RenderableSystem> ecsRenderableSystem
    ) : knoxicDevice{device}, renderableSystem{std::move(ecsRenderableSystem)} {
        createPipelineLayout(globalSetLayout, materialSetLayout);
        createPipeline(renderPass);
    }

    RenderSystem::~RenderSystem() {
        vkDestroyPipelineLayout(knoxicDevice.device(), pipelineLayout, nullptr);
    }

    void RenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout materialSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout, materialSetLayout};

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

    void RenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        KnoxicPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        knoxicPipeline = std::make_unique<KnoxicPipeline>(
            knoxicDevice,
            "shaders/vk_lighting.vert.spv",
            "shaders/vk_lighting.frag.spv",
            pipelineConfig
        );
    }

    void RenderSystem::renderGameObjects(FrameInfo &frameInfo) {
        knoxicPipeline->bind(frameInfo.commandBuffer);

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 1,
            &frameInfo.globalDescriptorSet,
            0, nullptr
        );

        // Iterate over ECS renderable entities
        for (auto entity : renderableSystem->mEntities) {
            // Required components
            auto &transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto &modelComp = gCoordinator.GetComponent<ModelComponent>(entity);

            if (!modelComp.model) continue;

            PushConstantData push{};
            push.modelMatrix = transform.mat4();
            push.normalMatrix = transform.normalMatrix();

            // Optional material
            if (gCoordinator.HasComponent<MaterialComponent>(entity)) {
                auto &matComp = gCoordinator.GetComponent<MaterialComponent>(entity);
                if (matComp.material) {
                    const auto& matProps = matComp.material->getProperties();
                    push.albedo = matProps.albedo;
                    push.metallic = matProps.metallic;
                    push.roughness = matProps.roughness;
                    push.ao = matProps.ao;
                    push.textureOffset = matProps.textureOffset;
                    push.textureScale = matProps.textureScale;

                    push.emissionColor = matProps.emissionColor;
                    push.emissionStrength = matProps.emissionStrength;

                    // Bind material descriptor set
                    VkDescriptorSet materialDescriptorSet = matComp.material->getDescriptorSet();
                    vkCmdBindDescriptorSets(
                        frameInfo.commandBuffer,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelineLayout,
                        1, 1,
                        &materialDescriptorSet,
                        0, nullptr
                    );
                }
            } else {
                // Optional color fallback
                if (gCoordinator.HasComponent<ColorComponent>(entity)) {
                    push.albedo = gCoordinator.GetComponent<ColorComponent>(entity).color;
                }

                // Default emission for non-material objects
                push.emissionColor = glm::vec3(0.0f);
                push.emissionStrength = 0.0f;
            }

            vkCmdPushConstants(
                frameInfo.commandBuffer,
                pipelineLayout, 
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                0, 
                sizeof(PushConstantData), 
                &push
            );

            modelComp.model->bind(frameInfo.commandBuffer);
            modelComp.model->draw(frameInfo.commandBuffer);
        }
    }
}