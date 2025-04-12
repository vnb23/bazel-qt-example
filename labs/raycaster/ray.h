#ifndef RAY_H
#define RAY_H
#include <QMainWindow>
#include <QLayout>
#include <QPoint>
#include <optional>
#include <QGraphicsEllipseItem>
#include <QGraphicsView>
#include <QComboBox>
class Ray {
public:
    Ray(const QPointF& begin, const QPointF& end, double angle) : begin_(begin), end_(end), angle_(angle) {}
    [[nodiscard]] QPointF getBegin() const { return begin_; }
    [[nodiscard]] QPointF getEnd() const { return end_; }
    [[nodiscard]] double getAngle() const { return angle_; }
    [[nodiscard]] Ray Rotate(double angle) {
        QPointF direction = end_ - begin_;

        QPointF rotated(
            direction.x() * cos(angle) - direction.y() * sin(angle),
            direction.x() * sin(angle) + direction.y() * cos(angle)
        );

        QPointF newEnd = begin_ + rotated;

        return {begin_, newEnd, angle_ + angle};
    }
private:
    QPointF begin_;
    QPointF end_;
    double angle_;
};
#endif //RAY_H
