#pragma once

#include "PluginAction.h"

#include <QLabel>

class DensityPlotAction : public PluginAction
{
protected:
    class Widget : public PluginAction::Widget {
    public:
        Widget(QWidget* parent, DensityPlotAction* densityPlotAction, const Widget::State& state);
    };

    QWidget* getWidget(QWidget* parent, const Widget::State& state = Widget::State::Standard) override {
        return new Widget(parent, this, state);
    };

public:
    DensityPlotAction(ScatterplotPlugin* scatterplotPlugin);

    QMenu* getContextMenu();

protected:
    hdps::gui::DecimalAction    _sigmaAction;

    static constexpr double DEFAULT_SIGMA = 25.0;

    friend class Widget;
    friend class PlotAction;
};