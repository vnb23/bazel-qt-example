#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "polygon.h"
class Controller {
public:
    Controller() {
        light_source_ = QPointF(400, 300);
        secondary_sources_.clear();
        static_sources_.clear();
    }
    std::optional<QPointF> getIntersection(const QPointF& p1, const QPointF& p2,
                                     const QPointF& p3, const QPointF& p4) const {
        const double epsilon = 1e-10;

        if (distance(p1, p2) < epsilon || distance(p3, p4) < epsilon) {
            return std::nullopt;
        }

        int o1 = orientation(p1, p2, p3);
        int o2 = orientation(p1, p2, p4);
        int o3 = orientation(p3, p4, p1);
        int o4 = orientation(p3, p4, p2);

        if (o1 != o2 && o3 != o4) {
            double x1 = p1.x(), y1 = p1.y();
            double x2 = p2.x(), y2 = p2.y();
            double x3 = p3.x(), y3 = p3.y();
            double x4 = p4.x(), y4 = p4.y();

            double denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

            if (std::abs(denominator) < epsilon) {
                return std::nullopt;
            }

            if (o1 == 0 && onSegment(p1, p3, p2)) return p3;
            if (o2 == 0 && onSegment(p1, p4, p2)) return p4;
            if (o3 == 0 && onSegment(p3, p1, p4)) return p1;
            if (o4 == 0 && onSegment(p3, p2, p4)) return p2;

            double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denominator;
            double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denominator;

            if (t >= -epsilon && t <= 1.0 + epsilon &&
                u >= -epsilon && u <= 1.0 + epsilon) {
                return QPointF(x1 + t * (x2 - x1), y1 + t * (y2 - y1));
                }
        }

        return std::nullopt;
    }
    void updateSecondarySources() {
        const int count = 8;
        const double radius = 15.0;
        secondary_sources_.clear();

        for (int i = 0; i < count; ++i) {
            double angle = 2 * M_PI * i / count;
            QPointF offset(radius * cos(angle), radius * sin(angle));
            secondary_sources_.push_back(light_source_ + offset);
        }
    }

    const std::vector<QPointF>& getSecondarySources() const {
        return secondary_sources_;
    }

    const std::vector<QPointF>& getStaticSources() const {
        return static_sources_;
    }

    void addStaticSource(const QPointF& point) {
        static_sources_.push_back(point);
    }

    void clearStaticSources() {
        static_sources_.clear();
    }

    const std::vector<Polygon>& GetPolygons() const {
        return polygons_;
    }
    bool DoesPolygonIntersectOthers(const std::vector<QPointF>& new_vertices) const {
        int idx = 0;
        for (const auto& poly : polygons_) {
            if (idx == 0 || idx == polygons_.size() - 1) {
                idx++;
                continue;
            }
            idx++;
            const auto& vertices = poly.getVertices();
            if (vertices.size() < 2) continue;

            for (size_t i = 0; i + 1 < new_vertices.size(); ++i) {
                for (size_t j = 0; j + 1 < vertices.size(); ++j) {
                    if (getIntersection(new_vertices[i], new_vertices[i+1], vertices[j], vertices[j+1])) {
                        return true;
                    }
                    if (getIntersection(new_vertices.back(), new_vertices[0], vertices[j], vertices[j+1])) {
                        return true;
                    }
                }
                if (getIntersection(new_vertices[i], new_vertices[i+1], vertices.back(), vertices[0])) {
                    return true;
                }
            }
        }
        return false;
    }
    void AddPolygon(const Polygon& polygon) {
        const auto& vertices = polygon.getVertices();

        QRectF sceneRect(0, 0, 1000, 800);
        for (const auto& v : vertices) {
            if (!sceneRect.contains(v)) return;
        }

        if (vertices.size() >= 3) {
            for (size_t i = 0; i + 1 < vertices.size(); ++i) {
                for (size_t j = i + 2; j + 1 < vertices.size(); ++j) {
                    if (j == i + 1) continue;

                    if (getIntersection(vertices[i], vertices[i+1],
                                      vertices[j], vertices[j+1])) {
                        return;
                                      }
                }
            }
        }

        if (DoesPolygonIntersectOthers(vertices)) {
            return;
        }

        polygons_.push_back(polygon);
    }
    void AddVertexToLastPolygon(const QPointF& new_vertex) {
        if (polygons_.empty()) return;

        QRectF sceneRect(0, 0, 1000, 800);
        if (!sceneRect.contains(new_vertex)) {
            return;
        }
        polygons_.back().AddVertex(new_vertex);
        std::vector<QPointF> vertices = polygons_.back().getVertices();
            if (vertices.size() >= 2) {
                for (size_t i = 0; i + 1 < vertices.size() - 1; ++i) {
                    for (size_t j = i + 2; j + 1 < vertices.size(); ++j) {
                        if (j == i + 1) continue;
                        if (getIntersection(vertices[i], vertices[i + 1],vertices[j], vertices[j+1])) {
                            polygons_.back().deleteVertex();
                            return;
                        }
                    }
                    if (i == 0 || i + 1 == vertices.size() - 1) continue;
                    if (getIntersection(vertices[i], vertices[i + 1], vertices.back(), vertices[0])) {
                        polygons_.back().deleteVertex();
                        return;
                    }
                }
            }
        if (DoesPolygonIntersectOthers(polygons_.back().getVertices())) {
            polygons_.back().deleteVertex();
        }
    }
    void UpdateLastPolygon(const QPointF& new_vertex) {
        polygons_.back().UpdateLastVertex(new_vertex);
    }

    void setLightSource(QPointF p) {
        light_source_ = p;
        updateSecondarySources();
    }

    QPointF getLightSource() const {
        return light_source_;
    }

    std::vector<Ray> CastRays() const {
        std::vector<Ray> rays;
        constexpr double epsilon = 0.0001;
        constexpr double scene_margin = 3000.0;

        for (const auto& polygon : polygons_) {
            if (polygon.getVertices().size() == 1) continue;
            for (const auto& vertex : polygon.getVertices()) {
                QPointF direction = vertex - light_source_;
                double length = std::hypot(direction.x(), direction.y());
                if (length < 1e-10) continue;
                direction /= length;

                QPointF extended_end = light_source_ + direction * scene_margin;
                double angle = std::atan2(direction.y(), direction.x());
                Ray base_ray(light_source_, extended_end, angle);

                rays.push_back(base_ray);
                rays.push_back(base_ray.Rotate(+epsilon));
                rays.push_back(base_ray.Rotate(-epsilon));
            }
        }
        return rays;
    }

    std::vector<Ray> CastStaticRays(const QPointF& source) const {
        std::vector<Ray> rays;
        constexpr double epsilon = 0.0001;
        constexpr double scene_margin = 3000.0;

        for (const auto& polygon : polygons_) {
            if (polygon.getVertices().size() == 1) continue;
            for (const auto& vertex : polygon.getVertices()) {
                QPointF direction = vertex - source;
                double length = std::hypot(direction.x(), direction.y());
                if (length < 1e-10) continue;
                direction /= length;

                QPointF extended_end = source + direction * scene_margin;
                double angle = std::atan2(direction.y(), direction.x());
                Ray base_ray(source, extended_end, angle);

                rays.push_back(base_ray);
                rays.push_back(base_ray.Rotate(+epsilon));
                rays.push_back(base_ray.Rotate(-epsilon));
            }
        }
        return rays;
    }

    std::vector <Ray> CastSecondaryRays (int idx) const {
        std::vector<Ray> rays;
        constexpr double epsilon = 0.0001;
        constexpr double scene_margin = 3000.0;

        for (const auto& polygon : polygons_) {
            if (polygon.getVertices().size() == 1) continue;
            for (const auto& vertex : polygon.getVertices()) {
                QPointF direction = vertex - secondary_sources_[idx];
                double length = std::hypot(direction.x(), direction.y());
                if (length < 1e-10) continue;
                direction /= length;

                QPointF extended_end = secondary_sources_[idx] + direction * scene_margin;
                double angle = std::atan2(direction.y(), direction.x());
                Ray base_ray(secondary_sources_[idx], extended_end, angle);

                rays.push_back(base_ray);
                rays.push_back(base_ray.Rotate(+epsilon));
                rays.push_back(base_ray.Rotate(-epsilon));
            }
        }
        return rays;
    }
    void IntersectRays(std::vector<Ray>* rays, const QPointF& source) const {
        constexpr double epsilon = 1e-6;

        for (auto& ray : *rays) {
            QPointF closest_end = ray.getEnd();
            double min_dist = std::numeric_limits<double>::max();

            for (const auto& polygon : polygons_) {
                auto intersection = polygon.IntersectRay(ray);
                if (intersection) {
                    double dist = distance(source, *intersection);
                    if (dist > epsilon && dist < min_dist) {
                        min_dist = dist;
                        closest_end = *intersection;
                    }
                }
            }

            ray = Ray(ray.getBegin(), closest_end, ray.getAngle());
        }
    }

    void RemoveAdjacentRays(std::vector<Ray>* rays) const {
        if (rays->empty()) return;

        std::sort(rays->begin(), rays->end(), [](const Ray& a, const Ray& b) {
            return a.getAngle() < b.getAngle();
        });

        std::vector<Ray> filtered;
        filtered.push_back(rays->front());

        constexpr double angleThreshold = 0.001;
        constexpr double distanceThreshold = 1.0;

        for (size_t i = 1; i < rays->size(); ++i) {
            const Ray& prev = filtered.back();
            const Ray& current = (*rays)[i];
            if (std::abs(current.getAngle() - prev.getAngle()) > angleThreshold ||
                distance(current.getEnd(), prev.getEnd()) > distanceThreshold) {
                filtered.push_back(current);
            }
        }

        *rays = std::move(filtered);
    }

    Polygon CreateLightArea() const {
        std::vector<Ray> rays = CastRays();
        IntersectRays(&rays, light_source_);
        RemoveAdjacentRays(&rays);

        std::sort(rays.begin(), rays.end(), [](const Ray& a, const Ray& b) {
            return a.getAngle() < b.getAngle();
        });

        std::vector<QPointF> points;
        points.reserve(rays.size());
        for (const auto& ray : rays) {
            points.push_back(ray.getEnd());
        }

        return Polygon(points);
    }

    Polygon CreateStaticLightArea(const QPointF& source) const {
        std::vector<Ray> rays = CastStaticRays(source);
        IntersectRays(&rays, source);
        RemoveAdjacentRays(&rays);

        std::sort(rays.begin(), rays.end(), [](const Ray& a, const Ray& b) {
            return a.getAngle() < b.getAngle();
        });

        std::vector<QPointF> points;
        points.reserve(rays.size());
        for (const auto& ray : rays) {
            points.push_back(ray.getEnd());
        }

        return Polygon(points);
    }

    Polygon CreateSecondaryLightArea(int idx) const {
        std::vector<Ray> rays = CastSecondaryRays(idx);
        IntersectRays(&rays, secondary_sources_[idx]);
        RemoveAdjacentRays(&rays);

        std::sort(rays.begin(), rays.end(), [](const Ray& a, const Ray& b) {
            return a.getAngle() < b.getAngle();
        });

        std::vector<QPointF> points;
        points.reserve(rays.size());
        for (const auto& ray : rays) {
            points.push_back(ray.getEnd());
        }

        return Polygon(points);
    }
private:
    double distance(const QPointF& p1, const QPointF& p2) const {
        return std::hypot(p2.x() - p1.x(), p2.y() - p1.y());
    }

    int orientation(const QPointF& p, const QPointF& q, const QPointF& r) const {
        double val = (q.y() - p.y()) * (r.x() - q.x()) -
                     (q.x() - p.x()) * (r.y() - q.y());

        if (val == 0) return 0;
        return (val > 0) ? 1 : 2;
    }

    bool onSegment(const QPointF& p, const QPointF& q, const QPointF& r) const {
        return q.x() <= std::max(p.x(), r.x()) && q.x() >= std::min(p.x(), r.x()) &&
               q.y() <= std::max(p.y(), r.y()) && q.y() >= std::min(p.y(), r.y());
    }
    std::vector <QPointF> secondary_sources_;
    std::vector <QPointF> static_sources_;
    std::vector<Polygon> polygons_;
    QPointF light_source_;
};
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    private slots:
        void onModeChanged(int index);

private:
    static constexpr qreal BORDER_MARGIN = 20.0;
    void updateScene();
    void addBorderPolygon();
    void addStaticLight(const QPointF& position);

    QGraphicsScene *scene;
    QGraphicsView *view;
    QComboBox *modeComboBox;
    Controller *controller;
    QGraphicsEllipseItem *lightPoint;
    std::vector<QGraphicsEllipseItem*> secondaryLightPoints;
    std::vector<QGraphicsEllipseItem*> staticLightPoints;
    std::vector<QGraphicsPolygonItem*> staticLightAreas;
    QString currentMode;
    bool isCreatingPolygon;
};
#endif // MAINWINDOW_H