//
// Created by Igor on 12/03/2023.
//

#ifndef SRENGINE_ABOUT_H
#define SRENGINE_ABOUT_H

#include <Graphics/GUI/Widget.h>

namespace SR_CORE_GUI_NS {
    class About : public SR_GRAPH_GUI_NS::Widget {
        using Super = SR_GRAPH_GUI_NS::Widget;
    public:
        About();

    protected:
        void Draw() override;

    };
}

#endif //SRENGINE_ABOUT_H
