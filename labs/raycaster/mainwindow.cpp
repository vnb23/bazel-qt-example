#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QTimer>
#include <QPen>
#include <QBrush>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , scene(new QGraphicsScene(this))
    , view(new QGraphicsView(scene))
    , modeComboBox(new QComboBox(this))
    , controller(new Controller())
    , lightPoint(nullptr)
    , currentMode("light")
    , isCreatingPolygon(false)
{
    setWindowTitle("2D Raycaster");
    resize(1000, 800);
    showFullScreen();

    scene->setSceneRect(0, 0, 1000, 800);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMouseTracking(true);
    view->viewport()->setMouseTracking(true);
    view->viewport()->installEventFilter(this);
    setMouseTracking(true);

    modeComboBox->addItem("Light Mode", "light");
    modeComboBox->addItem("Polygons Mode", "polygons");
    modeComboBox->addItem("Static Lights Mode", "static-lights");
    connect(modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeChanged);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    QHBoxLayout *controlsLayout = new QHBoxLayout();

    controlsLayout->addWidget(modeComboBox);
    layout->addLayout(controlsLayout);
    layout->addWidget(view);

    setCentralWidget(centralWidget);
    controller->updateSecondarySources();
    lightPoint = scene->addEllipse(-2, -2, 4, 4, QPen(Qt::NoPen), QBrush(Qt::red));
    lightPoint->setPos(controller->getLightSource());
    lightPoint->setZValue(10);
    for (const auto& source : controller->getSecondarySources()) {
        auto point = scene->addEllipse(-2, -2, 4, 4, QPen(Qt::NoPen), QBrush(Qt::red));
        point->setPos(source);
        point->setZValue(10);
        secondaryLightPoints.push_back(point);
    }

    addBorderPolygon();
}

MainWindow::~MainWindow()
{
    delete controller;
    delete scene;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == view->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (currentMode == "light") {
                QPointF scenePos = view->mapToScene(mouseEvent->pos());
                isCreatingPolygon = false;
                QRectF bounds = scene->sceneRect();
                scenePos.setX(qBound(bounds.left() + 15.5, scenePos.x(), bounds.right() - 15.5));
                scenePos.setY(qBound(bounds.top() + 15.5, scenePos.y(), bounds.bottom() - 15.5));

                controller->setLightSource(scenePos);
                controller->updateSecondarySources();
                lightPoint->setPos(scenePos);
                updateScene();
            }
        } else if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            QPointF scenePos = view->mapToScene(mouseEvent->pos());
            QRectF bounds = scene->sceneRect();
            scenePos.setX(qBound(bounds.left() + 15.5, scenePos.x(), bounds.right() - 15.5));
            scenePos.setY(qBound(bounds.top() + 15.5, scenePos.y(), bounds.bottom() - 15.5));

            if (currentMode == "polygons") {
                if (mouseEvent->button() == Qt::LeftButton) {
                    if (isCreatingPolygon) {
                        controller->AddVertexToLastPolygon(scenePos);
                    } else {
                        std::vector<QPointF> vertices = {scenePos};
                        controller->AddPolygon(Polygon(vertices));
                        isCreatingPolygon = true;
                    }
                } else if (mouseEvent->button() == Qt::RightButton && isCreatingPolygon) {
                    isCreatingPolygon = false;
                }
            } else if (currentMode == "static-lights" && mouseEvent->button() == Qt::LeftButton) {
                addStaticLight(scenePos);
            }

            updateScene();
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onModeChanged(int index)
{
    currentMode = modeComboBox->itemData(index).toString();
    lightPoint->setVisible(currentMode == "light");
    for (auto& point : secondaryLightPoints) {
        point->setVisible(currentMode == "light");
    }
    updateScene();
}

void MainWindow::updateScene()
{
    QPointF lightPos = controller->getLightSource();
    std::vector<QPointF> v = controller->getSecondarySources();

    QList<QGraphicsItem*> items = scene->items();
    for (QGraphicsItem* item : items) {
        if (item != lightPoint &&
            std::find(secondaryLightPoints.begin(), secondaryLightPoints.end(), item) == secondaryLightPoints.end() &&
            std::find(staticLightPoints.begin(), staticLightPoints.end(), item) == staticLightPoints.end()) {
            scene->removeItem(item);
            delete item;
        }
    }

    lightPoint->setVisible(currentMode == "light");
    for (auto& point : secondaryLightPoints) {
        point->setVisible(currentMode == "light");
    }

    if (currentMode == "light") {
        lightPoint->setPos(lightPos);
        for (int i = 0; i < controller->getSecondarySources().size(); i++) {
            secondaryLightPoints[i]->setPos(v[i]);
        }
    }

    QPen polygonPen(Qt::white, 2);
    QBrush polygonBrush(Qt::NoBrush);

    for (const auto& polygon : controller->GetPolygons()) {
        const auto vertices = polygon.getVertices();
        if(vertices.size() < 2) continue;

        QPainterPath path;
        path.moveTo(vertices[0]);
        for(size_t i = 1; i < vertices.size(); ++i) {
            path.lineTo(vertices[i]);
        }

        if(vertices.size() >= 3) {
            path.closeSubpath();
        }

        QGraphicsPathItem* polygonItem = scene->addPath(path, polygonPen, polygonBrush);
        polygonItem->setZValue(5);
    }

    if(currentMode == "light" || currentMode == "static-lights") {
        if (currentMode == "light") {
            std::vector<Ray> rays = controller->CastRays();
            controller->IntersectRays(&rays, controller->getLightSource());
            controller->RemoveAdjacentRays(&rays);

            Polygon lightArea = controller->CreateLightArea();
            const auto& vertices = lightArea.getVertices();
            if(vertices.size() >= 3) {
                QPainterPath path;
                path.moveTo(vertices[0]);
                for(size_t i = 1; i < vertices.size(); ++i) {
                    path.lineTo(vertices[i]);
                }
                path.closeSubpath();

                QGraphicsPathItem* lightItem = scene->addPath(path);
                lightItem->setBrush(QBrush("#fff"));
                lightItem->setPen(Qt::NoPen);
                lightItem->setZValue(1);
            }
            for (int i = 0; i < controller->getSecondarySources().size(); i++) {
                lightArea = controller->CreateSecondaryLightArea(i);
                const auto& vertices2 = lightArea.getVertices();
                if(vertices2.size() >= 3) {
                    QPainterPath path;
                    path.moveTo(vertices2[0]);
                    for(size_t i = 1; i < vertices2.size(); ++i) {
                        path.lineTo(vertices2[i]);
                    }
                    path.closeSubpath();

                    QGraphicsPathItem* lightItem = scene->addPath(path);
                    lightItem->setBrush(QBrush(QColor(255, 255, 255, 51)));
                    lightItem->setPen(Qt::NoPen);
                    lightItem->setZValue(1);
                }
            }
        }

        for (const auto& source : controller->getStaticSources()) {
            Polygon staticArea = controller->CreateStaticLightArea(source);
            const auto& staticVertices = staticArea.getVertices();
            if (staticVertices.size() >= 3) {
                QPainterPath path;
                path.moveTo(staticVertices[0]);
                for (size_t i = 1; i < staticVertices.size(); ++i) {
                    path.lineTo(staticVertices[i]);
                }
                path.closeSubpath();

                QGraphicsPathItem* staticLightItem = scene->addPath(path);
                staticLightItem->setBrush(QBrush(QColor(173, 216, 230, 100)));
                staticLightItem->setPen(Qt::NoPen);
                staticLightItem->setZValue(1);
            }
        }
    }
}

void MainWindow::addStaticLight(const QPointF& position)
{
    controller->addStaticSource(position);

    auto point = scene->addEllipse(-2, -2, 4, 4, QPen(Qt::NoPen), QBrush(Qt::blue));
    point->setPos(position);
    point->setZValue(10);
    staticLightPoints.push_back(point);

    updateScene();
}

void MainWindow::addBorderPolygon()
{
    QRectF sceneRect = scene->sceneRect();

    std::vector<QPointF> border = {
        QPointF(sceneRect.left(), sceneRect.top()),
        QPointF(sceneRect.right(), sceneRect.top()),
        QPointF(sceneRect.right(), sceneRect.bottom()),
        QPointF(sceneRect.left(), sceneRect.bottom())
    };

    controller->AddPolygon(Polygon(border));
    updateScene();
}