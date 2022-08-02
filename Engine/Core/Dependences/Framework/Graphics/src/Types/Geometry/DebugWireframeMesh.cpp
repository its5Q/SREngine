//
// Created by Monika on 29.10.2021.
//

#include <Types/Geometry/DebugWireframeMesh.h>
#include <Utils/Types/RawMesh.h>

namespace SR_GTYPES_NS {
    DebugWireframeMesh::DebugWireframeMesh()
        : IndexedMesh(MeshType::Wireframe)
    {
        /// override component
        Component::InitComponent<DebugWireframeMesh>();
    }

    DebugWireframeMesh::~DebugWireframeMesh() {
        SetRawMesh(nullptr);
    }

    SR_UTILS_NS::IResource* DebugWireframeMesh::Copy(IResource* destination) const {
        SR_LOCK_GUARD_INHERIT(SR_UTILS_NS::IResource);

        auto* wireFramed = dynamic_cast<DebugWireframeMesh *>(destination ? destination : new DebugWireframeMesh());
        wireFramed = dynamic_cast<DebugWireframeMesh *>(Framework::Graphics::Types::IndexedMesh::Copy(wireFramed));

        wireFramed->SetRawMesh(m_rawMesh);
        wireFramed->m_meshId = m_meshId;

        if (wireFramed->IsCalculated()) {
            auto &&manager = Memory::MeshManager::Instance();
            wireFramed->m_VBO = manager.CopyIfExists<Vertices::Type::SimpleVertex, Memory::MeshMemoryType::VBO>(GetResourceId());
        }

        return wireFramed;
    }

    void DebugWireframeMesh::Draw() {
        if (!IsActive() || IsDestroyed())
            return;

        if ((!m_isCalculated && !Calculate()) || m_hasErrors)
            return;

        auto &&shader = m_material->GetShader();
        auto&& uboManager = Memory::UBOManager::Instance();

        if (m_dirtyMaterial)
        {
            m_dirtyMaterial = false;

            m_virtualUBO = uboManager.ReAllocateUBO(m_virtualUBO, shader->GetUBOBlockSize(), shader->GetSamplersCount());

            if (m_virtualUBO != SR_ID_INVALID) {
                uboManager.BindUBO(m_virtualUBO);
            }
            else {
                m_pipeline->ResetDescriptorSet();
                m_hasErrors = true;
                return;
            }

            shader->InitUBOBlock();
            shader->Flush();

            m_material->UseSamplers();
        }

        switch (uboManager.BindUBO(m_virtualUBO)) {
            case Memory::UBOManager::BindResult::Duplicated:
                shader->InitUBOBlock();
                shader->Flush();
                m_material->UseSamplers();
                SR_FALLTHROUGH;
            case Memory::UBOManager::BindResult::Success:
                m_pipeline->DrawIndices(m_countIndices);
                break;
            case Memory::UBOManager::BindResult::Failed:
            default:
                break;
        }
    }

    bool DebugWireframeMesh::Calculate() {
        SR_LOCK_GUARD_INHERIT(SR_UTILS_NS::IResource);

        if (m_isCalculated)
            return true;

        const bool iboOK = m_IBO != SR_ID_INVALID;
        if (m_VBO != SR_ID_INVALID && iboOK && !m_hasErrors) {
            m_isCalculated = true;
            return true;
        }

        if (!IsCanCalculate()) {
            return false;
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("DebugWireframeMesh::Calculate() : calculating \"" + m_geometryName + "\"...");
        }

        auto vertices = Vertices::CastVertices<Vertices::SimpleVertex>(m_rawMesh->GetVertices(m_meshId));

        if (!CalculateVBO<Vertices::Type::SimpleVertex>(vertices))
            return false;

        return IndexedMesh::Calculate();
    }

    bool DebugWireframeMesh::Unload() {
        SetRawMesh(nullptr);
        return Mesh::Unload();
    }

    void DebugWireframeMesh::SetRawMesh(SR_HTYPES_NS::RawMesh *pRaw) {
        if (m_rawMesh) {
            RemoveDependency(m_rawMesh);
        }

        if (pRaw) {
            AddDependency(pRaw);
        }

        m_rawMesh = pRaw;
    }

    bool DebugWireframeMesh::Load() {
        if (auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(GetResourcePath())) {
            if (m_meshId >= pRawMesh->GetMeshesCount()) {
                if (pRawMesh->GetCountUses() == 0) {
                    SRHalt("DebugWireframeMesh::Load() : count uses is zero! Unresolved situation...");
                    pRawMesh->Destroy();
                }
                return false;
            }

            m_countIndices = pRawMesh->GetIndicesCount(m_meshId);
            m_countVertices = pRawMesh->GetVerticesCount(m_meshId);

            SetGeometryName(pRawMesh->GetGeometryName(m_meshId));
            SetRawMesh(pRawMesh);

            return true;
        }

        return false;
    }

    void DebugWireframeMesh::FreeVideoMemory() {
        SR_LOCK_GUARD_INHERIT(SR_UTILS_NS::IResource);

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("DebugWireframeMesh::FreeVideoMemory() : free \"" + m_geometryName + "\" mesh video memory...");
        }

        if (!FreeVBO<Vertices::Type::SimpleVertex>()) {
            SR_ERROR("DebugWireframeMesh::FreeVideoMemory() : failed to free VBO!");
        }

        IndexedMesh::FreeVideoMemory();
    }

    std::vector<uint32_t> DebugWireframeMesh::GetIndices() const {
        return m_rawMesh->GetIndices(m_meshId);
    }

    bool DebugWireframeMesh::Reload() {
        SR_SHADER_LOG("DebugWireframeMesh::Reload() : reloading \"" + GetResourceId() + "\" mesh...");

        m_loadState = LoadState::Reloading;

        Unload();

        if (!Load()) {
            return false;
        }

        m_loadState = LoadState::Loaded;

        UpdateResources();

        return true;
    }
}