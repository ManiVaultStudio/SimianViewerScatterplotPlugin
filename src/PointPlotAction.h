#pragma once

#include "PluginAction.h"

#include <QLabel>

class PointPlotAction : public PluginAction
{
protected: // Widget

    class Widget : public PluginAction::Widget {
    public:
        Widget(QWidget* parent, PointPlotAction* pointPlotAction, const Widget::State& state);
    };

    QWidget* getWidget(QWidget* parent, const Widget::State& state = Widget::State::Standard) override {
        return new Widget(parent, this, state);
    };

public:
    PointPlotAction(ScatterplotPlugin* scatterplotPlugin);

    QMenu* getContextMenu();

protected:
    hdps::gui::DecimalAction    _pointSizeAction;
    hdps::gui::DecimalAction    _pointOpacityAction;

    static constexpr double DEFAULT_POINT_SIZE = 10.0;
    static constexpr double DEFAULT_POINT_OPACITY = 50.0;

    friend class Widget;
    friend class PlotAction;
};