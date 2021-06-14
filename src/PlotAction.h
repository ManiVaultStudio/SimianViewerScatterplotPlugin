#pragma once

#include "PluginAction.h"
#include "PointPlotAction.h"
#include "DensityPlotAction.h"

class PlotAction : public PluginAction
{
protected: // Widget

    class Widget : public PluginAction::Widget {
    public:
        Widget(QWidget* parent, PlotAction* plotAction, const Widget::State& state);
    };

    QWidget* getWidget(QWidget* parent, const Widget::State& state = Widget::State::Standard) override {
        return new Widget(parent, this, state);
    };

public:
    PlotAction(ScatterplotPlugin* scatterplotPlugin);

    QMenu* getContextMenu();

protected:
    PointPlotAction     _pointPlotAction;
    DensityPlotAction   _densityPlotAction;

    friend class Widget;
};