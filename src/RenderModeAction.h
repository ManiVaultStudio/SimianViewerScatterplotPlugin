#pragma once

#include "PluginAction.h"

#include <QActionGroup>
#include <QHBoxLayout>

class QMenu;

class RenderModeAction : public PluginAction
{
protected: // Widget

    class Widget : public PluginAction::Widget {
    public:
        Widget(QWidget* parent, RenderModeAction* renderModeAction, const Widget::State& state);
    };

    QWidget* getWidget(QWidget* parent, const Widget::State& state = Widget::State::Standard) override {
        return new Widget(parent, this, state);
    };

public:
    RenderModeAction(ScatterplotPlugin* scatterplotPlugin);

    QMenu* getContextMenu();

protected:
    hdps::gui::ToggleAction     _scatterPlotAction;
    hdps::gui::ToggleAction     _densityPlotAction;
    hdps::gui::ToggleAction     _contourPlotAction;
    QActionGroup                _actionGroup;

    friend class Widget;
};