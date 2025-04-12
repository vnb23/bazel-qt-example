#ifndef POLYGON_H
#define POLYGON_H
#include "ray.h"
class Polygon {
public:
    Polygon(const std::vector<QPointF>& vertices) : vertices_(vertices) {}

    [[nodiscard]] std::vector<QPointF> getVertices() const { return vertices_; }

    void AddVertex(const QPointF& vertex) {
        vertices_.push_back(vertex);
    }

    void UpdateLastVertex(const QPointF& new_vertex) {
        vertices_.pop_back();
        vertices_.push_back(new_vertex);
    }
    std::optional<QPointF> IntersectRay(const Ray& ray) const {
        QPointF closest;
        double min_dist = 1e9;
        bool found = false;
        for (int i = 0; i + 1 < vertices_.size(); i++) {
            QPointF p1 = vertices_[i];
            QPointF p2 = vertices_[i + 1];
            auto intersection = getIntersection(ray.getBegin(), ray.getEnd(), p1, p2);
            if (intersection) {
                double dist = distance(ray.getBegin(), *intersection);
                if (dist < min_dist) {
                    min_dist = dist;
                    closest = *intersection;
                    found = true;
                }
            }
        }

        if (vertices_.size() > 1) {
            QPointF p1 = vertices_.back();
            QPointF p2 = vertices_[0];
            auto intersection = getIntersection(ray.getBegin(), ray.getEnd(), p1, p2);
            if (intersection) {
                double dist = distance(ray.getBegin(), *intersection);
                if (dist < min_dist) {
                    min_dist = dist;
                    closest = *intersection;
                    found = true;
                }
            }
        }

        return found ? std::optional<QPointF>(closest) : std::nullopt;
    }
    void deleteVertex() {
        vertices_.pop_back();
    }
private:
    std::vector<QPointF> vertices_;

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
};
#endif //POLYGON_H
