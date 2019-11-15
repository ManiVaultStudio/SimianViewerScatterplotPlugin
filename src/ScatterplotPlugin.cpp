#include "ScatterplotPlugin.h"

#include "ScatterplotSettings.h"
#include "PointData.h"
#include "ClusterData.h"

#include "graphics/Vector2f.h"
#include "graphics/Vector3f.h"

#include <QtCore>
#include <QtDebug>

#include <algorithm>
#include <limits>
#include <float.h>

Q_PLUGIN_METADATA(IID "nl.tudelft.ScatterplotPlugin")

using namespace hdps;

// =============================================================================
// View
// =============================================================================

ScatterplotPlugin::~ScatterplotPlugin(void)
{
    
}

void ScatterplotPlugin::init()
{
    _dataSlot = new DataSlot(QStringList("Points"));

    _scatterPlotWidget = new ScatterplotWidget();
    _scatterPlotWidget->setAlpha(0.5f);
    _scatterPlotWidget->addSelectionListener(this);

    _scatterPlotWidget->setRenderMode(ScatterplotWidget::RenderMode::SCATTERPLOT);
    _dataSlot->addWidget(_scatterPlotWidget);

    settings = new ScatterplotSettings(this);

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);

    setMainLayout(layout);
    addWidget(_dataSlot);
    addWidget(settings);

    connect(_dataSlot, &DataSlot::onDataInput, this, &ScatterplotPlugin::onDataInput);
    connect(_scatterPlotWidget, &ScatterplotWidget::initialized, this, &ScatterplotPlugin::updateData);
}
 
void ScatterplotPlugin::dataAdded(const QString name)
{
    
}

void ScatterplotPlugin::dataChanged(const QString name)
{
    if (name != _currentDataSet) {
        return;
    }
    updateData();
}

void ScatterplotPlugin::dataRemoved(const QString name)
{
    
}

void ScatterplotPlugin::selectionChanged(const QString dataName)
{
    const hdps::IndexSet& dataSet = _core->requestSet<hdps::IndexSet>(_currentDataSet);
    const auto& data = dataSet.getData<PointData>();

    if (data.isDerivedData())
    {
        if (dataName != data.getSourceData().getName())
            return;
    }
    else
    {
        if (dataName != data.getName())
            return;
    }

    updateSelection();
}

QStringList ScatterplotPlugin::supportedDataKinds()
{
    QStringList supportedKinds;
    supportedKinds << "Points";
    return supportedKinds;
}

void ScatterplotPlugin::dataSetPicked(const QString& name)
{
    const hdps::IndexSet& dataSet = _core->requestSet<hdps::IndexSet>(_currentDataSet);
    const PointData& points = dataSet.getData<PointData>();

    if (points.isDerivedData()) {
        const PointData& sourceData = dynamic_cast<const PointData&>(points.getSourceData());

        if (points.getDimensionNames().size() == points.getNumDimensions())
            settings->initDimOptions(points.getDimensionNames());
        else
            settings->initDimOptions(points.getNumDimensions());

        if (sourceData.getDimensionNames().size() == sourceData.getNumDimensions())
            settings->initScalarDimOptions(sourceData.getDimensionNames());
        else
            settings->initScalarDimOptions(sourceData.getNumDimensions());
    }
    else
    {
        if (points.getDimensionNames().size() == points.getNumDimensions()) {
            settings->initDimOptions(points.getDimensionNames());
            settings->initScalarDimOptions(points.getDimensionNames());
        }
        else {
            settings->initDimOptions(points.getNumDimensions());
            settings->initScalarDimOptions(points.getNumDimensions());
        }
    }

    updateData();
}

void ScatterplotPlugin::subsetCreated()
{
    const hdps::Set& set = _core->requestSet(_currentDataSet);
    const hdps::Set& selection = _core->requestSelection(set.getDataName());
    
    _core->createSubsetFromSelection(selection, set.getDataName(), "Subset");
}

void ScatterplotPlugin::xDimPicked(int index)
{
    updateData();
}

void ScatterplotPlugin::yDimPicked(int index)
{
    updateData();
}

void ScatterplotPlugin::cDimPicked(int index)
{
    updateData();
}

void ScatterplotPlugin::onDataInput(QString setName)
{
    _currentDataSet = setName;

    // Move this code to supportsSet in DataSlot
    const hdps::Set& set = _core->requestSet(setName);
    const plugin::RawData& rawData = _core->requestData(set.getDataName());

    bool supported = supportedDataKinds().contains(rawData.getKind());
    //

    if (!supported)
        return;

    const hdps::IndexSet& dataSet = _core->requestSet<hdps::IndexSet>(_currentDataSet);
    const PointData& points = dataSet.getData<PointData>();

    if (points.isDerivedData()) {
        const PointData& sourceData = dynamic_cast<const PointData&>(points.getSourceData());

        if (points.getDimensionNames().size() == points.getNumDimensions())
            settings->initDimOptions(points.getDimensionNames());
        else
            settings->initDimOptions(points.getNumDimensions());

        if (sourceData.getDimensionNames().size() == sourceData.getNumDimensions())
            settings->initScalarDimOptions(sourceData.getDimensionNames());
        else
            settings->initScalarDimOptions(sourceData.getNumDimensions());
    }
    else
    {
        if (points.getDimensionNames().size() == points.getNumDimensions()) {
            settings->initDimOptions(points.getDimensionNames());
            settings->initScalarDimOptions(points.getDimensionNames());
        }
        else {
            settings->initDimOptions(points.getNumDimensions());
            settings->initScalarDimOptions(points.getNumDimensions());
        }
    }

    updateData();
}

void ScatterplotPlugin::onColorDataInput(QString setName)
{
    const IndexSet& set = _core->requestSet<IndexSet>(setName);
    const RawData& rawData = set.getData<RawData>();
    QString kind = rawData.getKind();

    if (kind == "Points")
    {
        PointData& pointData = set.getData<PointData>();

        std::vector<float> scalars;
        if (pointData.getNumPoints() == _numPoints)
        {
            scalars.insert(scalars.begin(), pointData.getData().begin(), pointData.getData().end());
        }

        _scatterPlotWidget->setScalars(scalars);
        updateData();
    }
    if (kind == "Cluster")
    {
        ClusterData& clusterData = set.getData<ClusterData>();

        std::vector<Vector3f> colors(_points.size());
        for (const Cluster& cluster : clusterData.clusters)
        {
            for (const int& index : cluster.indices)
            {
                colors[index] = Vector3f(cluster._color.redF(), cluster._color.greenF(), cluster._color.blueF());
            }
        }

        _scatterPlotWidget->setColors(colors);
        updateData();
    }
}

void ScatterplotPlugin::updateData()
{
    // Check if the scatter plot is initialized, if not, don't do anything
    if (!_scatterPlotWidget->isInitialized())
        return;

    // If no dataset has been selected, don't do anything
    if (_currentDataSet.isEmpty())
        return;

    // Get the dataset and point data belonging to the currently selected dataset
    const hdps::IndexSet& dataSet = _core->requestSet<hdps::IndexSet>(_currentDataSet);
    const PointData& points = dataSet.getData<PointData>();

    // Get the selected dimensions to use as X and Y dimension in the plot
    int xDim = settings->getXDimension();
    int yDim = settings->getYDimension();

    // If one of the dimensions was not set, do not draw anything
    if (xDim < 0 || yDim < 0)
        return;

    // Determine number of points depending on if its a full dataset or a subset
    _numPoints = dataSet.isFull() ? points.getNumPoints() : dataSet.indices.size();

    // Extract 2-dimensional points from the data set based on the selected dimensions
    calculatePositions(dataSet);

    // Pass the 2D points to the scatter plot widget
    _scatterPlotWidget->setData(&_points);

    //updateSelection();
}

void ScatterplotPlugin::calculatePositions(const hdps::IndexSet& dataSet)
{
    const PointData& points = dataSet.getData<PointData>();

    int numDims = points.getNumDimensions();
    int xDim = settings->getXDimension();
    int yDim = settings->getYDimension();

    _points.resize(_numPoints);

    if (dataSet.isFull())
    {
        for (int i = 0; i < _numPoints; i++)
        {
            _points[i].set(points[i * numDims + xDim], points[i * numDims + yDim]);
        }
    }
    else
    {
        for (int i = 0; i < _numPoints; i++)
        {
            int setIndex = dataSet.indices[i];
            _points[i].set(points[setIndex * numDims + xDim], points[setIndex * numDims + yDim]);
        }
    }
}

void ScatterplotPlugin::calculateScalars(std::vector<float>& scalars, const PointData& points)
{
    int colorIndex = settings->getColorDimension();

    if (colorIndex >= 0) {
        int numDims = points.getNumDimensions();
        float minScalar = FLT_MAX, maxScalar = -FLT_MAX;

        for (int i = 0; i < _numPoints; i++) {
            float scalar = points[i * numDims + colorIndex];

            minScalar = scalar < minScalar ? scalar : minScalar;
            maxScalar = scalar > maxScalar ? scalar : maxScalar;
            scalars[i] = scalar;
        }
        // Normalize the scalars
        float scalarRange = maxScalar - minScalar;
        for (int i = 0; i < scalars.size(); i++) {
            scalars[i] = (scalars[i] - minScalar) / scalarRange;
        }
    }
}

void ScatterplotPlugin::updateSelection()
{
    const hdps::IndexSet& dataSet = _core->requestSet<hdps::IndexSet>(_currentDataSet);
    const RawData& data = _core->requestData(dataSet.getDataName());
    const hdps::IndexSet& selection = dynamic_cast<const hdps::IndexSet&>(data.getSelection());

    std::vector<char> highlights;
    highlights.resize(_numPoints, 0);

    if (dataSet.isFull())
    {
        for (unsigned int index : selection.indices)
        {
            highlights[index] = 1;
        }
    }
    else
    {
        for (int i = 0; i < _numPoints; i++)
        {
            unsigned int index = dataSet.indices[i];
            for (unsigned int selectionIndex : selection.indices)
            {
                if (index == selectionIndex) {
                    highlights[i] = 1;
                    break;
                }
            }
        }
    }

    _scatterPlotWidget->setHighlights(highlights);
}

void ScatterplotPlugin::makeSelection(hdps::Selection selection)
{
    if (_currentDataSet.isEmpty())
        return;

    Selection s = _scatterPlotWidget->getSelection();

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < _points.size(); i++)
    {
        const hdps::Vector2f& point = _points[i];

        if (s.contains(point))
            indices.push_back(i);
    }

    const hdps::IndexSet& set = _core->requestSet<hdps::IndexSet>(_currentDataSet);
    const PointData& data = set.getData<PointData>();

    hdps::IndexSet& selectionSet = dynamic_cast<hdps::IndexSet&>(data.getSelection());

    selectionSet.indices.clear();
    selectionSet.indices.reserve(indices.size());

    for (const unsigned int& index : indices) {
        selectionSet.indices.push_back(set.isFull() ? index : set.indices[index]);
    }

    _core->notifySelectionChanged(selectionSet.getDataName());
}

void ScatterplotPlugin::onSelecting(hdps::Selection selection)
{
    makeSelection(selection);
}

void ScatterplotPlugin::onSelection(hdps::Selection selection)
{
    makeSelection(selection);
}

// =============================================================================
// Factory
// =============================================================================

ViewPlugin* ScatterplotPluginFactory::produce()
{
    return new ScatterplotPlugin();
}
