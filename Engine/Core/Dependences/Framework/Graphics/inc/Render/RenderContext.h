//
// Created by Monika on 13.07.2022.
//

#ifndef SRENGINE_RENDERCONTEXT_H
#define SRENGINE_RENDERCONTEXT_H

#include <Render/MeshCluster.h>
#include <Utils/World/Scene.h>
#include <Utils/Types/SafePointer.h>
#include <Memory/IGraphicsResource.h>

namespace SR_GTYPES_NS {
    class Framebuffer;
    class Shader;
    class Texture;
    class Skybox;
    class Material;
}

namespace SR_GRAPH_NS {
    class RenderScene;
    class RenderTechnique;
    class Environment;

    /**
     * Здесь хранятся все контекстные ресурсы.
     * Исключение - меши, потому что они могут быть в нескольких экземплярах.
     * Управлением памяти мешей занимается MeshCluster, который у каждой RenderScene свой.
     */
    class RenderContext : public SR_HTYPES_NS::SafePtr<RenderContext> {
        using RenderScenePtr = SR_HTYPES_NS::SafePtr<RenderScene>;
        using PipelinePtr = Environment*;
        using Super = SR_HTYPES_NS::SafePtr<RenderContext>;
        using MaterialPtr = SR_GTYPES_NS::Material*;
        using TexturePtr = SR_GTYPES_NS::Texture*;
        using SkyboxPtr = SR_GTYPES_NS::Skybox*;
    public:
        RenderContext();
        virtual ~RenderContext() = default;

    public:
        void Update();

        bool Init();
        void Close();

        void SetDirty();

    public:
        RenderScenePtr CreateScene(const SR_WORLD_NS::Scene::Ptr& scene);

        void Register(Types::Framebuffer* pFramebuffer);
        void Register(Types::Shader* pShader);
        void Register(Types::Texture* pTexture);
        void Register(RenderTechnique* pTechnique);
        void Register(MaterialPtr pMaterial);
        void Register(SkyboxPtr pSkybox);

        SR_NODISCARD bool IsEmpty() const;
        SR_NODISCARD PipelinePtr GetPipeline() const;
        SR_NODISCARD PipeLineType GetPipelineType() const;
        SR_NODISCARD MaterialPtr GetDefaultMaterial() const;
        SR_NODISCARD TexturePtr GetDefaultTexture() const;
        SR_NODISCARD TexturePtr GetNoneTexture() const;

    private:
        template<typename T> bool Update(std::list<T>& resourceList);

    private:
        std::list<Types::Framebuffer*> m_framebuffers;
        std::list<Types::Shader*> m_shaders;
        std::list<TexturePtr> m_textures;
        std::list<RenderTechnique*> m_techniques;
        std::list<MaterialPtr> m_materials;
        std::list<SkyboxPtr> m_skyboxes;

        std::list<std::pair<SR_WORLD_NS::Scene::Ptr, RenderScenePtr>> m_scenes;

        MaterialPtr m_defaultMaterial = nullptr;
        TexturePtr m_defaultTexture = nullptr;
        TexturePtr m_noneTexture = nullptr;

        PipelinePtr m_pipeline = nullptr;
        PipeLineType m_pipelineType = PipeLineType::Unknown;

    };

    /// ------------------------------------------------------------------------------

    template<typename T> bool RenderContext::Update(std::list<T> &resourceList) {
        bool dirty = false;

        for (auto pIt = std::begin(resourceList); pIt != std::end(resourceList); ) {
            auto&& pResource = *pIt;

            if (pResource->GetCountUses() == 1) {
                /// Ресурс необязательно имеет видеопамять, а лишь содержит другие ресурсы, например материал.
                if (auto&& pGraphicsResource = dynamic_cast<Memory::IGraphicsResource*>(pResource)) {
                    pGraphicsResource->FreeVideoMemory();
                }
                /// Сперва ставим ресурс на уничтожение
                pResource->Destroy();
                /// Затем убираем use-point, чтобы его можно было синхронно освободить.
                /// Иначе ресурс может дважды уничтожиться.
                pResource->RemoveUsePoint();
                pIt = resourceList.erase(pIt);
                /// После освобождения ресурса необходимо перестроить все контекстные сцены рендера.
                dirty |= true;
            }
            else {
                ++pIt;
            }
        }

        return dirty;
    }
}

#endif //SRENGINE_RENDERCONTEXT_H
