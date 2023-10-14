#include "SelectionAction.h"
#include "ScatterplotPlugin.h"
#include "ScatterplotWidget.h"

#include <util/PixelSelectionTool.h>

#include <QHBoxLayout>
#include <QPushButton>

using namespace mv::gui;

SelectionAction::SelectionAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _pixelSelectionAction(this, "Point Selection"),
    _displayModeAction(this, "Display mode", { "Outline", "Override" }),
    _outlineOverrideColorAction(this, "Custom color", true),
    _outlineScaleAction(this, "Scale", 100.0f, 500.0f, 200.0f, 1),
    _outlineOpacityAction(this, "Opacity", 0.0f, 100.0f, 100.0f, 1),
    _outlineHaloEnabledAction(this, "Halo")
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("mouse-pointer"));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);

    addAction(&_pixelSelectionAction.getTypeAction());
    addAction(&_pixelSelectionAction.getBrushRadiusAction());
    addAction(&_pixelSelectionAction.getModifierAction(), OptionAction::HorizontalButtons);
    addAction(&_pixelSelectionAction.getSelectAction());
    addAction(&_pixelSelectionAction.getNotifyDuringSelectionAction());
    addAction(&_pixelSelectionAction.getOverlayColorAction());

    addAction(&getDisplayModeAction());
    addAction(&getOutlineScaleAction());
    addAction(&getOutlineOpacityAction());
    addAction(&getOutlineHaloEnabledAction());

    _pixelSelectionAction.getOverlayColorAction().setText("Color");

    _displayModeAction.setToolTip("The way in which selection is visualized");

    _outlineScaleAction.setSuffix("%");
    _outlineOpacityAction.setSuffix("%");

    const auto updateActionsReadOnly = [this]() -> void {
        const auto isOutline = static_cast<PointSelectionDisplayMode>(_displayModeAction.getCurrentIndex()) == PointSelectionDisplayMode::Outline;

        _outlineScaleAction.setEnabled(isOutline);
        _outlineOpacityAction.setEnabled(isOutline);
        _outlineHaloEnabledAction.setEnabled(isOutline);
    };

    updateActionsReadOnly();

    connect(&_displayModeAction, &OptionAction::currentIndexChanged, this, updateActionsReadOnly);
    connect(&_outlineOverrideColorAction, &ToggleAction::toggled, this, updateActionsReadOnly);
}

void SelectionAction::initialize(ScatterplotPlugin* scatterplotPlugin)
{
    Q_ASSERT(scatterplotPlugin != nullptr);

    if (scatterplotPlugin == nullptr)
        return;

    auto& scatterplotWidget = scatterplotPlugin->getScatterplotWidget();
    getPixelSelectionAction().initialize(&scatterplotWidget, &scatterplotWidget.getPixelSelectionTool(), {
        PixelSelectionType::Rectangle,
        PixelSelectionType::Brush,
        PixelSelectionType::Lasso,
        PixelSelectionType::Polygon
    });

    _displayModeAction.setCurrentIndex(static_cast<std::int32_t>(scatterplotPlugin->getScatterplotWidget().getSelectionDisplayMode()));
    _outlineScaleAction.setValue(100.0f * scatterplotPlugin->getScatterplotWidget().getSelectionOutlineScale());
    _outlineOpacityAction.setValue(100.0f * scatterplotPlugin->getScatterplotWidget().getSelectionOutlineOpacity());

    _outlineHaloEnabledAction.setChecked(scatterplotPlugin->getScatterplotWidget().getSelectionOutlineHaloEnabled());
    _outlineOverrideColorAction.setChecked(scatterplotPlugin->getScatterplotWidget().getSelectionOutlineOverrideColor());

    connect(&_pixelSelectionAction.getSelectAllAction(), &QAction::triggered, [this, scatterplotPlugin]() {
        if (scatterplotPlugin->getPositionDataset().isValid())
            scatterplotPlugin->getPositionDataset()->selectAll();
    });

    connect(&_pixelSelectionAction.getClearSelectionAction(), &QAction::triggered, this, [this, scatterplotPlugin]() {
        if (scatterplotPlugin->getPositionDataset().isValid())
            scatterplotPlugin->getPositionDataset()->selectNone();
    });

    connect(&_pixelSelectionAction.getInvertSelectionAction(), &QAction::triggered, this, [this, scatterplotPlugin]() {
        if (scatterplotPlugin->getPositionDataset().isValid())
            scatterplotPlugin->getPositionDataset()->selectInvert();
    });

    connect(&_outlineScaleAction, &DecimalAction::valueChanged, this, [this, scatterplotPlugin](float value) {
        scatterplotPlugin->getScatterplotWidget().setSelectionOutlineScale(0.01f * value);
    });

    connect(&_outlineOpacityAction, &DecimalAction::valueChanged, this, [this, scatterplotPlugin](float value) {
        scatterplotPlugin->getScatterplotWidget().setSelectionOutlineOpacity(0.01f * value);
    });

    connect(&_outlineHaloEnabledAction, &ToggleAction::toggled, this, [this, scatterplotPlugin](bool toggled) {
        scatterplotPlugin->getScatterplotWidget().setSelectionOutlineHaloEnabled(toggled);
    });

    connect(&_pixelSelectionAction.getOverlayColorAction(), &ColorAction::colorChanged, this, [this, scatterplotPlugin](const QColor& color) {
        scatterplotPlugin->getScatterplotWidget().setSelectionOutlineColor(color);
    });

    connect(&_displayModeAction, &OptionAction::currentIndexChanged, this, [this, scatterplotPlugin](const std::int32_t& currentIndex) {
        scatterplotPlugin->getScatterplotWidget().setSelectionDisplayMode(static_cast<PointSelectionDisplayMode>(currentIndex));
    });

    connect(&_outlineOverrideColorAction, &ToggleAction::toggled, this, [this, scatterplotPlugin](bool toggled) {
        scatterplotPlugin->getScatterplotWidget().setSelectionOutlineOverrideColor(toggled);
    });

    const auto updateReadOnly = [this, scatterplotPlugin]() -> void {
        setEnabled(scatterplotPlugin->getPositionDataset().isValid());
    };

    updateReadOnly();

    connect(&scatterplotPlugin->getPositionDataset(), &Dataset<Points>::changed, this, updateReadOnly);
}

void SelectionAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicSelectionAction = dynamic_cast<SelectionAction*>(publicAction);

    Q_ASSERT(publicSelectionAction != nullptr);

    if (publicSelectionAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_pixelSelectionAction, &publicSelectionAction->getPixelSelectionAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_displayModeAction, &publicSelectionAction->getDisplayModeAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_outlineOverrideColorAction, &publicSelectionAction->getOutlineOverrideColorAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_outlineScaleAction, &publicSelectionAction->getOutlineScaleAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_outlineOpacityAction, &publicSelectionAction->getOutlineOpacityAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_outlineHaloEnabledAction, &publicSelectionAction->getOutlineHaloEnabledAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void SelectionAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_pixelSelectionAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_displayModeAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_outlineOverrideColorAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_outlineScaleAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_outlineOpacityAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_outlineHaloEnabledAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void SelectionAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _pixelSelectionAction.fromParentVariantMap(variantMap);
    _displayModeAction.fromParentVariantMap(variantMap);
    _outlineOverrideColorAction.fromParentVariantMap(variantMap);
    _outlineScaleAction.fromParentVariantMap(variantMap);
    _outlineOpacityAction.fromParentVariantMap(variantMap);
    _outlineHaloEnabledAction.fromParentVariantMap(variantMap);
}

QVariantMap SelectionAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _pixelSelectionAction.insertIntoVariantMap(variantMap);
    _displayModeAction.insertIntoVariantMap(variantMap);
    _outlineOverrideColorAction.insertIntoVariantMap(variantMap);
    _outlineScaleAction.insertIntoVariantMap(variantMap);
    _outlineOpacityAction.insertIntoVariantMap(variantMap);
    _outlineHaloEnabledAction.insertIntoVariantMap(variantMap);

    return variantMap;
}