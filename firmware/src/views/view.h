#pragma once

#include "lvgl.h"
#include "../proto_gen/smartknob.pb.h"

typedef enum {
    VIEW_CIRCLE_MENU          = 0,
    VIEW_LIST_MENU,
    VIEW_DIAL
} view_t;

// Icons (codepoints) from Google Fonts, https://fonts.google.com/icons
#define ICON_GEAR         "\ue8b8"
#define ICON_BACK_ARROW   "\ue5c4"
#define ICON_ELLIPSIS     "\ue5d3"
#define ICON_NOTE_STACK   "\uf562"

#define ICON_CEILING_LAMP "\uf02a"

#define ICON_PLAY_PAUSE   "\uf137"
#define ICON_SKIP_NEXT    "\ue044"
#define ICON_SKIP_PREV    "\ue045"
#define ICON_VOLUME       "\ue050"

#define ICON_TIMER        "\ue425"
#define ICON_SHADES       "\uec12"
#define ICON_HEATING      "\ue1ff"

class View {
    public:
        View() {}
        virtual ~View(){}

        virtual void setupView(PB_SmartKnobConfig config) = 0;
        virtual void updateView(PB_SmartKnobState state) = 0;
};
