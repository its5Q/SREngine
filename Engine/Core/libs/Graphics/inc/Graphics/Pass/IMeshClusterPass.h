//
// Created by Monika on 04.06.2023.
//

#ifndef SRENGINE_IMESHCLUSTERPASS_H
#define SRENGINE_IMESHCLUSTERPASS_H

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Render/MeshCluster.h>

namespace SR_GRAPH_NS {
    class ShadowMapPass;
    class CascadedShadowMapPass;

    class IMeshClusterPass : public BasePass {
        using Super = BasePass;
    public:
        struct Sampler {
            uint32_t textureId = SR_ID_INVALID;
            uint32_t fboId = SR_ID_INVALID;
            SR_UTILS_NS::StringAtom id;
            SR_UTILS_NS::StringAtom fboName;
            uint64_t index = 0;
            bool depth = false;
        };
        using Samplers = std::vector<Sampler>;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using MeshPtr = SR_GTYPES_NS::Mesh*;
        using FramebufferPtr = SR_GTYPES_NS::Framebuffer*;
    public:
        bool Load(const SR_XML_NS::Node& passNode) override;

        bool Init() override;

        void Prepare() override;
        bool Render() override;
        void Update() override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;
        void OnSamplesChanged() override;

    protected:
        SR_NODISCARD virtual MeshClusterTypeFlag GetClusterType() const noexcept;
        SR_NODISCARD virtual ShaderPtr GetShader(SR_SRSL_NS::ShaderType shaderType) const { return nullptr; }

        virtual void UseSamplers(ShaderPtr pShader);
        virtual void UseUniforms(ShaderPtr pShader, MeshPtr pMesh);
        virtual void UseSharedUniforms(ShaderPtr pShader);
        virtual void UseConstants(ShaderPtr pShader);

        virtual void PrepareSamplers();
        virtual void PrepareFBODependencies();
        virtual void OnClusterDirty() { }

    protected:
        bool m_hasRendered = false;
        bool m_dirtySamplers = true;
        bool m_needUpdateMeshes = false;
        Samplers m_samplers;
        MeshClusterTypeFlag m_meshClusters = 0;

    };
}

#endif //SRENGINE_IMESHCLUSTERPASS_H
