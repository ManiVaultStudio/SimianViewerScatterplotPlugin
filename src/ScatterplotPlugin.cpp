#include "ScatterplotPlugin.h"
#include "ScatterplotWidget.h"
#include "DataHierarchyItem.h"
#include "Application.h"

#include "util/PixelSelectionTool.h"
#include "util/Timer.h"

#include "PointData/PointData.h"
#include "ClusterData/ClusterData.h"
#include "ColorData/ColorData.h"

#include "graphics/Vector2f.h"
#include "graphics/Vector3f.h"
#include "widgets/DropWidget.h"

#include <actions/PluginTriggerAction.h>

#include <DatasetsMimeData.h>

#include <QtCore>
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QMetaType>

#include <algorithm>
#include <functional>
#include <limits>
#include <set>
#include <vector>

Q_PLUGIN_METADATA(IID "nl.tudelft.ScatterplotPlugin")

using namespace hdps;
using namespace hdps::util;

ScatterplotPlugin::ScatterplotPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _positionDataset(),
    _positionSourceDataset(),
    _positions(),
    _numPoints(0),
    _scatterPlotWidget(new ScatterplotWidget()),
   // _dropWidget(nullptr),
    _settingsAction(this, "Settings"),
    _primaryToolbarAction(this, "Primary Toolbar"),
    _secondaryToolbarAction(this, "Secondary Toolbar"),
    _selectPointsTimer(),
    _selectedCrossSpeciesCluster(this, "CrossSpeciesclusterSelection"),
    _scatterplotColorControlAction(this, "Scatterplot color")
{
    setObjectName("Scatterplot");

    //_dropWidget = new DropWidget(_scatterPlotWidget);
    if (getFactory()->getNumberOfInstances() == 0) {
        _selectedCrossSpeciesCluster.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
        _scatterplotColorControlAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
        _selectedCrossSpeciesCluster.publish("GlobalSelectedCrossspeciesCluster");
        _scatterplotColorControlAction.publish("GlobalScatterplotColorControl");
    }
    getWidget().setFocusPolicy(Qt::ClickFocus);

    _primaryToolbarAction.addAction(&_settingsAction.getDatasetsAction());
    _primaryToolbarAction.addAction(&_settingsAction.getRenderModeAction(), 3, GroupAction::Horizontal);
    _primaryToolbarAction.addAction(&_settingsAction.getPositionAction(), 1, GroupAction::Horizontal);
    _primaryToolbarAction.addAction(&_settingsAction.getPlotAction(), 2, GroupAction::Horizontal);
    _primaryToolbarAction.addAction(&_settingsAction.getColoringAction());
    _primaryToolbarAction.addAction(&_settingsAction.getSubsetAction());
    _primaryToolbarAction.addAction(&_settingsAction.getClusteringAction());
    _primaryToolbarAction.addAction(&_settingsAction.getSelectionAction());

    _secondaryToolbarAction.addAction(&_settingsAction.getColoringAction().getColorMap1DAction(), 1);

    auto focusSelectionAction = new ToggleAction(this, "Focus selection");

    focusSelectionAction->setIcon(Application::getIconFont("FontAwesome").getIcon("mouse-pointer"));

    connect(focusSelectionAction, &ToggleAction::toggled, this, [this](bool toggled) -> void {
        _settingsAction.getPlotAction().getPointPlotAction().getFocusSelection().setChecked(toggled);
    });

    connect(&_settingsAction.getPlotAction().getPointPlotAction().getFocusSelection(), &ToggleAction::toggled, this, [this, focusSelectionAction](bool toggled) -> void {
        focusSelectionAction->setChecked(toggled);
    });

    const auto updateReadOnly = [this, focusSelectionAction]() {
        focusSelectionAction->setEnabled(getScatterplotWidget().getRenderMode() == ScatterplotWidget::SCATTERPLOT && _positionDataset.isValid());
    };

    updateReadOnly();

    connect(_scatterPlotWidget, &ScatterplotWidget::renderModeChanged, this, updateReadOnly);
    connect(&_positionDataset, &Dataset<Points>::changed, this, updateReadOnly);

    _secondaryToolbarAction.addAction(focusSelectionAction, 2);
    _secondaryToolbarAction.addAction(&_settingsAction.getExportAction());
    _secondaryToolbarAction.addAction(&_settingsAction.getMiscellaneousAction());

    connect(_scatterPlotWidget, &ScatterplotWidget::customContextMenuRequested, this, [this](const QPoint& point) {
        if (!_positionDataset.isValid())
            return;

        auto contextMenu = _settingsAction.getContextMenu();

        contextMenu->addSeparator();

        _positionDataset->populateContextMenu(contextMenu);

        contextMenu->exec(getWidget().mapToGlobal(point));
    });

   // _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag an item from the data hierarchy and drop it here to visualize data..."));
    //_dropWidget->initialize([this](const QMimeData* mimeData) -> DropWidget::DropRegions {
    //    DropWidget::DropRegions dropRegions;

    //    const auto datasetsMimeData = dynamic_cast<const DatasetsMimeData*>(mimeData);

    //    if (datasetsMimeData == nullptr)
    //        return dropRegions;

    //    if (datasetsMimeData->getDatasets().count() > 1)
    //        return dropRegions;

    //    const auto dataset          = datasetsMimeData->getDatasets().first();
    //    const auto datasetGuiName   = dataset->text();
    //    const auto datasetId        = dataset->getId();
    //    const auto dataType         = dataset->getDataType();
    //    const auto dataTypes        = DataTypes({ PointType , ColorType, ClusterType });

    //    // Check if the data type can be dropped
    //    if (!dataTypes.contains(dataType))
    //        dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);

    //    // Points dataset is about to be dropped
    //    if (dataType == PointType) {

    //        // Get points dataset from the core
    //        auto candidateDataset = _core->requestDataset<Points>(datasetId);

    //        // Establish drop region description
    //        const auto description = QString("Visualize %1 as points or density/contour map").arg(datasetGuiName);

    //        if (!_positionDataset.isValid()) {

    //            // Load as point positions when no dataset is currently loaded
    //            dropRegions << new DropWidget::DropRegion(this, "Point position", description, "map-marker-alt", true, [this, candidateDataset]() {
    //                _positionDataset = candidateDataset;
    //                });
    //        }
    //        else {
    //            if (_positionDataset != candidateDataset && candidateDataset->getNumDimensions() >= 2) {

    //                // The number of points is equal, so offer the option to replace the existing points dataset
    //                dropRegions << new DropWidget::DropRegion(this, "Point position", description, "map-marker-alt", true, [this, candidateDataset]() {
    //                    _positionDataset = candidateDataset;
    //                    });
    //            }

    //            if (candidateDataset->getNumPoints() == _positionDataset->getNumPoints()) {

    //                // The number of points is equal, so offer the option to use the points dataset as source for points colors
    //                dropRegions << new DropWidget::DropRegion(this, "Point color", QString("Colorize %1 points with %2").arg(_positionDataset->text(), candidateDataset->text()), "palette", true, [this, candidateDataset]() {
    //                    _settingsAction.getColoringAction().addColorDataset(candidateDataset);
    //                    _settingsAction.getColoringAction().setCurrentColorDataset(candidateDataset);
    //                    });

    //                // The number of points is equal, so offer the option to use the points dataset as source for points size
    //                dropRegions << new DropWidget::DropRegion(this, "Point size", QString("Size %1 points with %2").arg(_positionDataset->text(), candidateDataset->text()), "ruler-horizontal", true, [this, candidateDataset]() {
    //                    _settingsAction.getPlotAction().getPointPlotAction().addPointSizeDataset(candidateDataset);
    //                    _settingsAction.getPlotAction().getPointPlotAction().getSizeAction().setCurrentDataset(candidateDataset);
    //                    });

    //                // The number of points is equal, so offer the option to use the points dataset as source for points opacity
    //                dropRegions << new DropWidget::DropRegion(this, "Point opacity", QString("Set %1 points opacity with %2").arg(_positionDataset->text(), candidateDataset->text()), "brush", true, [this, candidateDataset]() {
    //                    _settingsAction.getPlotAction().getPointPlotAction().addPointOpacityDataset(candidateDataset);
    //                    _settingsAction.getPlotAction().getPointPlotAction().getOpacityAction().setCurrentDataset(candidateDataset);
    //                    });
    //            }
    //        }
    //    }

    //    // Cluster dataset is about to be dropped
    //    if (dataType == ClusterType) {

    //        // Get clusters dataset from the core
    //        auto candidateDataset = _core->requestDataset<Clusters>(datasetId);

    //        // Establish drop region description
    //        const auto description = QString("Color points by %1").arg(candidateDataset->text());

    //        // Only allow user to color by clusters when there is a positions dataset loaded
    //        if (_positionDataset.isValid()) {

    //            if (_settingsAction.getColoringAction().hasColorDataset(candidateDataset)) {

    //                // The clusters dataset is already loaded
    //                dropRegions << new DropWidget::DropRegion(this, "Color", description, "palette", true, [this, candidateDataset]() {
    //                    _settingsAction.getColoringAction().setCurrentColorDataset(candidateDataset);
    //                    });
    //            }
    //            else {

    //                // Use the clusters set for points color
    //                dropRegions << new DropWidget::DropRegion(this, "Color", description, "palette", true, [this, candidateDataset]() {
    //                    _settingsAction.getColoringAction().addColorDataset(candidateDataset);
    //                    _settingsAction.getColoringAction().setCurrentColorDataset(candidateDataset);
    //                    });
    //            }
    //        }
    //        else {

    //            // Only allow user to color by clusters when there is a positions dataset loaded
    //            dropRegions << new DropWidget::DropRegion(this, "No points data loaded", "Clusters can only be visualized in concert with points data", "exclamation-circle", false);
    //        }
    //    }

    //    return dropRegions;
    //});

    /*
    _selectPointsTimer.setSingleShot(true);

    connect(&_selectPointsTimer, &QTimer::timeout, this, [this]() -> void {
        if (_selectPointsTimer.isActive())
            _selectPointsTimer.start(LAZY_UPDATE_INTERVAL);
        else {
            _selectPointsTimer.stop();
            selectPoints();
        }
    });
    */
}

ScatterplotPlugin::~ScatterplotPlugin()
{
}

void ScatterplotPlugin::init()
{
     
    // Get the available screen height and width
    QRect availableScreenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    int screenWidth = availableScreenGeometry.width();
    int screenHeight = availableScreenGeometry.height();

    // Calculate the size of the scatter plot widget and the view based on the available screen width
    int scatterPlotWidth = 0.9 * screenWidth;
    int viewWidth = 0.1 * screenWidth;
    int widgetHeight = 0.9 * screenHeight;

    QGraphicsView* view = new QGraphicsView(&_scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setAlignment(Qt::AlignRight | Qt::AlignTop);
    view->setFrameStyle(QFrame::NoFrame);
    view->setFixedWidth(viewWidth);
    view->setInteractive(true);

    view->setWhatsThis("Cluster colors");
    auto chartLegendLayout = new QHBoxLayout();
    chartLegendLayout->addWidget(_scatterPlotWidget, scatterPlotWidth);
    chartLegendLayout->addWidget(view, viewWidth);
    chartLegendLayout->setContentsMargins(0, 0, 0, 0);
    chartLegendLayout->setSpacing(0);
    chartLegendLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto optionsLayout = new QHBoxLayout();

    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(1);
    optionsLayout->setAlignment(Qt::AlignLeft);
    auto coloringWidget = _settingsAction.getColoringAction().getColorMap1DAction().createWidget(&getWidget());
    coloringWidget->setMaximumWidth(100);
    QLabel* ColorLabel = new QLabel();
    ColorLabel->setText("Color map: ");
    optionsLayout->addWidget(ColorLabel);
    optionsLayout->addWidget(coloringWidget);
    optionsLayout->addWidget((_primaryToolbarAction.createWidget(&getWidget())));
    layout->addLayout(optionsLayout);
    layout->addLayout(chartLegendLayout);


    getWidget().setLayout(layout);

    // Update the data when the scatter plot widget is initialized
    connect(_scatterPlotWidget, &ScatterplotWidget::initialized, this, &ScatterplotPlugin::updateData);

    // Update the selection when the pixel selection tool selected area changed
    connect(&_scatterPlotWidget->getPixelSelectionTool(), &PixelSelectionTool::areaChanged, [this]() {
        if (_scatterPlotWidget->getPixelSelectionTool().isNotifyDuringSelection()) {
            //_selectPointsTimer.start(LAZY_UPDATE_INTERVAL);
            selectPoints();
        }
    });

    // Update the selection when the pixel selection process ended
    connect(&_scatterPlotWidget->getPixelSelectionTool(), &PixelSelectionTool::ended, [this]() {
        if (_scatterPlotWidget->getPixelSelectionTool().isNotifyDuringSelection())
            return;

        //_selectPointsTimer.start(LAZY_UPDATE_INTERVAL);
        selectPoints();
    });

    connect(&_positionDataset, &Dataset<Points>::changed, this, &ScatterplotPlugin::positionDatasetChanged);
    connect(&_positionDataset, &Dataset<Points>::dataChanged, this, &ScatterplotPlugin::updateData);
    connect(&_positionDataset, &Dataset<Points>::dataSelectionChanged, this, &ScatterplotPlugin::updateSelection);
    // Update the window title when the GUI name of the position dataset changes
    connect(&_positionDataset, &Dataset<Points>::guiNameChanged, this, &ScatterplotPlugin::updateWindowTitle);
    // Do an initial update of the window title
    updateWindowTitle();
}

void ScatterplotPlugin::loadData(const Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    // Load the first dataset
    _positionDataset = datasets.first();

    // And set the coloring mode to constant
    _settingsAction.getColoringAction().getColorByAction().setCurrentIndex(0);
}

void ScatterplotPlugin::createSubset(const bool& fromSourceData /*= false*/, const QString& name /*= ""*/)
{
    // Create the subset
    hdps::Dataset<DatasetImpl> subset;

    if (fromSourceData)
        // Make subset from the source data, this is not the displayed data, so no restrictions here
        subset = _positionSourceDataset->createSubsetFromSelection(name, _positionSourceDataset);
    else
        // Avoid making a bigger subset than the current data by restricting the selection to the current data
        subset = _positionDataset->createSubsetFromVisibleSelection(name, _positionDataset);

    // Notify others that the subset was added
    events().notifyDatasetAdded(subset);

    // And select the subset
    subset->getDataHierarchyItem().select();
}

void ScatterplotPlugin::selectPoints()
{
    // Only proceed with a valid points position dataset and when the pixel selection tool is active
    if (!_positionDataset.isValid() || !_scatterPlotWidget->getPixelSelectionTool().isActive())
        return;

    //qDebug() << _positionDataset->getGuiName() << "selectPoints";

    // Get binary selection area image from the pixel selection tool
    auto selectionAreaImage = _scatterPlotWidget->getPixelSelectionTool().getAreaPixmap().toImage();

    // Get smart pointer to the position selection dataset
    auto selectionSet = _positionDataset->getSelection<Points>();

    // Create vector for target selection indices
    std::vector<std::uint32_t> targetSelectionIndices;

    // Reserve space for the indices
    targetSelectionIndices.reserve(_positionDataset->getNumPoints());

    // Mapping from local to global indices
    std::vector<std::uint32_t> localGlobalIndices;

    // Get global indices from the position dataset
    _positionDataset->getGlobalIndices(localGlobalIndices);

    const auto dataBounds = _scatterPlotWidget->getBounds();
    const auto width = selectionAreaImage.width();
    const auto height = selectionAreaImage.height();
    const auto size = width < height ? width : height;

    // Loop over all points and establish whether they are selected or not
    for (std::uint32_t i = 0; i < _positions.size(); i++) {
        const auto uvNormalized = QPointF((_positions[i].x - dataBounds.getLeft()) / dataBounds.getWidth(), (dataBounds.getTop() - _positions[i].y) / dataBounds.getHeight());
        const auto uvOffset     = QPoint((selectionAreaImage.width() - size) / 2.0f, (selectionAreaImage.height() - size) / 2.0f);
        const auto uv           = uvOffset + QPoint(uvNormalized.x() * size, uvNormalized.y() * size);

        // Add point if the corresponding pixel selection is on
        if (selectionAreaImage.pixelColor(uv).alpha() > 0)
            targetSelectionIndices.push_back(localGlobalIndices[i]);
    }

    // Selection should be subtracted when the selection process was aborted by the user (e.g. by pressing the escape key)
    const auto selectionModifier = _scatterPlotWidget->getPixelSelectionTool().isAborted() ? PixelSelectionModifierType::Subtract : _scatterPlotWidget->getPixelSelectionTool().getModifier();

    switch (selectionModifier)
    {
        case PixelSelectionModifierType::Replace:
            break;

        case PixelSelectionModifierType::Add:
        case PixelSelectionModifierType::Subtract:
        {
            // Get reference to the indices of the selection set
            auto& selectionSetIndices = selectionSet->indices;

            // Create a set from the selection set indices
            QSet<std::uint32_t> set(selectionSetIndices.begin(), selectionSetIndices.end());

            switch (selectionModifier)
            {
                // Add points to the current selection
            case PixelSelectionModifierType::Add:
            {
                // Add indices to the set 
                for (const auto& targetIndex : targetSelectionIndices)
                    set.insert(targetIndex);

                break;
            }

            // Remove points from the current selection
            case PixelSelectionModifierType::Subtract:
            {
                // Remove indices from the set 
                for (const auto& targetIndex : targetSelectionIndices)
                    set.remove(targetIndex);

                break;
            }

            default:
                break;
            }

            // Convert set back to vector
            targetSelectionIndices = std::vector<std::uint32_t>(set.begin(), set.end());

            break;
        }

        default:
            break;
    }

    _positionDataset->setSelectionIndices(targetSelectionIndices);

    events().notifyDatasetDataSelectionChanged(_positionDataset->getSourceDataset<Points>());
}

void ScatterplotPlugin::updateWindowTitle()
{
    if (!_positionDataset.isValid())
        getWidget().setWindowTitle(getGuiName());
    else
        getWidget().setWindowTitle(QString("%1: %2").arg(getGuiName(), _positionDataset->getDataHierarchyItem().getSerializationName()));
}

Dataset<Points>& ScatterplotPlugin::getPositionDataset()
{
    return _positionDataset;
}

Dataset<Points>& ScatterplotPlugin::getPositionSourceDataset()
{
    return _positionSourceDataset;
}

void ScatterplotPlugin::positionDatasetChanged()
{
    if (!_positionDataset.isValid())
        return;

    // Reset dataset references
    //_positionSourceDataset.reset();

    // Set position source dataset reference when the position dataset is derived
    //if (_positionDataset->isDerivedData())
    _positionSourceDataset = _positionDataset->getSourceDataset<Points>();

    _numPoints = _positionDataset->getNumPoints();

    _scatterPlotWidget->getPixelSelectionTool().setEnabled(_positionDataset.isValid());

   // _dropWidget->setShowDropIndicator(!_positionDataset.isValid());

    updateData();
    // Update the window title to reflect the position dataset change
    updateWindowTitle();
}

void ScatterplotPlugin::loadColors(const Dataset<Points>& points, const std::uint32_t& dimensionIndex)
{
    // Only proceed with valid points dataset
    if (!points.isValid())
        return;

    // Generate point scalars for color mapping
    std::vector<float> scalars;

    if (_positionDataset->getNumPoints() != _numPoints)
    {
        qWarning("Number of points used for coloring does not match number of points in data, aborting attempt to color plot");
        return;
    }

    // Populate point scalars
    if (dimensionIndex >= 0)
        points->extractDataForDimension(scalars, dimensionIndex);

    // Assign scalars and scalar effect
    _scatterPlotWidget->setScalars(scalars);
    _scatterPlotWidget->setScalarEffect(PointEffect::Color);
    _scene.clear();
    _settingsAction.getColoringAction().updateColorMapActionScalarRange();

    // Render
    getWidget().update();
}

void ScatterplotPlugin::loadColors(const Dataset<Clusters>& clusters)
{
    // Only proceed with valid clusters and position dataset
    if (!clusters.isValid() || !_positionDataset.isValid())
        return;

    // Mapping from local to global indices
    std::vector<std::uint32_t> globalIndices;

    // Get global indices from the position dataset
    int totalNumPoints = 0;
    if (_positionDataset->isDerivedData())
        totalNumPoints = _positionSourceDataset->getFullDataset<Points>()->getNumPoints();
    else
        totalNumPoints = _positionDataset->getFullDataset<Points>()->getNumPoints();

    _positionDataset->getGlobalIndices(globalIndices);

    // Generate color buffer for global and local colors
    std::vector<Vector3f> globalColors(totalNumPoints);
    std::vector<Vector3f> localColors(_positions.size());

    // Loop over all clusters and populate global colors
    for (const auto& cluster : clusters->getClusters())
        for (const auto& index : cluster.getIndices())
            globalColors[index] = Vector3f(cluster.getColor().redF(), cluster.getColor().greenF(), cluster.getColor().blueF());
    std::int32_t localColorIndex = 0;

    // Loop over all global indices and find the corresponding local color
    for (const auto& globalIndex : globalIndices)
        localColors[localColorIndex++] = globalColors[globalIndex];
    _scene.clear();
    updateLegend(clusters);
    // Apply colors to scatter plot widget without modification
    _scatterPlotWidget->setColors(localColors);

    // Render
    getWidget().update();
}

ScatterplotWidget& ScatterplotPlugin::getScatterplotWidget()
{
    return *_scatterPlotWidget;
}

void ScatterplotPlugin::updateLegend(const Dataset<Clusters>& clusters)
{
    if (!clusters.isValid())
        return;
    QGraphicsView* view = _scene.views().first();
    _scene.clear();
    int baseline = 0;
    QGraphicsEllipseItem* ellipseItem;
    QGraphicsTextItem* textItem;
    int clusterIndex = 0;

    for (const auto& cluster : clusters->getClusters())
    {
        QPen pen(Qt::NoPen);
        pen.setWidth(1);
        QBrush brush(Qt::SolidPattern);
        brush.setColor(cluster.getColor());
        ellipseItem = _scene.addEllipse(view->width() - 32, baseline + 5, 10, 10, pen, brush);//right aligned
        //ellipseItem = _scene.addEllipse(20, baseline + 5, 10, 10, pen, brush);//left aligned
        textItem = _scene.addText(cluster.getName(), QFont("Arial", 8));
        textItem->setPos(view->width() - textItem->boundingRect().width() - 34, baseline); //right aligned
        //textItem->setPos(30, baseline);//left aligned
        //textItem->setOpenExternalLinks(true);
        //textItem->setTextInteractionFlags(Qt::TextBrowserInteraction);
        //textItem->setCursor(Qt::PointingHandCursor);
        connect(textItem, &QGraphicsTextItem::linkActivated, this, [=]() {
            qDebug() << "**clicked " << textItem->toPlainText() << " **";
            textClicked(textItem->toPlainText());
            });
        clusterIndex++;
        baseline = clusterIndex * 20;
    }
    int totalHeight = clusters->getClusters().size() * 20;
    view->setSceneRect(0, 0, view->width(), totalHeight);
}
void ScatterplotPlugin::textClicked(QString clickedItem)
{

    if (_selectedCrossSpeciesCluster.getString() != "")
    {
        if (_selectedCrossSpeciesCluster.getString() == clickedItem)
        {
            _selectedCrossSpeciesCluster.setString("");
        }
        else
        {
            _selectedCrossSpeciesCluster.setString(clickedItem);
        }
    }
    _scene.setSceneRect(0, 0, 0, 0);
}
void ScatterplotPlugin::selectTextEllipse()
{
    if (_scatterplotColorControlAction.getCurrentText() == "cross-species cluster")
    {
        changeHighlight();
        scrollToHighlight();
    }
    else if (_scatterplotColorControlAction.getCurrentText() != "expression")
    {
        // Get the first view in the scene
        QGraphicsView* view = _scene.views().first();
        // Scroll to the top of the scene
        view->ensureVisible(0, 0, 0, 0);
    }
    _scene.setSceneRect(0, 0, 0, 0);
}
void ScatterplotPlugin::changeHighlight()
{
    QList<QGraphicsItem*> items = _scene.items();
    foreach(QGraphicsItem * item, items) {
        QGraphicsTextItem* textItem = dynamic_cast<QGraphicsTextItem*>(item);
        if (textItem) {
            if (_selectedCrossSpeciesCluster.getString() == textItem->toPlainText())
            {
                textItem->setDefaultTextColor(_settingsAction.getSelectionAction().getPixelSelectionAction().getOverlayColorAction().getColor());
            }
            else
            {
                textItem->setDefaultTextColor(Qt::black);
            }
        }
    }
    _scene.setSceneRect(0, 0, 0, 0);
}
void ScatterplotPlugin::scrollToHighlight()
{
    QList<QGraphicsItem*> items = _scene.items();
    foreach(QGraphicsItem * item, items) {
        QGraphicsTextItem* textItem = dynamic_cast<QGraphicsTextItem*>(item);
        if (textItem) {
            if (_selectedCrossSpeciesCluster.getString() == textItem->toPlainText())
            {
                // Scroll to the position of the text item
                _scene.views().first()->ensureVisible(textItem);
            }
        }
    }
    _scene.setSceneRect(0, 0, 0, 0);
}
void ScatterplotPlugin::handleSelection(QGraphicsItem* item)
{
    qDebug() << "handleSelection called";
}
void ScatterplotPlugin::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    qDebug() << "Mouse pressed!";
    QGraphicsItem* item = _scene.itemAt(event->scenePos(), QTransform());
    if (item && item->type() == QGraphicsTextItem::Type) {
        handleSelection(item);
    }
}

void ScatterplotPlugin::updateData()
{
    // Check if the scatter plot is initialized, if not, don't do anything
    if (!_scatterPlotWidget->isInitialized())
        return;

    // If no dataset has been selected, don't do anything
    if (_positionDataset.isValid()) {

        // Get the selected dimensions to use as X and Y dimension in the plot
        const auto xDim = _settingsAction.getPositionAction().getDimensionX();
        const auto yDim = _settingsAction.getPositionAction().getDimensionY();

        // If one of the dimensions was not set, do not draw anything
        if (xDim < 0 || yDim < 0)
            return;

        // Ensure that if positionDataset has now more points, the additional points are plotted
        if (_numPoints != _positionDataset->getNumPoints())
        {
            _settingsAction.getPlotAction().getPointPlotAction().updateScatterPlotWidgetPointSizeScalars();
            _settingsAction.getPlotAction().getPointPlotAction().updateScatterPlotWidgetPointOpacityScalars();
        }

        // Determine number of points depending on if its a full dataset or a subset
        _numPoints = _positionDataset->getNumPoints();

        // Extract 2-dimensional points from the data set based on the selected dimensions
        calculatePositions(*_positionDataset);

        // Pass the 2D points to the scatter plot widget
        _scatterPlotWidget->setData(&_positions);

        updateSelection();
    }
    else {
        _positions.clear();
        _scatterPlotWidget->setData(&_positions);
    }
}

void ScatterplotPlugin::calculatePositions(const Points& points)
{
    points.extractDataForDimensions(_positions, _settingsAction.getPositionAction().getDimensionX(), _settingsAction.getPositionAction().getDimensionY());
}

void ScatterplotPlugin::updateSelection()
{
    if (!_positionDataset.isValid())
        return;

    //Timer timer(__FUNCTION__);

    auto selection = _positionDataset->getSelection<Points>();

    std::vector<bool> selected;
    std::vector<char> highlights;

    _positionDataset->selectedLocalIndices(selection->indices, selected);

    highlights.resize(_positionDataset->getNumPoints(), 0);

    for (int i = 0; i < selected.size(); i++)
        highlights[i] = selected[i] ? 1 : 0;

    _scatterPlotWidget->setHighlights(highlights, static_cast<std::int32_t>(selection->indices.size()));
}

void ScatterplotPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    ViewPlugin::fromVariantMap(variantMap);

    variantMapMustContain(variantMap, "Settings");

    _primaryToolbarAction.fromParentVariantMap(variantMap);
    _secondaryToolbarAction.fromParentVariantMap(variantMap);
    _settingsAction.fromVariantMap(variantMap["Settings"].toMap());
}

QVariantMap ScatterplotPlugin::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();

    _primaryToolbarAction.insertIntoVariantMap(variantMap);
    _secondaryToolbarAction.insertIntoVariantMap(variantMap);
    _settingsAction.insertIntoVariantMap(variantMap);

    return variantMap;
}

std::uint32_t ScatterplotPlugin::getNumberOfPoints() const
{
    if (!_positionDataset.isValid())
        return 0;

    return _positionDataset->getNumPoints();
}

void ScatterplotPlugin::setXDimension(const std::int32_t& dimensionIndex)
{
    updateData();
}

void ScatterplotPlugin::setYDimension(const std::int32_t& dimensionIndex)
{
    updateData();
}

QIcon ScatterplotPluginFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return Application::getIconFont("FontAwesome").getIcon("braille", color);
}

ViewPlugin* ScatterplotPluginFactory::produce()
{
    return new ScatterplotPlugin(this);
}

PluginTriggerActions ScatterplotPluginFactory::getPluginTriggerActions(const hdps::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getInstance = [this]() -> ScatterplotPlugin* {
        return dynamic_cast<ScatterplotPlugin*>(Application::core()->getPluginManager().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto& fontAwesome = Application::getIconFont("FontAwesome");

        if (numberOfDatasets >= 1) {
            auto pluginTriggerAction = new PluginTriggerAction(const_cast<ScatterplotPluginFactory*>(this), this, "Scatterplot", "View selected datasets side-by-side in separate scatter plot viewers", fontAwesome.getIcon("braille"), [this, getInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
                for (auto dataset : datasets)
                    getInstance()->loadData(Datasets({ dataset }));
            });

            pluginTriggerActions << pluginTriggerAction;
        }
    }

    /*
    const auto numberOfPointsDatasets   = PluginFactory::getNumberOfDatasetsForType(datasets, PointType);
    const auto numberOfClusterDatasets  = PluginFactory::getNumberOfDatasetsForType(datasets, ClusterType);

    if (numberOfPointsDatasets == numberOfClusterDatasets) {
        QRegularExpression re("(Points, Clusters)");

        const auto reMatch = re.match(PluginFactory::getDatasetTypesAsStringList(datasets).join(","));

        if (reMatch.hasMatch() && reMatch.captured().count() == numberOfPointsDatasets) {
            auto pluginTriggerAction = createPluginTriggerAction("Scatterplot", "Load points dataset in separate viewer and apply cluster", datasets, "braille");

            connect(pluginTriggerAction, &QAction::triggered, [this, getInstance, datasets, numberOfPointsDatasets]() -> void {

                for (int i = 0; i < numberOfPointsDatasets; i++) {
                    getInstance()->loadData(Datasets({ datasets[i * 2] }));
                    getInstance()->loadColors(Dataset<Clusters>(datasets[i * 2 + 1]));
                }
            });

            pluginTriggerActions << pluginTriggerAction;
        }
    }
    */

    return pluginTriggerActions;
}