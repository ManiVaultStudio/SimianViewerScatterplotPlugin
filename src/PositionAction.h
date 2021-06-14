#pragma once

#include "PluginAction.h"

#include <QHBoxLayout>
#include <QLabel>

class PositionAction : public PluginAction
{
protected: // Widget

    class Widget : public PluginAction::Widget {
    public:
        Widget(QWidget* parent, PositionAction* positionAction, const Widget::State& state);
    };

    QWidget* getWidget(QWidget* parent, const Widget::State& state = Widget::State::Standard) override {
        return new Widget(parent, this, state);
    };

public:
    PositionAction(ScatterplotPlugin* scatterplotPlugin);

    QMenu* getContextMenu();

    void setDimensions(const std::uint32_t& numberOfDimensions, const std::vector<QString>& dimensionNames = std::vector<QString>());
    void setDimensions(const std::vector<QString>& dimensionNames);

    std::int32_t getXDimension() const;
    std::int32_t getYDimension() const;

protected:
    hdps::gui::OptionAction     _xDimensionAction;
    hdps::gui::OptionAction     _yDimensionAction;

    friend class Widget;
};