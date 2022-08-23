 //
// Created by Nikita on 27.11.2020.
//

#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/Transform3D.h>
#include <Utils/ECS/Component.h>

#include <Utils/World/Scene.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Quaternion.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Mathematics.h>
#include <Utils/Types/SafePtrLockGuard.h>

namespace SR_UTILS_NS {
    GameObject::GameObject(const ScenePtr& scene, std::string name, std::string tag)
        : Super(this)
    {
        m_name = std::move(name);
        m_tag = std::move(tag);
        m_scene = scene;

        SetTransform(new Transform3D());

        UpdateEntityPath();
    }

    GameObject::~GameObject() {
        delete m_transform;
    }

    Component *Framework::Helper::GameObject::GetComponent(const std::string &name) {
        for (auto&& pComponent : m_components) {
            if (pComponent->GetComponentName() == name) {
                return pComponent;
            }
        }

        return nullptr;
    }

    void GameObject::Destroy(GODestroyByBits by /** = DestroyByFlags::DestroyBy_Other */) {
        if (m_isDestroy) {
            SR_ERROR("GameObject::Destroy() : \"" + m_name + "\" game object already destroyed!");
            return;
        }

        if (m_scene.RecursiveLockIfValid()) {
            const bool byParent = by & GAMEOBJECT_DESTROY_BY_GAMEOBJECT;

            if (!byParent && m_parent.RecursiveLockIfValid()) {
                GameObject::Ptr copy = m_parent;
                copy->RemoveChild(*this);
                copy.Unlock();
            }

            m_scene->Remove(*this);

            m_scene.Unlock();
        }

        for (auto&& pComponent : m_components) {
            pComponent->OnDestroy();
        }
        m_components.clear();

        for (auto&& gameObject : m_children) {
            gameObject.AutoFree([by](GameObject *gm) {
                gm->Destroy(by | GAMEOBJECT_DESTROY_BY_GAMEOBJECT);
            });
        }
        m_children.clear();

        m_isDestroy = true;

        delete this;
    }

    void GameObject::OnMatrixDirty() {
        for (auto&& pComponent : m_components) {
            pComponent->OnMatrixDirty();
        }
    }

    bool GameObject::AddChild(const GameObject::Ptr& child) {
        if (child.Get() == this) {
            SRAssert2(false, "It is impossible to make a parent a child!");
            return false;
        }

        if (child->GetParent()) {
            SRAssert2(false, "Child has parent!");
            return false;
        }

        if (Contains(child)) {
            SRAssert2(false, "This child already exists in this game object!");
            return false;
        }

        if (!child->SetParent(*this)) {
            SR_WARN("GameObject::AddChild() : failed to set parent!");
            return false;
        }

        m_children.push_back(child);

        /* TODO: Update child transforms with parent */

        child->OnAttached();

        m_scene->OnChanged();
        SetDirty();

        return true;
    }

    void GameObject::SetName(const std::string &name) {
        m_name = name;
        m_scene->OnChanged();
    }

    void GameObject::SetTag(const std::string &tag) {
        m_tag = tag;
        m_scene->OnChanged();
    }

    bool GameObject::Contains(const GameObject::Ptr& gameObject) {
        for (auto&& children : m_children) {
            if (children.Get() == gameObject.Get()) {
                return true;
            }
        }

        return false;
    }

    bool GameObject::SetParent(const GameObject::Ptr &parent) {
        if (parent == m_parent) {
            return false;
        }

        GameObject::Ptr oldParent = m_parent;
        m_parent = parent;

        if (!UpdateEntityPath()) {
            m_parent = oldParent;
            return false;
        }

        m_scene->OnChanged();

        return true;
    }

    void GameObject::RemoveChild(const GameObject::Ptr &ptr) {
        ptr->SetParent(GameObject::Ptr());

        for (auto pIt = m_children.begin(); pIt != m_children.end(); ) {
            if (pIt->Get() == ptr.Get()) {
                pIt = m_children.erase(pIt);
                return;
            }

            ++pIt;
        }

        SRHalt(Format("GameObject %s is not child for %s!", ptr->GetName().c_str(), GetName().c_str()));
    }

    bool GameObject::ContainsComponent(const std::string &name) {
        for (auto&& pComponent : m_components) {
            if (pComponent->GetComponentName() == name) {
                return true;
            }
        }

        return false;
    }

    void GameObject::ForEachChild(const std::function<void(GameObject::Ptr &)> &fun) {
        for (auto&& child : m_children) {
            if (child.Valid()) {
                fun(child);
            }
        }
    }

    void GameObject::ForEachChild(const std::function<void(const GameObject::Ptr &)> &fun) const {
        for (auto&& child : m_children) {
            if (child.RecursiveLockIfValid()) {
                fun(child);
                child.Unlock();
            }
        }
    }

    void GameObject::SetEnabled(bool value) {
        if (m_isEnabled == value) {
            return;
        }

        m_isEnabled = value;

        SetDirty();
    }

    bool GameObject::IsActive() const {
        if (m_parent.RecursiveLockIfValid()) {
            const bool parentActive = m_parent->m_isActive;
            m_parent.Unlock();
            return IsEnabled() && parentActive;
        }

        return IsEnabled();
    }

    void GameObject::CheckActivity() noexcept {
        if (!m_dirty) {
            return;
        }

        m_isActive = IsEnabled() && (!m_parent || m_parent->m_isActive);

        for (auto&& pComponent : m_components) {
            if (!pComponent->IsAwake()) {
                continue;
            }

            pComponent->CheckActivity();
        }

        for (auto&& child : m_children) {
            child->m_dirty = true;
            child->CheckActivity();
        }
    }


    void GameObject::Awake(bool isPaused) noexcept {
        /// Проверяем на IsEnabled а не на IsActive,
        /// так как если родитель не активен, то метод не вызвался бы.
        if (!m_dirty || !IsEnabled()) {
            return;
        }

        for (auto&& pComponent : m_components) {
            if (isPaused && !pComponent->ExecuteInEditMode()) {
                continue;
            }

            if (pComponent->IsAwake()) {
                continue;
            }

            pComponent->Awake();
        }

        for (auto&& child : m_children) {
            child->m_dirty = true;
            child->Awake(isPaused);
        }
    }

    void GameObject::Start() noexcept {
        /// Проверяем на IsEnabled а не на IsActive,
        /// так как если родитель не активен, то метод не вызвался бы.
        if (!m_dirty || !IsEnabled()) {
            return;
        }

        for (auto&& pComponent : m_components) {
            if (!pComponent->IsAwake()) {
                continue;
            }

            if (pComponent->IsStarted()) {
                continue;
            }

            pComponent->Start();
        }

        for (auto&& child : m_children) {
            child->m_dirty = true;
            child->Start();
        }
    }

    SR_MATH_NS::FVector3 GameObject::GetBarycenter() {
        auto barycenter = Math::FVector3();
        uint32_t count = 0;

        //for (auto comp : m_components)
        //    if (auto br = comp->GetBarycenter(); !br.IsInfinity()) {
        //        barycenter += br;
        //        count++;
        //    }
        //
        //if (count != 0) {
        //    barycenter /= count;
        //    if (!m_parent.Valid())
        //        return barycenter;
        //    ///else
        //    ///return barycenter + m_transform->m_globalPosition;
        //}

        return Math::FVector3(Math::InfinityFV3);
    }

    Math::FVector3 GameObject::GetHierarchyBarycenter() {
        auto barycenter = Math::FVector3((Math::Unit) 0);
        uint32_t count = 0;

        if (auto self = this->GetBarycenter(); !self.IsInfinity()) {
            barycenter += self;
            count++;
        }

        ForEachChild([=](const GameObject::Ptr &child) mutable {
            if (auto self = this->GetBarycenter(); !self.IsInfinity()) {
                barycenter += self;
                count++;
            }
        });

        return count == 0 ? Math::InfinityFV3 : barycenter / count;
    }

    void GameObject::ForEachComponent(const std::function<bool(Component *)> &fun) {
        for (auto&& component : m_components) {
            if (!fun(component)) {
                break;
            }
        }
    }

    bool GameObject::LoadComponent(Component *pComponent) {
        if (m_isDestroy) {
            SR_ERROR("GameObject::LoadComponent() : this \"" + m_name + "\" game object is destroyed!");
            return false;
        }

        m_components.emplace_back(pComponent);
        pComponent->SetParent(this);
        pComponent->OnAttached();

        return true;
    }

    bool GameObject::AddComponent(Component* pComponent) {
        if (m_isDestroy) {
            SR_ERROR("GameObject::AddComponent() : this \"" + m_name + "\" game object is destroyed!");
            return false;
        }

        m_components.emplace_back(pComponent);
        pComponent->SetParent(this);
        pComponent->OnAttached();
        SetDirty();

        return true;
    }

    bool GameObject::RemoveComponent(Component *component) {
        for (auto it = m_components.begin(); it != m_components.end(); ++it) {
            if (*it != component) {
                continue;
            }

            if (component->GetParent() != this) {
                SRHalt("Game object not are children!");
            }

            component->OnDestroy();
            m_components.erase(it);
            SetDirty();

            return true;
        }

        SR_ERROR("GameObject::RemoveComponent() : component \"" + component->GetComponentName() + "\" not found!");

        return false;
    }

    bool GameObject::ReplaceComponent(Component *source, Component *destination) {
        if (m_isDestroy) {
            SR_ERROR("GameObject::ReplaceComponent() : this \"" + m_name + "\" game object is destroyed!");
            return false;
        }

        for (auto it = m_components.begin(); it != m_components.end(); ++it) {
            if (*it == source) {
                source->OnDestroy();
                *it = destination;

                destination->SetParent(this);
                destination->OnAttached();
                SetDirty();

                return true;
            }
        }

        SR_ERROR("GameObject::ReplaceComponent() : component \"" + source->GetComponentName() + "\" not found!");

        return false;
    }

    SR_HTYPES_NS::Marshal GameObject::Save(SavableFlags flags) const {
        if (GetFlags() & GAMEOBJECT_FLAG_NO_SAVE) {
            return SR_HTYPES_NS::Marshal();
        }

        SR_HTYPES_NS::Marshal marshal = Entity::Save(flags);

        marshal.Write(VERSION);

        marshal.Write(IsEnabled());
        marshal.Write(m_name);

        if (HasTag()) {
            marshal.Write(true);
            marshal.Write(m_tag);
        }
        else {
            marshal.Write(false);
        }

        marshal.Append(m_transform->Save(flags));

        /// save components

        marshal.Write(static_cast<uint32_t>(m_components.size()));
        for (auto&& pComponent : m_components) {
            auto&& marshalComponent = pComponent->Save(flags);
            marshal.Write<uint64_t>(marshalComponent.BytesCount());
            marshal.Append(std::move(marshalComponent));
        }

        /// save children

        uint32_t childrenNum = 0;
        for (auto&& child : m_children) {
            if (child->GetFlags() & GAMEOBJECT_FLAG_NO_SAVE) {
                continue;
            }
            ++childrenNum;
        }

        marshal.Write(static_cast<uint32_t>(childrenNum));
        for (auto&& child : m_children) {
            if (child->GetFlags() & GAMEOBJECT_FLAG_NO_SAVE) {
                continue;
            }
            marshal.Append(child->Save(flags));
        }

        return marshal;
    }

    bool GameObject::UpdateEntityPath() {
        GameObject::Ptr current = *this;
        EntityPath path;

        do {
            path.ConcatBack(current->GetEntityId());
            current = current->m_parent;

            if (current && current->GetEntityId() == GetEntityId()) {
                SRAssert2(false, "Recursive entity path!");
                return false;
            }
        } while (current.Valid());

        SetEntityPath(path);

        return true;
    }

    Component *GameObject::GetComponent(size_t id) {
        for (auto&& pComponent : m_components) {
            if (pComponent->GetComponentId() != id) {
                continue;
            }

            return pComponent;
        }

        return nullptr;
    }

    void GameObject::SetTransform(Transform *transform) {
        if (m_transform == transform || !transform) {
            SR_WARN("GameObject::SetTransform() : invalid transform!");
        }
        else {
            if (m_transform) {
                delete m_transform;
            }
            m_transform = transform;
            m_transform->SetGameObject(this);
            SetDirty();
        }
    }

    std::list<EntityBranch> GameObject::GetEntityBranches() const {
        std::list<EntityBranch> branches;

        ForEachChild([&branches](const GameObject::Ptr &ptr) {
            branches.emplace_back(ptr->GetEntityTree());
        });

        return std::move(branches);
    }

    bool GameObject::MoveToTree(const GameObject::Ptr &destination) {
        if (m_parent) {
            GameObject::Ptr copy = m_parent;
            if (copy.RecursiveLockIfValid()) {
                copy->RemoveChild(*this);
                copy->Unlock();
            }
        }

        return destination->AddChild(*this);
    }

    bool GameObject::HasTag() const {
        return m_tag != "Untagged";
    }

    void GameObject::OnAttached() {

    }

    void GameObject::FixedUpdate(bool isPaused) noexcept {
        if (!m_isEnabled) {
            return;
        }

        m_dirty = false;

        uint32_t i, size = static_cast<uint32_t>(m_components.size());

        for (i = 0; i < size; ++i) {
            auto&& pComponent = m_components.at(i);

            if (isPaused && !pComponent->ExecuteInEditMode()) {
                continue;
            }

            if (!pComponent->IsEnabled() || !pComponent->IsStarted()) {
                continue;
            }

            pComponent->FixedUpdate();
        }

        size = static_cast<uint32_t>(m_children.size());

        for (i = 0; i < size; ++i) {
            m_children.at(i)->FixedUpdate(isPaused);
        }
    }

    void GameObject::Update(float_t dt, bool isPaused) noexcept {
        if (!m_isEnabled) {
            return;
        }

        uint32_t i, size = static_cast<uint32_t>(m_components.size());

        for (i = 0; i < size; ++i) {
            auto&& pComponent = m_components.at(i);

            if (isPaused && !pComponent->ExecuteInEditMode()) {
                continue;
            }

            if (!pComponent->IsEnabled() || !pComponent->IsStarted()) {
                continue;
            }

            pComponent->Update(dt);
        }

        size = static_cast<uint32_t>(m_children.size());

        for (i = 0; i < size; ++i) {
            m_children.at(i)->Update(dt, isPaused);
        }
    }

    void GameObject::SetDirty() {
        m_dirty = true;
        if (m_parent) {
            m_parent->SetDirty();
        }
    }
}