#pragma once

#include "PluginAction.h"

class ConstantColorAction : public PluginAction
{
protected:
    class Widget : public PluginAction::Widget {
    public:
        Widget(QWidget* parent, ConstantColorAction* colorByConstantAction, const Widget::State& state);
    };

    QWidget* getWidget(QWidget* parent, const Widget::State& state = Widget::State::Standard) override {
        return new Widget(parent, this, state);
    };

public:
    ConstantColorAction(ScatterplotPlugin* scatterplotPlugin);

    QMenu* getContextMenu();

protected:
    hdps::gui::ColorAction      _constantColorAction;
    hdps::gui::TriggerAction    _resetAction;

    static const QColor DEFAULT_COLOR;

    friend class Widget;
};