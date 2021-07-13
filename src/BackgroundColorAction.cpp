#include "BackgroundColorAction.h"
#include "Application.h"

#include "ScatterplotPlugin.h"
#include "ScatterplotWidget.h"

#include <QHBoxLayout>
#include <QMenu>

using namespace hdps::gui;

const QColor BackgroundColorAction::DEFAULT_COLOR = qRgb(255, 255, 255);

BackgroundColorAction::BackgroundColorAction(ScatterplotPlugin* scatterplotPlugin) :
    PluginAction(scatterplotPlugin, "Coloring"),
    _backgroundColorAction(this, "Background color", DEFAULT_COLOR, DEFAULT_COLOR),
    _resetAction(this, "Reset")
{
    _backgroundColorAction.setToolTip("Background color");
    _resetAction.setToolTip("Reset color settings");

    const auto updateBackgroundColor = [this]() -> void {
        const auto color = _backgroundColorAction.getColor();

        getScatterplotWidget()->setBackgroundColor(color);
    };

    const auto updateResetAction = [this]() -> void {
        _resetAction.setEnabled(getScatterplotWidget()->getBackgroundColor() != DEFAULT_COLOR);
    };

    connect(&_backgroundColorAction, &ColorAction::colorChanged, this, [this, updateBackgroundColor, updateResetAction](const QColor& color) {
        updateBackgroundColor();
        updateResetAction();
    });

    connect(&_resetAction, &QAction::triggered, this, [this, updateResetAction](const QColor& color) {
        _backgroundColorAction.setColor(DEFAULT_COLOR);
    });

    //connect(getScatterplotWidget(), &ScatterplotWidget::coloringModeChanged, this, [this, updateBackgroundColor](const ScatterplotWidget::ColoringMode& coloringMode) {
    //    if (coloringMode == ScatterplotWidget::ColoringMode::ConstantColor)
    //        updateBackgroundColor();
    //});

    //connect(_scatterplotPlugin, &ScatterplotPlugin::currentDatasetChanged, this, [this, updateBackgroundColor](const QString& datasetName) {
    //    if (getScatterplotWidget()->getColoringMode() == ScatterplotWidget::ColoringMode::ConstantColor)
    //        updateBackgroundColor();
    //});

    updateResetAction();
    updateBackgroundColor();
}

QMenu* BackgroundColorAction::getContextMenu()
{
    auto menu = new QMenu("Background color");

    menu->addAction(&_backgroundColorAction);
    menu->addAction(&_resetAction);

    return menu;
}

BackgroundColorAction::Widget::Widget(QWidget* parent, BackgroundColorAction* backgroundColorAction, const Widget::State& state) :
    WidgetAction::Widget(parent, backgroundColorAction, state)
{
    switch (state)
    {
    case Widget::State::Standard:
    case Widget::State::Popup:
    {
        auto layout = new QHBoxLayout();

        layout->setMargin(0);
        layout->addWidget(backgroundColorAction->_backgroundColorAction.createWidget(this));

        setLayout(layout);
        break;
    }

    default:
        break;
    }
}
