//
// Created by Nikita on 17.11.2020.
//

#ifndef GAMEENGINE_MESH_H
#define GAMEENGINE_MESH_H

#include <vector>
#include <macros.h>

#include <Types/Material.h>
#include <Types/RawMesh.h>

#include <Environment/Environment.h>
#include <EntityComponentSystem/Component.h>
#include <Types/List.h>
#include <FbxLoader/Loader.h>

namespace SR_UTILS_NS::Types {
    class RawMesh;
}

namespace Framework::Graphics {
    class Render;
    class Shader;
}

namespace Framework {
    class API;
}

namespace Framework::Graphics::Types {
    class Material;

    enum class MeshFeatures {
        None     = 0,
        Vertices = 1 << 0,
        Indexed  = 1 << 1,
        Skinned  = 1 << 2
    };

    SR_ENUM_CLASS(MeshType,
        Unknown = 0,
        Static = 1,
        Wireframe = 2,
        Skinned = 3,
    )

    class Mesh : public Helper::IResource, public Helper::Component {
        friend class Material;
        friend class ::Framework::API;
    protected:
        explicit Mesh(MeshType type, const std::string& name = "Unnamed");
        ~Mesh() override;

    public:
        static std::vector<Mesh*> Load(const std::string& path, MeshType type);
        static Mesh* TryLoad(const std::string& path, MeshType type, uint32_t id);
        static Mesh* Load(const std::string& path, MeshType type, uint32_t id);
        static Mesh* LoadFbx(MeshType type, bool withIndices, const FbxLoader::Geometry& geometry);

    public:
        bool Destroy() override;

    protected:
        virtual void ReCalcModel();
        virtual bool Calculate();
        virtual void SetRawMesh(Helper::Types::RawMesh* raw);

    public:
        IResource* Copy(IResource* destination) const override;

        virtual void DrawVulkan() = 0;
        virtual void DrawOpenGL() = 0;
        virtual void UpdateUBO() = 0;

        /** \warning call only from render */
        virtual bool FreeVideoMemory();

    public:
        Helper::Math::FVector3 GetBarycenter() const override;

        void OnMove(const Helper::Math::FVector3& newValue) override;
        void OnRotate(const Helper::Math::FVector3& newValue) override;
        void OnScaled(const Helper::Math::FVector3& newValue) override;
        void OnSkewed(const Helper::Math::FVector3& newValue) override;

        void OnSelected(bool value) override;
        void OnDestroyGameObject() override;
        void OnRemoveComponent() override {
            OnDestroyGameObject();
        }
        void OnReady(bool ready) override {
            m_env->SetBuildState(false);
        }
        void OnAttachComponent() override { }
        void OnTransparencyChanged();

    public:
        void WaitCalculate() const;
        bool IsCanCalculate() const;
        std::string GetPath() const;

        SR_NODISCARD std::string GetGeometryName() const { return m_geometryName; }
        SR_NODISCARD Shader* GetShader()           const { return m_shader; }
        SR_NODISCARD Render* GetRender()           const { return m_render; }
        SR_NODISCARD Material* GetMaterial()       const { return m_material; }
        SR_NODISCARD bool IsCalculated()           const { return m_isCalculated; }
        SR_NODISCARD bool IsInverse()              const { return m_inverse; }
        SR_NODISCARD bool IsRegistered()           const { return m_render; }
        SR_NODISCARD uint32_t GetMeshId()          const { return m_meshId; }

        void SetRender(Render* render) { m_render = render; };
        void SetInverse(bool value) { m_inverse = value; ReCalcModel(); }
        void SetGeometryName(const std::string& name) { m_geometryName = name; }
        void SetMaterial(Material* material);
        void SetShader(Shader* shader);

    public:
        Helper::Math::FVector3       m_barycenter        = Helper::Math::FVector3(Helper::Math::UnitMAX);
        Helper::Math::FVector3       m_position          = Helper::Math::FVector3();
        Helper::Math::FVector3       m_rotation          = Helper::Math::FVector3();
        Helper::Math::FVector3       m_skew              = Helper::Math::FVector3(1);
        Helper::Math::FVector3       m_scale             = Helper::Math::FVector3(1);
        glm::mat4                    m_modelMat          = glm::mat4(0);

    protected:
        bool                         m_inverse           = false;

        Environment*                 m_env               = nullptr;
        const PipeLine               m_pipeline          = PipeLine::Unknown;
        const MeshType               m_type              = MeshType::Unknown;

        mutable std::recursive_mutex m_mutex             = std::recursive_mutex();

        std::string                  m_geometryName      = "Unnamed";
        Shader*                      m_shader            = nullptr;
        Render*                      m_render            = nullptr;
        Material*                    m_material          = nullptr;
        Helper::Types::RawMesh*      m_rawMesh           = nullptr;

        std::atomic<bool>            m_hasErrors         = false;
        std::atomic<bool>            m_isCalculated      = false;

        int32_t                      m_descriptorSet     = SR_ID_INVALID;
        int32_t                      m_UBO               = SR_ID_INVALID;

        /// определяет порядок меша в файле, если их там несколько
        uint32_t                     m_meshId            = SR_UINT32_MAX;

    };
}

#endif //GAMEENGINE_MESH_H
