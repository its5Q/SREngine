//
// Created by Monika on 22.07.2022.
//

#include <Pass/GroupPass.h>

namespace SR_GRAPH_NS {
    GroupPass::GroupPass(RenderTechnique *pTechnique)
        : BasePass(pTechnique)
    { }

    GroupPass::~GroupPass() {
        for (auto&& pPass : m_passes) {
            delete pPass;
        }
        m_passes.clear();
    }

    bool GroupPass::Load(const SR_XML_NS::Node &passNode) {
        for (auto&& subPassNode : passNode.GetNodes()) {
            if (auto&& pPass = SR_ALLOCATE_RENDER_PASS(GetTechnique(), subPassNode)) {
                m_passes.emplace_back(pPass);
            }
            else {
                SR_ERROR("GroupPass::Load() : failed to load \"" + subPassNode.Name() + "\" pass!");
            }
        }

        return BasePass::Load(passNode);
    }

    bool GroupPass::Init() {
        for (auto&& pPass : m_passes) {
            pPass->Init();
        }

        return BasePass::Init();
    }

    void GroupPass::DeInit() {
        for (auto&& pPass : m_passes) {
            pPass->DeInit();
        }

        BasePass::DeInit();
    }

    void GroupPass::Prepare() {
        for (auto&& pPass : m_passes) {
            pPass->Prepare();
        }

        BasePass::Prepare();
    }

    bool GroupPass::PreRender() {
        bool hasDrawData = false;
        for (auto&& pPass : m_passes) {
            hasDrawData |= pPass->PreRender();
        }

        return hasDrawData;
    }

    bool GroupPass::Render() {
        bool hasDrawData = false;
        for (auto&& pPass : m_passes) {
            hasDrawData |= pPass->Render();
        }

        return hasDrawData;
    }

    bool GroupPass::PostRender() {
        bool hasDrawData = false;
        for (auto&& pPass : m_passes) {
            hasDrawData |= pPass->PostRender();
        }

        return hasDrawData;
    }

    void GroupPass::Update() {
        for (auto&& pPass : m_passes) {
            pPass->Update();
        }

        BasePass::Update();
    }

    bool GroupPass::Overlay() {
        bool hasDrawData = false;
        for (auto&& pPass : m_passes) {
            hasDrawData |= pPass->Overlay();
        }

        return hasDrawData;
    }

    void GroupPass::OnResize(const SR_MATH_NS::IVector2 &size) {
        for (auto&& pPass : m_passes) {
            pPass->OnResize(size);
        }

        BasePass::OnResize(size);
    }

    void GroupPass::OnMeshAdded(SR_GTYPES_NS::Mesh *pMesh, bool transparent) {
        for (auto&& pPass : m_passes) {
            pPass->OnMeshAdded(pMesh, transparent);
        }

        BasePass::OnMeshAdded(pMesh, transparent);
    }

    void GroupPass::OnMeshRemoved(SR_GTYPES_NS::Mesh *pMesh, bool transparent) {
        for (auto&& pPass : m_passes) {
            pPass->OnMeshRemoved(pMesh, transparent);
        }

        BasePass::OnMeshRemoved(pMesh, transparent);
    }
}