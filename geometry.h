#include <algorithm>
#include <cmath>
#include <iostream>
const double kAccuracy = 1e-6;

class Line;
class Vector;

class Point {
 public:
  double x;
  double y;

  Point() = default;

  Point(double x, double y) : x(x), y(y) {}

  Point(const Vector&);

  bool operator==(const Point& other) const {
    return std::abs(x - other.x) < kAccuracy &&
           std::abs(y - other.y < kAccuracy);
  }

  bool operator!=(const Point& other) const {
    return !(*this == other);
  }

  Point simetric(const Line& line) const;

  double distance(const Point& other) const {
    return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
  }
};

class Vector {
 public:
  double x_coord;
  double y_coord;

  Vector(double x, double y) : x_coord(x), y_coord(y) {}

  Vector(const Point& first, const Point& second)
      : x_coord(second.x - first.x), y_coord(second.y - first.y) {}

  Vector(const Point& point) : x_coord(point.x), y_coord(point.y) {}

  Vector(const Vector& other) = default;

  Vector rotated(double angle) const {
    return Vector(std::cos(angle) * x_coord - std::sin(angle) * y_coord,
                  std::cos(angle) * y_coord + std::sin(angle) * x_coord);
  }

  double abs() const {
    return std::sqrt(x_coord * x_coord + y_coord * y_coord);
  }

  Vector operator*(double k) const {
    return Vector(x_coord * k, y_coord * k);
  }

  Vector operator-() const {
    return Vector(-x_coord, -y_coord);
  }

  Vector operator+(const Vector& other) const {
    return Vector(x_coord + other.x_coord, y_coord + other.y_coord);
  }

  Vector operator-(const Vector& other) const {
    return *this + (-other);
  }

  double operator*(const Vector& other) const {
    return x_coord * other.x_coord + y_coord * other.y_coord;
  }

  Vector& operator*=(double k) {
    x_coord *= k;
    y_coord *= k;
    return *this;
  }

  bool is_coliniar(const Vector& other) const {
    return std::abs(x_coord * other.y_coord - y_coord * other.x_coord) <
           kAccuracy;
  }

  Vector normed() const {
    return *this * (1. / abs());
  }
};

class Line {
 public:
  Point point;
  Vector dir;

  Line(const Point& first, const Point& second)
      : point(first), dir(Vector(first, second)) {}

  Line(double k, double b_)
      : point(Point(0, b_)),
        dir(Vector(std::cos(std::atan(k)), std::sin(std::atan(k)))) {}

  Line(const Point& point, double k)
      : point(point),
        dir(Vector(std::cos(std::atan(k)), std::sin(std::atan(k)))) {}

  Line(const Point& point, const Vector& vec) : point(point), dir(vec) {}

  bool operator==(const Line& other) const {
    return Vector(point, other.point).is_coliniar(dir) &&
           dir.is_coliniar(other.dir);
  }

  bool operator!=(const Line& other) const {
    return !(*this == other);
  }

  void print() {  ///////
    std::cout << point.x << ' ' << point.y << ' ' << dir.x_coord / dir.abs()
              << ' ' << dir.y_coord / dir.abs();
  }
};

Point operator+(const Point& point, const Vector& vec) {
  return Point(point.x + vec.x_coord, point.y + vec.y_coord);
}

Point Point::simetric(const Line& line) const {
  Vector r1 = *this;
  Vector r0 = line.point;
  Vector projection = line.dir * (line.dir * (r1 - r0)) *
                      (1 / (line.dir.abs() * line.dir.abs()));
  Vector ans = r1 - (r1 - r0 - projection) * 2;
  return Point(0, 0) + ans;
}

Point::Point(const Vector& other) : x(other.x_coord), y(other.y_coord) {}

class Shape {
 public:
  virtual double perimeter() const = 0;

  virtual double area() const = 0;

  bool operator==(const Shape&) const;

  bool isCongruentTo(const Shape&) const;

  bool isSimilarTo(const Shape&) const;

  virtual bool containsPoint(const Point&) const = 0;

  virtual void rotate(const Point&, double) = 0;

  virtual void reflect(const Point&) = 0;

  virtual void reflect(const Line&) = 0;

  virtual void scale(const Point&, double) = 0;
};

class Ellipse : public Shape {
 private:
  Point F1_;
  Point F2_;
  double a_;
  double b_;

 public:
  Ellipse(const Point& F1_, const Point& F2_, double len)
      : F1_(F1_),
        F2_(F2_),
        a_(len / 2),
        b_(std::sqrt(std::pow(len, 2) - std::pow(F1_.distance(F2_), 2)) / 2) {}

  std::pair<Point, Point> focuses() const {
    return std::make_pair(F1_, F2_);
  }

  double eccentricity() const {
    return std::sqrt(1 - std::pow(b_ / a_, 2));
  }

  Point center_() const {
    Vector F1_F2_dir = Vector(F1_, F2_);

    return F1_ + F1_F2_dir * (1. / 2);
  }

  std::pair<Line, Line> directrices() const {
    double e = eccentricity();

    Vector F1_F2_dir = Vector(F1_, F2_);
    Vector d_dir = Vector(-F1_F2_dir.y_coord, F1_F2_dir.x_coord);

    Point d1_point = center_() + F1_F2_dir.normed() * (a_ / e);
    Point d2_point = center_() + -F1_F2_dir.normed() * (a_ / e);

    Line d1(d1_point, d_dir);
    Line d2(d2_point, d_dir);

    return std::make_pair(d1, d2);
  }

  double perimeter() const final {
    return M_PIf64x *
           (3 * (a_ + b_) - std::sqrt((3 * a_ + b_) * (3 * b_ + a_)));
  }

  double area() const final {
    return M_PIf64x * a_ * b_;
  }

  bool containsPoint(const Point& point) const final {
    return F1_.distance(point) + F2_.distance(point) <= 2 * a_ + kAccuracy;  //
  }

  void rotate(const Point& point, double angle) final {
    F1_ = point + Vector(point, F1_).rotated(angle);
    F2_ = point + Vector(point, F2_).rotated(angle);
  }

  void reflect(const Point& point) final {
    F1_ = point + -Vector(point, F1_);
    F2_ = point + -Vector(point, F2_);
  }

  void reflect(const Line& line) final {
    F1_ = F1_.simetric(line);
    F2_ = F2_.simetric(line);
  }

  void scale(const Point& point, double coef) final {
    F1_ = point + Vector(point, F1_) * coef;
    F2_ = point + Vector(point, F2_) * coef;
    a_ *= std::abs(coef);
    b_ *= std::abs(coef);
  }
};

class Circle : public Shape {
 public:
  Point center_;
  double r_;

 public:
  Circle(const Point& point, double r_) : center_(point), r_(r_) {}

  double radius() const {
    return r_;
  }

  double perimeter() const final {
    return 2 * M_PIf64x * r_;
  }

  double area() const final {
    return M_PIf64x * std::pow(r_, 2);
  }

  bool containsPoint(const Point& point) const final {
    return center_.distance(point) <= r_ + kAccuracy;
  }

  void rotate(const Point& point, double angle) final {
    center_ = point + Vector(point, center_).rotated(angle);
  }

  void reflect(const Point& point) final {
    center_ = point + -Vector(point, center_);
  }

  void reflect(const Line& line) final {
    center_ = center_.simetric(line);
  }

  void scale(const Point& point, double coef) final {
    center_ = point + Vector(point, center_) * coef;
    r_ *= std::abs(coef);
  }
};

class Polygon : public Shape {
 private:
  bool is_z_sign_posotive(const Point& first, const Point& second,
                          const Point& third) const {
    Vector v1(first, second);
    Vector v2(second, third);

    return v1.x_coord * v2.y_coord > v2.x_coord * v1.y_coord;
  }

 protected:
  std::vector<Point> points_;

 public:
  Polygon() = default;

  Polygon(const std::vector<Point>& points) : points_(points) {}

  template <typename... T>
  Polygon(T... elems) : points_{elems...} {}

  size_t verticesCount() const {
    return points_.size();
  }

  std::vector<Point> getVertices() const {  // const std::vector?
    return points_;
  }

  bool isConvex() const {  // less 1 vertex ???
    bool sign = is_z_sign_posotive(points_[0], points_[1 % points_.size()],
                                   points_[2 % points_.size()]);

    for (size_t i = 1; i < points_.size(); ++i) {
      bool cnt_sign =
          is_z_sign_posotive(points_[i], points_[(i + 1) % points_.size()],
                             points_[(i + 2) % points_.size()]);
      if (cnt_sign != sign) {
        return false;
      }
    }

    return true;
  }

  double perimeter() const final {
    double ans = 0;

    for (size_t i = 0; i < points_.size(); ++i) {
      ans += points_[i].distance(points_[(i + 1) % points_.size()]);
    }
    return ans;
  }

  double area() const override {
    double ans = 0;

    for (size_t i = 0; i < points_.size(); ++i) {
      ans +=
          points_[i].x * (points_[(i + 1) % points_.size()].y -
                          points_[(i - 1 + points_.size()) % points_.size()].y);
    }

    return std::abs(ans / 2);
  }

  bool containsPoint(const Point& point) const final {  // kAccuracy???
    bool result = false;

    for (size_t i = 0; i < points_.size(); ++i) {
      if ((points_[i].y < point.y &&
               points_[(i - 1 + points_.size()) % points_.size()].y >=
                   point.y ||
           points_[(i - 1 + points_.size()) % points_.size()].y < point.y &&
               points_[i].y >= point.y) &&
          (points_[i].x +
               (point.y - points_[i].y) /
                   (points_[(i - 1 + points_.size()) % points_.size()].y -
                    points_[i].y) *
                   (points_[(i - 1 + points_.size()) % points_.size()].x -
                    points_[i].x) <
           point.x)) {
        result = !result;
      }
    }

    return result;
  }

  void rotate(const Point& point, double angle) final {
    for (size_t i = 0; i < points_.size(); ++i) {
      points_[i] = point + Vector(point, points_[i]).rotated(angle);
    }
  }

  void reflect(const Point& point) final {
    for (size_t i = 0; i < points_.size(); ++i) {
      points_[i] = point + -Vector(point, points_[i]);
    }
  }

  void reflect(const Line& line) final {
    for (size_t i = 0; i < points_.size(); ++i) {
      points_[i] = points_[i].simetric(line);
    }
  }

  void scale(const Point& point, double coef) final {
    for (size_t i = 0; i < points_.size(); ++i) {
      points_[i] = point + Vector(point, points_[i]) * coef;
    }
  }
};

class Rectangle : public Polygon {
 public:
  Rectangle(const Point& P1, const Point& P3, double coef) {
    Point center = (Vector(P1) + Vector(P3)) * 0.5;
    Vector cnt = Vector(P1) + -Vector(center);
    Point P2(Vector(center) + cnt.rotated(-2 * std::atan(coef)));
    Point P4 = P3 + -Vector(P1, P2);
    points_ = {P1, P2, P3, P4};
  }

  Point center() const {
    return Point((Vector(points_[0]) + Vector(points_[2])) * 0.5);
  }

  std::pair<Line, Line> diagonals() const {
    return std::make_pair(Line(points_[0], points_[2]),
                          Line(points_[1], points_[3]));
  }

  double area() const override {
    return points_[0].distance(points_[1]) * points_[1].distance(points_[2]);
  }
};

class Square : public Rectangle {
 public:
  Square(const Point& P1, const Point& P3) : Rectangle(P1, P3, 1) {}

  Circle inscribedCircle() const {
    return Circle(center(), points_[0].distance(points_[1]) / 2);
  }

  Circle circumscribedCircle() const {
    return Circle(center(), points_[0].distance(center()) / 2);
  }

  double area() const final {
    return std::pow(points_[0].distance(points_[1]), 2);
  }
};

class Triangle : public Polygon {
 public:
  using Polygon::Polygon;

  Point inCenter() const {
    Vector v1(points_[0], points_[1]);
    Vector v2(points_[0], points_[2]);
    double angle = std::acos((v1 * v2) / (v1.abs() * v2.abs()));
    return points_[0] +
           (v1.normed() + v2.normed()) * (0.5 / std::sin(angle / 2));
  }

  Circle inscribedCircle() const {
    return Circle(inCenter(), 2 * area() / perimeter());
  }

  Point centroid() const {
    return points_[0] +
           (Vector(points_[0], points_[1]) + Vector(points_[0], points_[2])) *
               (1. / 3);
  }

  Circle circumscribedCircle() const {
    return Circle(outcenter(), points_[0].distance(points_[1]) *
                                   points_[0].distance(points_[2]) *
                                   points_[1].distance(points_[2]) /
                                   (4 * area()));
  }

  Point outcenter() const {
    Point A = points_[0];
    Point B = points_[1];
    Point C = points_[2];

    double denom =
        2 * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));

    double x_coord = ((std::pow(A.x, 2) + std::pow(A.y, 2)) * (B.y - C.y) +
                      (std::pow(B.x, 2) + std::pow(B.y, 2)) * (C.y - A.y) +
                      (std::pow(C.x, 2) + std::pow(C.y, 2)) * (A.y - B.y)) /
                     denom;
    double y_coord = ((std::pow(A.x, 2) + std::pow(A.y, 2)) * (C.x - B.x) +
                      (std::pow(B.x, 2) + std::pow(B.y, 2)) * (A.x - C.x) +
                      (std::pow(C.x, 2) + std::pow(C.y, 2)) * (B.x - A.x)) /
                     denom;
    return Point(x_coord, y_coord);
  }

  Point orthocenter() const {
    return Point(centroid() + Vector(outcenter(), centroid()) * 2);
  }

  Line EulerLine() const {
    return Line(orthocenter(), centroid());
  }

  Circle ninePointsCircle() const {
    Circle ans = circumscribedCircle();
    ans.scale(orthocenter(), 1. / 2);
    return ans;
  }

  double area() const final {
    ///
  }
};

/*

void print() {
    for (const Point& point : points_) {
      std::cout << point.x << ' ' << point.y << '\n';
    }
  }
*/