#include "ScatterplotWidget.h"
#include "Application.h"

#include "util/PixelSelectionTool.h"
#include "util/Math.h"
#include "util/Exception.h"

#include <vector>

#include <QSize>
#include <QPainter>
#include <QDebug>
#include <QOpenGLFramebufferObject>
#include <QWindow>

#include <math.h>

namespace
{
    Bounds getDataBounds(const std::vector<Vector2f>& points)
    {
        Bounds bounds = Bounds::Max;

        for (const Vector2f& point : points)
        {
            bounds.setLeft(std::min(point.x, bounds.getLeft()));
            bounds.setRight(std::max(point.x, bounds.getRight()));
            bounds.setBottom(std::min(point.y, bounds.getBottom()));
            bounds.setTop(std::max(point.y, bounds.getTop()));
        }

        return bounds;
    }
}

ScatterplotWidget::ScatterplotWidget() :
    _densityRenderer(DensityRenderer::RenderMode::DENSITY),
    _backgroundColor(1, 1, 1),
    _pointRenderer(),
    _pixelSelectionTool(this),
    _pixelRatio(1.0)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    _pointRenderer.setPointScaling(Absolute);

    // Configure pixel selection tool
    _pixelSelectionTool.setEnabled(true);
    _pixelSelectionTool.setMainColor(QColor(Qt::black));
    
    QObject::connect(&_pixelSelectionTool, &PixelSelectionTool::shapeChanged, [this]() {
        if (isInitialized())
            update();
    });

    QSurfaceFormat surfaceFormat;

    surfaceFormat.setRenderableType(QSurfaceFormat::OpenGL);

    // Ask for an different OpenGL versions depending on OS
#if defined(__APPLE__) 
    surfaceFormat.setVersion(3, 3); // https://support.apple.com/en-us/101525
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
#elif defined(__linux__ )
    surfaceFormat.setVersion(4, 2); // glxinfo | grep "OpenGL version"
    surfaceFormat.setProfile(QSurfaceFormat::CompatibilityProfile);
#else
    surfaceFormat.setVersion(4, 3);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
#endif

#ifdef _DEBUG
    surfaceFormat.setOption(QSurfaceFormat::DebugContext);
#endif

    surfaceFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    surfaceFormat.setSamples(16);

    setFormat(surfaceFormat);
    
    // we connect screenChanged to updating the pixel ratio
    // this is necessary in case the window is moved between hi and low dpi screens
    // e.g., from a laptop display to a projector
    winId(); // This is needed to produce a valid windowHandle
    QObject::connect(windowHandle(), &QWindow::screenChanged, this, &ScatterplotWidget::updatePixelRatio);
}

bool ScatterplotWidget::isInitialized()
{
    return _isInitialized;
}

ScatterplotWidget::RenderMode ScatterplotWidget::getRenderMode() const
{
    return _renderMode;
}

void ScatterplotWidget::setRenderMode(const RenderMode& renderMode)
{
    if (renderMode == _renderMode)
        return;

    _renderMode = renderMode;

    emit renderModeChanged(_renderMode);

    switch (_renderMode)
    {
        case ScatterplotWidget::SCATTERPLOT:
            break;
        
        case ScatterplotWidget::DENSITY:
            computeDensity();
            break;

        case ScatterplotWidget::LANDSCAPE:
            computeDensity();
            break;

        default:
            break;
    }

    update();
}

ScatterplotWidget::ColoringMode ScatterplotWidget::getColoringMode() const
{
    return _coloringMode;
}

void ScatterplotWidget::setColoringMode(const ColoringMode& coloringMode)
{
    if (coloringMode == _coloringMode)
        return;

    _coloringMode = coloringMode;

    emit coloringModeChanged(_coloringMode);
}

PixelSelectionTool& ScatterplotWidget::getPixelSelectionTool()
{
    return _pixelSelectionTool;
}

void ScatterplotWidget::computeDensity()
{
    emit densityComputationStarted();

    _densityRenderer.computeDensity();

    emit densityComputationEnded();

    update();
}

// Positions need to be passed as a pointer as we need to store them locally in order
// to be able to find the subset of data that's part of a selection. If passed
// by reference then we can upload the data to the GPU, but not store it in the widget.
void ScatterplotWidget::setData(const std::vector<Vector2f>* points)
{
    auto dataBounds = getDataBounds(*points);

    dataBounds.ensureMinimumSize(1e-07f, 1e-07f);
    dataBounds.makeSquare();
    dataBounds.expand(0.1f);

    _dataBounds = dataBounds;

    // Pass bounds and data to renderers
    _pointRenderer.setBounds(_dataBounds);
    _densityRenderer.setBounds(_dataBounds);

    _pointRenderer.setData(*points);
    _densityRenderer.setData(points);

    switch (_renderMode)
    {
        case ScatterplotWidget::SCATTERPLOT:
            break;
        
        case ScatterplotWidget::DENSITY:
        case ScatterplotWidget::LANDSCAPE:
        {
            _densityRenderer.computeDensity();
            break;
        }

        default:
            break;
    }
   // _pointRenderer.setSelectionOutlineColor(Vector3f(1, 0, 0));

    update();
}

QColor ScatterplotWidget::getBackgroundColor()
{
    return _backgroundColor;
}

void ScatterplotWidget::setBackgroundColor(QColor color)
{
    _backgroundColor = color;

    update();
}

void ScatterplotWidget::setHighlights(const std::vector<char>& highlights, const std::int32_t& numSelectedPoints)
{
    _pointRenderer.setHighlights(highlights, numSelectedPoints);

    update();
}

void ScatterplotWidget::setScalars(const std::vector<float>& scalars)
{
    _pointRenderer.setColorChannelScalars(scalars);
    
    update();
}

void ScatterplotWidget::setColors(const std::vector<Vector3f>& colors)
{
    _pointRenderer.setColors(colors);
    _pointRenderer.setScalarEffect(None);

    update();
}

void ScatterplotWidget::setPointSizeScalars(const std::vector<float>& pointSizeScalars)
{
    _pointRenderer.setSizeChannelScalars(pointSizeScalars);
    _pointRenderer.setPointSize(*std::max_element(pointSizeScalars.begin(), pointSizeScalars.end()));

    update();
}

void ScatterplotWidget::setPointOpacityScalars(const std::vector<float>& pointOpacityScalars)
{
    _pointRenderer.setOpacityChannelScalars(pointOpacityScalars);

    update();
}

void ScatterplotWidget::setPointScaling(mv::gui::PointScaling scalingMode)
{
    _pointRenderer.setPointScaling(scalingMode);

    update();
}

void ScatterplotWidget::setScalarEffect(PointEffect effect)
{
    _pointRenderer.setScalarEffect(effect);

    update();
}

void ScatterplotWidget::setSigma(const float sigma)
{
    _densityRenderer.setSigma(sigma);

    update();
}

mv::Vector3f ScatterplotWidget::getColorMapRange() const
{
    switch (_renderMode) {
        case SCATTERPLOT:
            return _pointRenderer.getColorMapRange();

        case LANDSCAPE:
            return _densityRenderer.getColorMapRange();

        default:
            break;
    }
    
    return Vector3f();
}

void ScatterplotWidget::setColorMapRange(const float& min, const float& max)
{
    switch (_renderMode) {
        case SCATTERPLOT:
        {
            _pointRenderer.setColorMapRange(min, max);
            break;
        }

        case LANDSCAPE:
        {
            _densityRenderer.setColorMapRange(min, max);
            break;
        }

        default:
            break;
    }

    update();
}

void ScatterplotWidget::showHighlights(bool show)
{
    _pointRenderer.setSelectionOutlineScale(show ? 0.5f : 0);
    update();
}

void ScatterplotWidget::createScreenshot(std::int32_t width, std::int32_t height, const QString& fileName, const QColor& backgroundColor)
{
    // Exit if the viewer is not initialized
    if (!_isInitialized)
        return;

    // Exit prematurely if the file name is invalid
    if (fileName.isEmpty())
        return;

    makeCurrent();

    try {

        // Use custom FBO format
        QOpenGLFramebufferObjectFormat fboFormat;
        
        fboFormat.setTextureTarget(GL_TEXTURE_2D);
        fboFormat.setInternalTextureFormat(GL_RGB);

        QOpenGLFramebufferObject fbo(width, height, fboFormat);

        // Bind the FBO and render into it when successfully bound
        if (fbo.bind()) {

            // Clear the widget to the background color
            glClearColor(backgroundColor.redF(), backgroundColor.greenF(), backgroundColor.blueF(), backgroundColor.alphaF());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Reset the blending function
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Resize OpenGL to intended screenshot size
            resizeGL(width, height);

            switch (_renderMode)
            {
                case SCATTERPLOT:
                {
                    _pointRenderer.setPointScaling(Relative);
                    _pointRenderer.render();
                    _pointRenderer.setPointScaling(Absolute);

                    break;
                }

                case DENSITY:
                case LANDSCAPE:
                    _densityRenderer.setRenderMode(_renderMode == DENSITY ? DensityRenderer::DENSITY : DensityRenderer::LANDSCAPE);
                    _densityRenderer.render();
                    break;
            }

            // Save FBO image to disk
            //fbo.toImage(false, QImage::Format_RGB32).convertToFormat(QImage::Format_RGB32).save(fileName);
            //fbo.toImage(false, QImage::Format_ARGB32).save(fileName);

            QImage fboImage(fbo.toImage());
            QImage image(fboImage.constBits(), fboImage.width(), fboImage.height(), QImage::Format_ARGB32);

            image.save(fileName);

            // Resize OpenGL back to original OpenGL widget size
            resizeGL(this->width(), this->height());

            fbo.release();
        }
    }
    catch (std::exception& e)
    {
        exceptionMessageBox("Rendering failed", e);
    }
    catch (...) {
        exceptionMessageBox("Rendering failed");
    }
}

PointSelectionDisplayMode ScatterplotWidget::getSelectionDisplayMode() const
{
    return _pointRenderer.getSelectionDisplayMode();
}

void ScatterplotWidget::setSelectionDisplayMode(PointSelectionDisplayMode selectionDisplayMode)
{
    _pointRenderer.setSelectionDisplayMode(selectionDisplayMode);

    update();
}

QColor ScatterplotWidget::getSelectionOutlineColor() const
{
    QColor haloColor;

    haloColor.setRedF(_pointRenderer.getSelectionOutlineColor().x);
    haloColor.setGreenF(_pointRenderer.getSelectionOutlineColor().y);
    haloColor.setBlueF(_pointRenderer.getSelectionOutlineColor().z);

    return haloColor;
}

void ScatterplotWidget::setSelectionOutlineColor(const QColor& selectionOutlineColor)
{
    _pointRenderer.setSelectionOutlineColor(Vector3f(selectionOutlineColor.redF(), selectionOutlineColor.greenF(), selectionOutlineColor.blueF()));

   update();
}

bool ScatterplotWidget::getSelectionOutlineOverrideColor() const
{
    return _pointRenderer.getSelectionOutlineOverrideColor();
}

void ScatterplotWidget::setSelectionOutlineOverrideColor(bool selectionOutlineOverrideColor)
{
    _pointRenderer.setSelectionOutlineOverrideColor(selectionOutlineOverrideColor);

    update();
}

float ScatterplotWidget::getSelectionOutlineScale() const
{
    return _pointRenderer.getSelectionOutlineScale();
}

void ScatterplotWidget::setSelectionOutlineScale(float selectionOutlineScale)
{
    _pointRenderer.setSelectionOutlineScale(selectionOutlineScale);

    update();
}

float ScatterplotWidget::getSelectionOutlineOpacity() const
{
    return _pointRenderer.getSelectionOutlineOpacity();
}

void ScatterplotWidget::setSelectionOutlineOpacity(float selectionOutlineOpacity)
{
    _pointRenderer.setSelectionOutlineOpacity(selectionOutlineOpacity);

    update();
}

bool ScatterplotWidget::getSelectionOutlineHaloEnabled() const
{
    return _pointRenderer.getSelectionHaloEnabled();
}

void ScatterplotWidget::setSelectionOutlineHaloEnabled(bool selectionOutlineHaloEnabled)
{
    _pointRenderer.setSelectionHaloEnabled(selectionOutlineHaloEnabled);

    update();
}

void ScatterplotWidget::initializeGL()
{
    initializeOpenGLFunctions();

#ifdef SCATTER_PLOT_WIDGET_VERBOSE
    qDebug() << "Initializing scatterplot widget with context: " << context();

    std::string versionString = std::string((const char*) glGetString(GL_VERSION));

    qDebug() << versionString.c_str();
#endif

    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ScatterplotWidget::cleanup);

    // Initialize renderers
    _pointRenderer.init();
    _densityRenderer.init();

    // Set a default color map for both renderers
    _pointRenderer.setScalarEffect(PointEffect::Color);

    _pointRenderer.setSelectionOutlineColor(Vector3f(1, 0, 0));

    // OpenGL is initialized
    _isInitialized = true;

    // Initialize the point and density renderer with a color map
    setColorMap(_colorMapImage);

    emit initialized();
}

void ScatterplotWidget::resizeGL(int w, int h)
{
    // we need this here as we do not have the screen yet to get the actual devicePixelRatio when the view is created
    _pixelRatio = devicePixelRatio();
    
    // Pixelration tells us how many pixels map to a point
    // That is needed as macOS calculates in points and we do in pixels
    // On macOS high dpi displays pixel ration is 2
    w *= _pixelRatio;
    h *= _pixelRatio;
    
    _windowSize.setWidth(w);
    _windowSize.setHeight(h);

    _pointRenderer.resize(QSize(w, h));
    _densityRenderer.resize(QSize(w, h));

    // Set matrix for normalizing from pixel coordinates to [0, 1]
    toNormalisedCoordinates = Matrix3f(1.0f / w, 0, 0, 1.0f / h, 0, 0);

    // Take the smallest dimensions in order to calculate the aspect ratio
    int size = w < h ? w : h;

    float wAspect = (float)w / size;
    float hAspect = (float)h / size;
    float wDiff = ((wAspect - 1) / 2.0);
    float hDiff = ((hAspect - 1) / 2.0);

    toIsotropicCoordinates = Matrix3f(wAspect, 0, 0, hAspect, -wDiff, -hDiff);
}

void ScatterplotWidget::paintGL()
{
    try {
        const auto areaPixmap   = _pixelSelectionTool.getAreaPixmap();
        const auto shapePixmap  = _pixelSelectionTool.getShapePixmap();

        QPainter painter;

        // Begin mixed OpenGL/native painting
        if (!painter.begin(this))
            throw std::runtime_error("Unable to begin painting");

        // Draw layers with OpenGL
        painter.beginNativePainting();
        {
            // Bind the framebuffer belonging to the widget
            // glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

            // Clear the widget to the background color
            glClearColor(_backgroundColor.redF(), _backgroundColor.greenF(), _backgroundColor.blueF(), _backgroundColor.alphaF());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Reset the blending function
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
               
            switch (_renderMode)
            {
                case SCATTERPLOT:
                    _pointRenderer.render();
                    break;

                case DENSITY:
                case LANDSCAPE:
                    _densityRenderer.setRenderMode(_renderMode == DENSITY ? DensityRenderer::DENSITY : DensityRenderer::LANDSCAPE);
                    _densityRenderer.render();
                    break;
            }
                
        }
        painter.endNativePainting();
        
        // Draw the pixel selection tool overlays if the pixel selection tool is enabled
        if (_pixelSelectionTool.isEnabled()) {
            painter.drawPixmap(rect(), areaPixmap);
            painter.drawPixmap(rect(), shapePixmap);
        }
        
        painter.end();
    }
    catch (std::exception& e)
    {
        exceptionMessageBox("Rendering failed", e);
    }
    catch (...) {
        exceptionMessageBox("Rendering failed");
    }
}

void ScatterplotWidget::cleanup()
{
    qDebug() << "Deleting scatterplot widget, performing clean up...";
    _isInitialized = false;

    makeCurrent();
    _pointRenderer.destroy();
    _densityRenderer.destroy();
}

void ScatterplotWidget::setColorMap(const QImage& colorMapImage)
{
    _colorMapImage = colorMapImage;

    // Do not update color maps of the renderers when OpenGL is not initialized
    if (!_isInitialized)
        return;

    // Apply color maps to renderers
    _pointRenderer.setColormap(_colorMapImage);
    _densityRenderer.setColormap(_colorMapImage);

    // Render
    update();
}

void ScatterplotWidget::updatePixelRatio()
{
    float pixelRatio = devicePixelRatio();
    
#ifdef SCATTER_PLOT_WIDGET_VERBOSE
    qDebug() << "Window moved to screen " << window()->screen() << ".";
    qDebug() << "Pixelratio before was " << _pixelRatio << ". New pixelratio is: " << pixelRatio << ".";
#endif // SCATTER_PLOT_WIDGET_VERBOSE
    
    // we only update if the ratio actually changed
    if( _pixelRatio != pixelRatio )
    {
        _pixelRatio = pixelRatio;
        resizeGL(width(), height());
        update();
    }
}

ScatterplotWidget::~ScatterplotWidget()
{
    disconnect(QOpenGLWidget::context(), &QOpenGLContext::aboutToBeDestroyed, this, &ScatterplotWidget::cleanup);
    cleanup();
}
