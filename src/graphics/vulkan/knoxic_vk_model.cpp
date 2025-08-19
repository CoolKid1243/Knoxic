#include "knoxic_vk_model.hpp"
#include "../../core/vulkan/knoxic_vk_buffer.hpp"
#include "../../core/vulkan/knoxic_vk_device.hpp"
#include "../../core/knoxic_utils.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>

#define ENGINE_DIR "../"

namespace std {
    template <>
    struct hash<knoxic::KnoxicModel::Vertex> {
        size_t operator()(knoxic::KnoxicModel::Vertex const &vertex) const {
            size_t seed = 0;
            knoxic::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
}

namespace knoxic {

    KnoxicModel::KnoxicModel(KnoxicDevice &device, const KnoxicModel::Data &data) : knoxicDevice(device) {
        createVertexBuffers(data.vertices);
        createIndexBuffer(data.indices);
    }

    KnoxicModel::~KnoxicModel() {}

    std::unique_ptr<KnoxicModel> KnoxicModel::createModelFromFile(KnoxicDevice &device, const std::string &filePath) {
        Data data;
        data.loadModel(ENGINE_DIR + filePath);

        // Print each object and its data
        // std::cout << "\n=== Model Loaded ===\n"
        //     << "File Path    : " << filePath << "\n"
        //     << "Vertex Count : " << data.vertices.size() << "\n"
        //     << "Index Count  : " << data.indices.size() << "\n"
        //     << "====================\n";

        //std::cout << filePath << "\n"; // print the object file path

        return std::make_unique<KnoxicModel>(device, data);
    }

    void KnoxicModel::createVertexBuffers(const std::vector<Vertex> &vertices) {
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Vertex count must be at least 3");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
        uint32_t vertexSize = sizeof(vertices[0]);

        KnoxicBuffer stagingBuffer {
            knoxicDevice,
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void *) vertices.data());

        vertexBuffer = std::make_unique<KnoxicBuffer>(
            knoxicDevice,
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        knoxicDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
    }

    void KnoxicModel::createIndexBuffer(const std::vector<uint32_t> &indices) {
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer = indexCount > 0;

        if (!hasIndexBuffer) return;

        VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
        uint32_t indexSize = sizeof(indices[0]);

        KnoxicBuffer stagingBuffer {
            knoxicDevice,
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void *) indices.data());

        indexBuffer = std::make_unique<KnoxicBuffer>(
            knoxicDevice,
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        knoxicDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
    }

    void KnoxicModel::draw(VkCommandBuffer commandBuffer) {
        if (hasIndexBuffer) {
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        } else  {
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    }

    void KnoxicModel::bind(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = {vertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        
        if (hasIndexBuffer) vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    std::vector<VkVertexInputBindingDescription> KnoxicModel::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> KnoxicModel::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
        attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
        attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
        attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

        return attributeDescriptions;
    }

    void KnoxicModel::Data::loadModel(const std::string &filePath) {
        // Create Assimp importer
        Assimp::Importer importer;
        
        // Import settings for better compatibility and performance
        const aiScene* scene = importer.ReadFile(filePath, 
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType |
            aiProcess_ImproveCacheLocality |
            aiProcess_OptimizeMeshes |
            aiProcess_OptimizeGraph |
            aiProcess_ValidateDataStructure
        );

        // Check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            throw std::runtime_error("Failed to load model: " + filePath);
        }

        // Clear existing data
        vertices.clear();
        indices.clear();

        // Process the root node recursively
        processNode(scene->mRootNode, scene);
    }

    void KnoxicModel::Data::processNode(aiNode* node, const aiScene* scene) {
        // Process all the node's meshes
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene);
        }

        // Process all the node's children
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    void KnoxicModel::Data::processMesh(aiMesh* mesh, const aiScene* scene) {
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        
        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex{};

            // Position
            if (mesh->mVertices) {
                vertex.position = {
                    mesh->mVertices[i].x,
                    mesh->mVertices[i].y,
                    mesh->mVertices[i].z
                };
            }

            // Normal
            if (mesh->mNormals) {
                vertex.normal = {
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                };
            } else {
                vertex.normal = {0.0f, 1.0f, 0.0f}; // Default up normal
            }

            // Texture coordinates
            if (mesh->mTextureCoords[0]) {
                vertex.uv = {
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                };
            } else {
                vertex.uv = {0.0f, 0.0f};
            }

            // Vertex colors
            if (mesh->mColors[0]) {
                vertex.color = {
                    mesh->mColors[0][i].r,
                    mesh->mColors[0][i].g,
                    mesh->mColors[0][i].b
                };
            } else {
                vertex.color = {1.0f, 1.0f, 1.0f}; // Default white
            }

            // Check for unique vertices to avoid duplicates
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                Vertex vertex{};
                
                // Get vertex data for this index
                unsigned int vertexIndex = face.mIndices[j];
                
                if (mesh->mVertices) {
                    vertex.position = {
                        mesh->mVertices[vertexIndex].x,
                        mesh->mVertices[vertexIndex].y,
                        mesh->mVertices[vertexIndex].z
                    };
                }

                if (mesh->mNormals) {
                    vertex.normal = {
                        mesh->mNormals[vertexIndex].x,
                        mesh->mNormals[vertexIndex].y,
                        mesh->mNormals[vertexIndex].z
                    };
                } else {
                    vertex.normal = {0.0f, 1.0f, 0.0f};
                }

                if (mesh->mTextureCoords[0]) {
                    vertex.uv = {
                        mesh->mTextureCoords[0][vertexIndex].x,
                        mesh->mTextureCoords[0][vertexIndex].y
                    };
                } else {
                    vertex.uv = {0.0f, 0.0f};
                }

                if (mesh->mColors[0]) {
                    vertex.color = {
                        mesh->mColors[0][vertexIndex].r,
                        mesh->mColors[0][vertexIndex].g,
                        mesh->mColors[0][vertexIndex].b
                    };
                } else {
                    vertex.color = {1.0f, 1.0f, 1.0f};
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

        // Process material
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            // For now, we'll just note that materials exist
            // Future enhancement: load textures here
        }
    }

    void KnoxicModel::Data::loadMaterialTextures(aiMaterial* mat, int type, const std::string& directory) {
        // This method can be implemented later for automatic texture loading
        // For now, textures are loaded manually through the material system
    }
}