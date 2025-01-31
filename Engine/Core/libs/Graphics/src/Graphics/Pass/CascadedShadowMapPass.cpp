//
// Created by Monika on 06.06.2023.
//

#include <Graphics/Pass/CascadedShadowMapPass.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(CascadedShadowMapPass);

    bool CascadedShadowMapPass::Init() {
        return Super::Init();
    }

    void CascadedShadowMapPass::DeInit() {
        Super::DeInit();
    }

    bool CascadedShadowMapPass::Load(const SR_XML_NS::Node& passNode) {
        m_cascadesCount = passNode.TryGetAttribute("Cascades").ToUInt64(4);
        m_cascadeSplitLambda = passNode.TryGetAttribute("SplitLambda").ToFloat(0.95);
        m_usePerspective = passNode.TryGetAttribute("UsePerspective").ToBool(false);
        m_near = passNode.TryGetAttribute("Near").ToFloat(0.1f);
        m_far = passNode.TryGetAttribute("Far").ToFloat(100.f);
        return Super::Load(passNode);
    }

    MeshClusterTypeFlag CascadedShadowMapPass::GetClusterType() const noexcept {
        return static_cast<uint64_t>(MeshClusterType::Opaque) | static_cast<uint64_t>(MeshClusterType::Transparent);
    }

    void CascadedShadowMapPass::UseSharedUniforms(IMeshClusterPass::ShaderPtr pShader) {
        if (m_camera) {
            pShader->SetMat4(SHADER_VIEW_MATRIX, m_camera->GetViewTranslateRef());
            pShader->SetMat4(SHADER_PROJECTION_MATRIX, m_camera->GetProjectionRef());
        }
        Super::UseSharedUniforms(pShader);
    }

    void CascadedShadowMapPass::UseUniforms(IMeshClusterPass::ShaderPtr pShader, IMeshClusterPass::MeshPtr pMesh) {
        SR_TRACY_ZONE;

        pMesh->UseModelMatrix();

        if (CheckCamera()) {
            UpdateCascades();
        }

        pShader->SetValue<false>(SHADER_CASCADE_LIGHT_SPACE_MATRICES, m_cascadeMatrices.data());

        SR_MATH_NS::FVector3 lightPos = GetRenderScene()->GetLightSystem()->m_position;
        pShader->SetVec3(SHADER_DIRECTIONAL_LIGHT_POSITION, lightPos);
    }

    void CascadedShadowMapPass::UpdateCascades() {
        SR_MATH_NS::FVector3 lightPos = GetRenderScene()->GetLightSystem()->m_position;

        std::vector<float_t> cascadeSplits;
        cascadeSplits.resize(m_cascadesCount);

        m_cascadeMatrices.resize(4);
        m_cascadeSplitDepths.resize(4);

        const float_t clipRange = m_far - m_near;

        const float_t minZ = m_near;
        const float_t maxZ = m_near + clipRange;

        const float_t range = maxZ - minZ;
        const float_t ratio = maxZ / minZ;

        for (uint32_t i = 0; i < m_cascadesCount; i++) {
            const float_t p = (i + 1) / static_cast<float_t>(m_cascadesCount);
            const float_t log = minZ * std::pow(ratio, p);
            const float_t uniform = minZ + range * p;
            const float_t d = m_cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - m_near) / clipRange;
        }

        float_t lastSplitDist = 0.0;

        for (uint32_t i = 0; i < m_cascadesCount; i++) {
            const float_t splitDist = cascadeSplits[i];

            SR_MATH_NS::FVector3 frustumCorners[8] = {
                SR_MATH_NS::FVector3(-1.0f,  1.0f, -1.0f),
                SR_MATH_NS::FVector3( 1.0f,  1.0f, -1.0f),
                SR_MATH_NS::FVector3( 1.0f, -1.0f, -1.0f),
                SR_MATH_NS::FVector3(-1.0f, -1.0f, -1.0f),
                SR_MATH_NS::FVector3(-1.0f,  1.0f,  1.0f),
                SR_MATH_NS::FVector3( 1.0f,  1.0f,  1.0f),
                SR_MATH_NS::FVector3( 1.0f, -1.0f,  1.0f),
                SR_MATH_NS::FVector3(-1.0f, -1.0f,  1.0f),
            };

            auto&& invCamera = (m_camera->GetProjectionRef() * m_camera->GetViewTranslateRef()).Inverse();

            for (uint32_t j = 0; j < 8; j++) {
                SR_MATH_NS::FVector4 invCorner = invCamera * SR_MATH_NS::FVector4(frustumCorners[j], 1.0f);
                frustumCorners[j] = (invCorner / invCorner.w).XYZ();
            }

            for (uint32_t j = 0; j < 4; j++) {
                SR_MATH_NS::FVector3 dist = frustumCorners[j + 4] - frustumCorners[j];
                frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
            }

            SR_MATH_NS::FVector3 frustumCenter = SR_MATH_NS::FVector3(0.0f);
            for (uint32_t j = 0; j < 8; j++) {
                frustumCenter += frustumCorners[j];
            }
            frustumCenter /= 8.0f;

            float_t radius = 0.0f;
            for (uint32_t j = 0; j < 8; j++) {
                float_t distance = (frustumCorners[j] - frustumCenter).Length();
                radius = SR_MAX(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            SR_MATH_NS::FVector3 maxExtents = SR_MATH_NS::FVector3(radius);
            SR_MATH_NS::FVector3 minExtents = -maxExtents;

            SR_MATH_NS::FVector3 lightDir = (-lightPos).Normalize();

            SR_MATH_NS::Matrix4x4 lightViewMatrix = SR_MATH_NS::Matrix4x4::LookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0f));

            m_cascadeSplitDepths[i] = (m_near + splitDist * clipRange) * -1.0f;

            if (m_usePerspective) {
                /// TODO: not works
                m_cascadeMatrices[i] = m_camera->GetProjectionRef() * lightViewMatrix;
            }
            else {
                auto&& lightOrthoMatrix = SR_MATH_NS::Matrix4x4::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
                m_cascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;
            }

            lastSplitDist = cascadeSplits[i];
        }
    }

    bool CascadedShadowMapPass::Render() {
        if (m_cascadesCount == 0 || IsDirectional() || !m_framebuffer) {
            return false;
        }

        bool rendered = false;

        m_framebuffer->Update();
        /// установим кадровый буфер, чтобы BeginCmdBuffer понимал какие значение для очистки ставить
        GetPipeline()->SetCurrentFrameBuffer(m_framebuffer);

        m_framebuffer->BeginCmdBuffer(m_clearColors, m_depth);
        m_framebuffer->SetViewportScissor();

        for (uint32_t i = 0; i < m_cascadesCount; ++i) {
            m_currentCascade = i;
            GetPipeline()->SetFrameBufferLayer(i);

            auto&& pIdentifier = m_uboManager.GetIdentifier();
            m_uboManager.SetIdentifier(this);

            if (m_framebuffer->Bind()) {
                m_framebuffer->BeginRender();
                IMesh3DClusterPass::Render(); /// NOLINT
                m_framebuffer->EndRender();
            }

            m_uboManager.SetIdentifier(pIdentifier);

            rendered |= IsDirectional();
        }

        m_framebuffer->EndCmdBuffer();

        return rendered;
    }

    void CascadedShadowMapPass::Update() {
        for (uint32_t i = 0; i < m_cascadesCount; ++i) {
            m_currentCascade = i;
            GetPipeline()->SetFrameBufferLayer(i);
            Super::Update();
        }
    }

    void CascadedShadowMapPass::UseConstants(IMeshClusterPass::ShaderPtr pShader) {
        pShader->SetConstInt(SHADER_SHADOW_CASCADE_INDEX, m_currentCascade);
        Super::UseConstants(pShader);
    }

    bool CascadedShadowMapPass::CheckCamera() {
        if (!m_camera) {
            return false;
        }

        if (m_cameraPosition.Distance(m_camera->GetPosition()) > 1.0) {
            goto dirty;
        }

        if (m_cameraRotation != m_camera->GetRotation()) {
            goto dirty;
        }

        if (m_screenSize != m_camera->GetSize()) {
            goto dirty;
        }

        return false;

    dirty:
        m_cameraPosition = m_camera->GetPosition();
        m_cameraRotation = m_camera->GetRotation();
        m_screenSize = m_camera->GetSize();

        return true;
    }
}