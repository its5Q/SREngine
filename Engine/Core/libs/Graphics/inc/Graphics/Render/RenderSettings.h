//
// Created by Monika on 11.08.2023.
//

#ifndef SRENGINE_RENDERSETTINGS_H
#define SRENGINE_RENDERSETTINGS_H

#include <Graphics/Render/RenderSettings.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(Quality, uint8_t,
        None,
        Low,
        Medium,
        High,
        Ultra,
        Extreme
    );

    class RenderSettings : public SR_UTILS_NS::NonCopyable {
        Quality shadows = Quality::None;
        Quality ScreenSpaceAO = Quality::None;
        Quality particles = Quality::None;

    };
}

#endif //SRENGINE_RENDERSETTINGS_H
