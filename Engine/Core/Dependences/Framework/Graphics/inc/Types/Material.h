//
// Created by Nikita on 17.11.2020.
//

#ifndef GAMEENGINE_MATERIAL_H
#define GAMEENGINE_MATERIAL_H

#include <Types/Texture.h>
#include <Render/Shader.h>
#include <Math/Vector3.h>

#include <glm/glm.hpp>
#include <macros.h>

namespace Framework::Graphics::Types {
    class Mesh;
    class Mesh3D;

    /// TO_REFACTORING (need make as resource)
    class Material {
        friend class Mesh;
        friend class Mesh3D;
    public:
        Material() : Material(nullptr, nullptr, nullptr, nullptr) { }
        Material(Texture* diffuse, Texture* normal, Texture* specular, Texture* glossiness);
    private:
        ~Material();
    private:
        std::vector<Mesh*>         m_subscriptions  = {};

        const Environment*         m_env            = nullptr;

        bool                       m_transparent    = false;
        volatile bool              m_bloom          = false;

        glm::vec4                  m_color          = glm::vec4(1, 1, 1, 1);

        Texture*                   m_diffuse        = nullptr;
        Texture*                   m_normal         = nullptr;
        Texture*                   m_specular       = nullptr;
        Texture*                   m_glossiness     = nullptr;
    private:
        bool FreeTextures();
    public:
        void UseVulkan();
        void UseOpenGL() const;
    public:
        Material* Copy();
        bool SetTransparent(bool value);

        void SetBloom(bool value) { this->m_bloom = value; };
        [[nodiscard]] bool GetBloomEnabled() const { return this->m_bloom;  };
        [[nodiscard]] bool IsTransparent() const { return m_transparent;  };

        void SetColor(float r, float g, float b)    { this->m_color = { r, g, b, 1};                  }
        void SetColor(Helper::Math::FVector3 color) { this->m_color = {color.x, color.y, color.z, 1}; }
        void SetColor(glm::vec4 color)              { this->m_color = color;                          }

        [[nodiscard]] glm::vec4 GetColor() const { return this->m_color; }

        void SetDiffuse(Texture* tex);
        void SetNormal(Texture* tex);
        void SetSpecular(Texture* tex);
        void SetGlossiness(Texture* tex);
    };
}

#endif //GAMEENGINE_MATERIAL_H
