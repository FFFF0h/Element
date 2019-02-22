#pragma once

#include "gui/GraphEditorView.h"
#include "gui/workspace/ContentViewPanel.h"

namespace Element {

class GraphEditorPanel : public ContentViewPanel<GraphEditorView>
{
public:
    GraphEditorPanel() { setName ("Graph Editor"); }
    ~GraphEditorPanel() = default;
};

}