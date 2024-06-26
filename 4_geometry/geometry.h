#include <algorithm>
#include <cmath>

class Line;

namespace my {
const double kAccuracy = 1e-6;

class Vector;

bool double_equal(double first, double second) {
  return std::abs(first - second) < kAccuracy;
}
}

class Point {
 public:
  double x;
  double y;

  Point() = default;

  Point(double x, double y) : x(x), y(y) {}

  Point(const my::Vector&);

  bool operator==(const Point& other) const {
    return my::double_equal(x, other.x) && my::double_equal(y, other.y);
  }

  bool operator!=(const Point& other) const {
    return !(*this == other);
  }

  Point symmetrical(const Line& line) const;

  double distance(const Point& other) const {
    return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
  }
};

namespace my {
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
    return std::sqrt(std::pow(x_coord, 2) + std::pow(y_coord, 2));
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

  Vector& operator*=(double k) {
    x_coord *= k;
    y_coord *= k;
    return *this;
  }

  bool is_collinear(const Vector& other) const {
    return my::double_equal(x_coord * other.y_coord, y_coord * other.x_coord);
  }

  Vector normed() const {
    return *this * (1. / abs());
  }

  bool operator==(const Vector& other) const {
    return is_collinear(other) && my::double_equal(other.abs(), abs());
  }

  bool operator!=(const Vector& other) const {
    return !(*this == other);
  }
};

double ScalarProduct(const Vector& first, const Vector& second) {
  return first.x_coord * second.x_coord + first.y_coord * second.y_coord;
}
}

class Line {
 public:
  Point point;
  my::Vector dir;

  Line(const Point& first, const Point& second)
      : point(first), dir(my::Vector(first, second)) {}

  Line(double k, double b_)
      : point(Point(0, b_)),
        dir(my::Vector(std::cos(std::atan(k)), std::sin(std::atan(k)))) {}

  Line(const Point& point, double k)
      : point(point),
        dir(my::Vector(std::cos(std::atan(k)), std::sin(std::atan(k)))) {}

  Line(const Point& point, const my::Vector& vec) : point(point), dir(vec) {}

  bool operator==(const Line& other) const {
    return my::Vector(point, other.point).is_collinear(dir) &&
           dir.is_collinear(other.dir);
  }

  bool operator!=(const Line& other) const {
    return !(*this == other);
  }
};

Point operator+(const Point& point, const my::Vector& vec) {
  return Point(point.x + vec.x_coord, point.y + vec.y_coord);
}

Point Point::symmetrical(const Line& line) const {
  my::Vector r1 = *this;
  my::Vector r0 = line.point;
  my::Vector projection = line.dir * my::ScalarProduct(line.dir, r1 - r0) *
                          (1 / (line.dir.abs() * line.dir.abs()));
  my::Vector ans = r1 - (r1 - r0 - projection) * 2;
  return Point(0, 0) + ans;
}

Point::Point(const my::Vector& other) : x(other.x_coord), y(other.y_coord) {}

class Shape {
 public:
  virtual double perimeter() const = 0;

  virtual double area() const = 0;

  virtual bool operator==(const Shape&) const = 0;

  bool operator!=(const Shape& other) const {
    return !(*this == other);
  }

  bool isCongruentTo(const Shape&) const;

  bool isSimilarTo(const Shape&) const;

  virtual bool containsPoint(const Point&) const = 0;

  virtual void rotate(const Point&, double) = 0;

  virtual void reflect(const Point&) = 0;

  virtual void reflect(const Line&) = 0;

  virtual void scale(const Point&, double) = 0;

  virtual ~Shape() {}
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
    my::Vector F1_F2_dir = my::Vector(F1_, F2_);

    return F1_ + F1_F2_dir * (1. / 2);
  }

  std::pair<Line, Line> directrices() const {
    double e = eccentricity();

    my::Vector F1_F2_dir = my::Vector(F1_, F2_);
    my::Vector d_dir = my::Vector(-F1_F2_dir.y_coord, F1_F2_dir.x_coord);

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
    return F1_.distance(point) + F2_.distance(point) <= 2 * a_ + my::kAccuracy;
  }

  void rotate(const Point& point, double angle) final {
    F1_ = point + my::Vector(point, F1_).rotated(angle);
    F2_ = point + my::Vector(point, F2_).rotated(angle);
  }

  void reflect(const Point& point) final {
    F1_ = point + -my::Vector(point, F1_);
    F2_ = point + -my::Vector(point, F2_);
  }

  void reflect(const Line& line) final {
    F1_ = F1_.symmetrical(line);
    F2_ = F2_.symmetrical(line);
  }

  void scale(const Point& point, double coef) final {
    F1_ = point + my::Vector(point, F1_) * coef;
    F2_ = point + my::Vector(point, F2_) * coef;
    a_ *= std::abs(coef);
    b_ *= std::abs(coef);
  }

  bool operator==(const Shape& other) const final {
    const Ellipse* cnt_other = dynamic_cast<const Ellipse*>(&other);

    if (cnt_other != nullptr) {
      auto this_focuses = focuses();
      auto other_focuses = cnt_other->focuses();
      return (this_focuses == other_focuses ||
              (std::swap(
                   this_focuses.first,
                   this_focuses.second),  // офигеть я чё придумал, запятую
                                          // использовал первый раз
               this_focuses) == other_focuses) &&
             my::double_equal(eccentricity(), cnt_other->eccentricity());
    }
    return false;
  }
};

class Circle : public Shape {
 private:
  Point center_;
  double r_;

 public:
  Circle(const Point& point, double r_) : center_(point), r_(r_) {}

  Point center() const {
    return center_;
  }

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
    return center_.distance(point) <= r_ + my::kAccuracy;
  }

  void rotate(const Point& point, double angle) final {
    center_ = point + my::Vector(point, center_).rotated(angle);
  }

  void reflect(const Point& point) final {
    center_ = point + -my::Vector(point, center_);
  }

  void reflect(const Line& line) final {
    center_ = center_.symmetrical(line);
  }

  void scale(const Point& point, double coef) final {
    center_ = point + my::Vector(point, center_) * coef;
    r_ *= std::abs(coef);
  }

  bool operator==(const Shape& other) const final {
    const Circle* cnt_other = dynamic_cast<const Circle*>(&other);

    if (cnt_other != nullptr) {
      return center_ == cnt_other->center_ &&
             my::double_equal(r_, cnt_other->r_);
    }
    return false;
  }
};

class Polygon : public Shape {
 private:
  bool is_z_sign_posotive(const Point& first, const Point& second,
                          const Point& third) const {
    my::Vector v1(first, second);
    my::Vector v2(second, third);

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

  std::vector<Point> getVertices() const {
    return points_;
  }

  bool isConvex() const {
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

  bool containsPoint(const Point& point) const override {
    bool result = false;

    for (size_t i = 0; i < points_.size(); ++i) {
      size_t next_i = (i + 1) % points_.size();
      if (((points_[i].y < point.y && points_[next_i].y >= point.y) ||
           // Check if the point is to the left of the line between the two
           // points
           (points_[next_i].y < point.y && points_[i].y >= point.y)) &&
          // Check if the point is to the left of the line defined by the two
          // points
          (points_[i].x + (point.y - points_[i].y) /
                              (points_[next_i].y - points_[i].y) *
                              (points_[next_i].x - points_[i].x) <
           point.x)) {
        // Invert result if point is outside of the polygon
        result = !result;
      }
    }

    return result;
  }

  void rotate(const Point& point, double angle) final {
    for (size_t i = 0; i < points_.size(); ++i) {
      points_[i] = point + my::Vector(point, points_[i]).rotated(angle);
    }
  }

  void reflect(const Point& point) final {
    for (size_t i = 0; i < points_.size(); ++i) {
      points_[i] = point + -my::Vector(point, points_[i]);
    }
  }

  void reflect(const Line& line) final {
    for (size_t i = 0; i < points_.size(); ++i) {
      points_[i] = points_[i].symmetrical(line);
    }
  }

  void scale(const Point& point, double coef) final {
    for (size_t i = 0; i < points_.size(); ++i) {
      points_[i] = point + my::Vector(point, points_[i]) * coef;
    }
  }

  bool operator==(const Shape& other) const final {
    const Polygon* pol_others = dynamic_cast<const Polygon*>(&other);

    if (pol_others != nullptr) {
      std::vector<Point> this_points = getVertices();
      std::vector<Point> this_points_rev = this_points;
      std::reverse(this_points_rev.begin(), this_points_rev.end());
      std::vector<Point> other_points = pol_others->getVertices();

      if (this_points.size() != other_points.size()) {
        return false;
      }

      for (size_t i = 0; i < this_points.size(); ++i) {
        bool all_good_1 = true;
        bool all_good_2 = true;

        for (size_t j = 0; j < this_points.size(); ++j) {
          if (this_points[(i + j) % this_points.size()] != other_points[j]) {
            all_good_1 = false;
          }

          if (this_points_rev[(i + j) % this_points.size()] !=
              other_points[j]) {
            all_good_2 = false;
          }

          if (!all_good_1 && !all_good_1) {
            break;
          }
        }
        if (all_good_1 || all_good_2) {
          return true;
        }
      }

      return false;
    }

    return false;
  }
};

class Rectangle : public Polygon {
 public:
  Rectangle(const Point& P1, const Point& P3, double coef) {
    Point center = (my::Vector(P1) + my::Vector(P3)) * 0.5;
    my::Vector cnt = my::Vector(P1) + -my::Vector(center);
    Point P2(my::Vector(center) + cnt.rotated(-2 * std::atan(coef)));
    Point P4 = P3 + -my::Vector(P1, P2);
    points_ = {P1, P2, P3, P4};
  }

  Point center() const {
    return Point((my::Vector(points_[0]) + my::Vector(points_[2])) * 0.5);
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
    my::Vector v1(points_[0], points_[1]);
    my::Vector v2(points_[0], points_[2]);
    double angle = std::acos(my::ScalarProduct(v1, v2) / (v1.abs() * v2.abs()));
    double r = 2 * area() / perimeter();

    return points_[0] + (v1.normed() + v2.normed()).normed() *
                            (r / std::sin(angle / 2));  ///???
  }

  Circle inscribedCircle() const {
    return Circle(inCenter(), 2 * area() / perimeter());
  }

  Point centroid() const {
    return points_[0] + (my::Vector(points_[0], points_[1]) +
                         my::Vector(points_[0], points_[2])) *
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
    return Point(
        centroid() +
        my::Vector(outcenter(), centroid()) *
            2);  // Point(centroid()) + my::Vector(out, cen).scale(-1./2)
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
    double a = points_[0].distance(points_[1]);
    double b = points_[0].distance(points_[2]);
    double c = points_[1].distance(points_[2]);
    double p = (a + b + c) / 2;

    return std::sqrt(p * (p - a) * (p - b) * (p - c));
  }
};

bool Shape::isCongruentTo(const Shape& other) const {
  const Ellipse* cnt_this = dynamic_cast<const Ellipse*>(this);
  const Ellipse* cnt_other = dynamic_cast<const Ellipse*>(&other);

  if (cnt_this != nullptr && cnt_other != nullptr) {
    auto this_focuses = cnt_this->focuses();
    auto other_focuses = cnt_other->focuses();
    return (my::Vector(this_focuses.first, other_focuses.first)
                .is_collinear(
                    my::Vector(this_focuses.second, other_focuses.second)) ||
            my::Vector(this_focuses.second, other_focuses.first)
                .is_collinear(
                    my::Vector(this_focuses.first, other_focuses.second))) &&
           my::double_equal(cnt_this->eccentricity(),
                            cnt_other->eccentricity());
  }

  const Circle* cir_this = dynamic_cast<const Circle*>(this);
  const Circle* cir_other = dynamic_cast<const Circle*>(&other);

  if (cir_this != nullptr && cir_other != nullptr) {
    return my::double_equal(cir_other->radius(), cir_this->radius());
  }

  const Polygon* pol_this = dynamic_cast<const Polygon*>(this);
  const Polygon* pol_others = dynamic_cast<const Polygon*>(&other);

  if (pol_others != nullptr && pol_this != nullptr) {
    std::vector<Point> this_points = pol_this->getVertices();
    std::vector<Point> this_points_rev = this_points;
    std::reverse(this_points_rev.begin(), this_points_rev.end());
    std::vector<Point> other_points = pol_others->getVertices();

    if (this_points.size() != other_points.size()) {
      return false;
    }

    for (size_t i = 0; i < this_points.size(); ++i) {
      bool all_good_1 = true;
      bool all_good_2 = true;

      for (size_t j = 0; j < this_points.size(); ++j) {
        my::Vector v1 =
            my::Vector(this_points[(i + j) % this_points.size()],
                       this_points[(i + j + 1) % this_points.size()]);
        my::Vector v2 =
            my::Vector(this_points[(i + j + 1) % this_points.size()],
                       this_points[(i + j + 2) % this_points.size()]);

        my::Vector v3 = my::Vector(other_points[j % other_points.size()],
                                   other_points[(j + 1) % other_points.size()]);
        my::Vector v4 = my::Vector(other_points[(j + 1) % other_points.size()],
                                   other_points[(j + 2) % other_points.size()]);

        my::Vector v5 =
            my::Vector(this_points_rev[(i + j) % this_points_rev.size()],
                       this_points_rev[(i + j + 1) % this_points_rev.size()]);
        my::Vector v6 =
            my::Vector(this_points_rev[(i + j + 1) % this_points_rev.size()],
                       this_points_rev[(i + j + 2) % this_points_rev.size()]);

        if (!my::double_equal(v1.abs(), v3.abs()) ||
            !my::double_equal(my::ScalarProduct(v1, v2),
                              my::ScalarProduct(v3, v4))) {
          all_good_1 = false;
        }

        if (!my::double_equal(v5.abs(), v3.abs()) ||
            !my::double_equal(my::ScalarProduct(v5, v6),
                              my::ScalarProduct(v3, v4))) {
          all_good_2 = false;
        }

        if (!all_good_1 && !all_good_1) {
          break;
        }
      }
      if (all_good_1 || all_good_2) {
        return true;
      }
    }

    return false;
  }

  return false;
}

bool Shape::isSimilarTo(const Shape& other) const {
  const Ellipse* cnt_this = dynamic_cast<const Ellipse*>(this);
  const Ellipse* cnt_other = dynamic_cast<const Ellipse*>(&other);

  if (cnt_this != nullptr && cnt_other != nullptr) {
    return my::double_equal(cnt_this->eccentricity(),
                            cnt_other->eccentricity());
  }

  const Circle* cir_this = dynamic_cast<const Circle*>(this);
  const Circle* cir_other = dynamic_cast<const Circle*>(&other);

  if (cir_this != nullptr && cir_other != nullptr) {
    return true;
  }

  const Polygon* pol_this = dynamic_cast<const Polygon*>(this);
  const Polygon* pol_others = dynamic_cast<const Polygon*>(&other);

  if (pol_others != nullptr && pol_this != nullptr) {
    Polygon copy_this = *dynamic_cast<const Polygon*>(this);

    double s = other.perimeter() / perimeter();
    copy_this.scale(Point(0, 0), s);

    return copy_this.isCongruentTo(other);
  }

  return false;
}
