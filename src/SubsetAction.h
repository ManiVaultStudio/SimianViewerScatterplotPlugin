#pragma once

#include "PluginAction.h"

#include <QHBoxLayout>

class SubsetAction : public PluginAction
{
protected: // Widget

    class Widget : public PluginAction::Widget {
    public:
        Widget(QWidget* parent, SubsetAction* subsetAction, const Widget::State& state);
    };

    QWidget* getWidget(QWidget* parent, const Widget::State& state = Widget::State::Standard) override {
        return new Widget(parent, this, state);
    };

public:
    SubsetAction(ScatterplotPlugin* scatterplotPlugin);

    QMenu* getContextMenu();

protected:
    hdps::gui::StringAction     _subsetNameAction;
    hdps::gui::TriggerAction    _createSubsetAction;
    hdps::gui::OptionAction     _sourceDataAction;

    friend class Widget;
};