//
// Created by Monika on 07.07.2022.
//

#ifndef SRENGINE_SOUNDMANAGER_H
#define SRENGINE_SOUNDMANAGER_H

#include <Utils/Common/Singleton.h>
#include <Utils/Types/Thread.h>

#include <Audio/PlayParams.h>

namespace SR_AUDIO_NS {
    class Sound;
    class SoundDevice;
    class SoundData;
    class SoundContext;
    class SoundListener;

    using AudioDeviceName = std::string;

    struct PlayData : public SR_UTILS_NS::NonCopyable {
        SoundData* pData = nullptr;
        SoundSource pSource = nullptr;
        PlayParams params;
        float_t offset = 0.f;
        bool isPlaying = false;
        bool isFailed = false;
    };

    class SoundManager : public SR_UTILS_NS::Singleton<SoundManager> {
        SR_REGISTER_SINGLETON(SoundManager)
    public:
        enum class State : uint8_t {
            Stopped, Active, Paused
        };
        using Handle = void*;
    private:
        SoundManager() = default;
        ~SoundManager() override = default;

    public:
        void StopAll();

        Handle Play(const std::string& path);
        Handle Play(const std::string& path, const PlayParams& params);
        Handle Play(Sound* pSound, const PlayParams& params);

        bool IsExists(Handle pHandle) const;
        bool IsPlaying(Handle pHandle) const;
        bool IsInitialized(Handle pHandle) const;
        bool IsFailed(Handle pHandle) const;

        void ApplyParams(Handle pHandle, const PlayParams& params);
        void Stop(Handle pHandle);

        SoundData* Register(Sound* pSound);
        bool Unregister(SoundData** pSoundData);

        SR_NODISCARD SoundListener* CreateListener();
        SR_NODISCARD SoundListener* CreateListener(AudioLibrary library);
        void DestroyListener(SoundListener* pListener);

    protected:
        SR_NODISCARD SoundContext* GetSoundContext(const PlayParams& params) noexcept;
        SR_NODISCARD AudioLibrary GetRelevantLibrary() const noexcept;

        void DestroyPlayData(PlayData* pPlayData);

        bool PlayInternal(PlayData* pPlayData);
        bool PrepareData(PlayData* pPlayData);

        void InitSingleton() override;
        void OnSingletonDestroy() override;

        void Update();
        void Destroy();

    private:
        SR_HTYPES_NS::Thread::Ptr m_thread = nullptr;
        std::atomic<State> m_state = State::Stopped;
        std::list<PlayData*> m_playStack;
        std::map<AudioLibrary, std::map<AudioDeviceName, SoundContext*>> m_contexts;

    };
}

#endif //SRENGINE_SOUNDMANAGER_H
