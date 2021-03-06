
#pragma GCC optimize("O2")

#include "vector2.h"
#include "Location.h"
#include "common_math.h"

using namespace LOC;

template <typename T>
float Vector2<T>::length_squared() const
{
    return (float)(x*x + y*y);
}

template <typename T>
float Vector2<T>::length(void) const
{
    return norm(x, y);
}

// dot product
template <typename T>
T Vector2<T>::operator *(const Vector2<T> &v) const
{
    return x*v.x + y*v.y;
}

// cross product
template <typename T>
T Vector2<T>::operator %(const Vector2<T> &v) const
{
    return x*v.y - y*v.x;
}

template <typename T>
Vector2<T> &Vector2<T>::operator *=(const T num)
{
    x*=num; y*=num;
    return *this;
}

template <typename T>
Vector2<T> &Vector2<T>::operator /=(const T num)
{
    x /= num; y /= num;
    return *this;
}

template <typename T>
Vector2<T> &Vector2<T>::operator -=(const Vector2<T> &v)
{
    x -= v.x; y -= v.y;
    return *this;
}

template <typename T>
bool Vector2<T>::is_nan(void) const
{
    return isnan(x) || isnan(y);
}

template <typename T>
bool Vector2<T>::is_inf(void) const
{
    return isinf(x) || isinf(y);
}

template <typename T>
Vector2<T> &Vector2<T>::operator +=(const Vector2<T> &v)
{
    x+=v.x; y+=v.y;
    return *this;
}

template <typename T>
Vector2<T> Vector2<T>::operator /(const T num) const
{
    return Vector2<T>(x/num, y/num);
}

template <typename T>
Vector2<T> Vector2<T>::operator *(const T num) const
{
    return Vector2<T>(x*num, y*num);
}

template <typename T>
Vector2<T> Vector2<T>::operator -(const Vector2<T> &v) const
{
    return Vector2<T>(x-v.x, y-v.y);
}

template <typename T>
Vector2<T> Vector2<T>::operator +(const Vector2<T> &v) const
{
    return Vector2<T>(x+v.x, y+v.y);
}

template <typename T>
Vector2<T> Vector2<T>::operator -(void) const
{
    return Vector2<T>(-x,-y);
}

template <typename T>
bool Vector2<T>::operator ==(const Vector2<T> &v) const
{
    return (is_equal(x,v.x) && is_equal(y,v.y));
}

template <typename T>
bool Vector2<T>::operator !=(const Vector2<T> &v) const
{
    return (!is_equal(x,v.x) || !is_equal(y,v.y));
}

template <typename T>
float Vector2<T>::angle(const Vector2<T> &v2) const
{
    const float len = this->length() * v2.length();
    if (len <= 0) {
        return 0.0f;
    }
    const float cosv = ((*this)*v2) / len;
    if (cosv >= 1) {
        return 0.0f;
    }
    if (cosv <= -1) {
        return M_PI;
    }
    return acosf(cosv);
}

template <typename T>
float Vector2<T>::angle(void) const
{
    return M_PI_2 + atan2f(-x, y);
}

// find the intersection between two line segments
// returns true if they intersect, false if they do not
// the point of intersection is returned in the intersection argument
template <typename T>
bool Vector2<T>::segment_intersection(const Vector2<T>& seg1_start, const Vector2<T>& seg1_end, const Vector2<T>& seg2_start, const Vector2<T>& seg2_end, Vector2<T>& intersection)
{
    // implementation borrowed from http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
    const Vector2<T> r1 = seg1_end - seg1_start;
    const Vector2<T> r2 = seg2_end - seg2_start;
    const Vector2<T> ss2_ss1 = seg2_start - seg1_start;
    const float r1xr2 = r1 % r2;
    const float q_pxr = ss2_ss1 % r1;
    if (fabsf(r1xr2) < FLT_EPSILON) {
        // either collinear or parallel and non-intersecting
        return false;
    } else {
        // t = (q - p) * s / (r * s)
        // u = (q - p) * r / (r * s)
        const float t = (ss2_ss1 % r2) / r1xr2;
        const float u = q_pxr / r1xr2;
        if ((u >= 0) && (u <= 1) && (t >= 0) && (t <= 1)) {
            // lines intersect
            // t can be any non-negative value because (p, p + r) is a ray
            // u must be between 0 and 1 because (q, q + s) is a line segment
            intersection = seg1_start + (r1*t);
            return true;
        } else {
            // non-parallel and non-intersecting
            return false;
        }
    }
}

// find the intersection between a line segment and a circle
// returns true if they intersect and intersection argument is updated with intersection closest to seg_start
// solution adapted from http://stackoverflow.com/questions/1073336/circle-line-segment-collision-detection-algorithm
template <typename T>
bool Vector2<T>::circle_segment_intersection(const Vector2<T>& seg_start, const Vector2<T>& seg_end, const Vector2<T>& circle_center, float radius, Vector2<T>& intersection)
{
    // calculate segment start and end as offsets from circle's center
    const Vector2f seg_start_local = seg_start - circle_center;

    // calculate vector from start to end
    const Vector2f seg_end_minus_start = seg_end - seg_start;

    const float a = SQ(seg_end_minus_start.x) + SQ(seg_end_minus_start.y);
    const float b = 2 * ((seg_end_minus_start.x * seg_start_local.x) + (seg_end_minus_start.y * seg_start_local.y));
    const float c = SQ(seg_start_local.x) + SQ(seg_start_local.y) - SQ(radius);

    // check for invalid data
    if (is_zero(a) || isnan(a) || isnan(b) || isnan(c)) {
       return false;
    }

    const float delta = SQ(b) - (4.0f * a * c);

    if (isnan(delta)) {
       return false;
    }

    // check for invalid delta (i.e. discriminant)
    if (delta < 0.0f) {
        return false;
    }

    const float delta_sqrt = sqrtf(delta);
    const float t1 = (-b + delta_sqrt) / (2.0f * a);
    const float t2 = (-b - delta_sqrt) / (2.0f * a);

    // Three hit cases:
    //          -o->             --|-->  |            |  --|->
    // Impale(t1 hit,t2 hit), Poke(t1 hit,t2>1), ExitWound(t1<0, t2 hit),

    // Three miss cases:
    //       ->  o                     o ->              | -> |
    // FallShort (t1>1,t2>1), Past (t1<0,t2<0), CompletelyInside(t1<0, t2>1)

    // intersection = new Vector3(E.x + t1 * d.x, secondPoint.y, E.y + t1 * d.y);
    //   intersection.x = seg_start.x + t1 * seg_end_minus_start.x;
    //   intersection.y = seg_start.y + t1 * seg_end_minus_start.y;

    if ((t1 >= 0.0f) && (t1 <= 1.0f)) {
        // t1 is the intersection, and it is closer than t2 (since t1 uses -b - discriminant)
        // Impale, Poke
        intersection = seg_start + (seg_end_minus_start * t1);
        return true;
    }

    // here t1 did not intersect so we are either started inside the sphere or completely past it
    if ((t2 >= 0.0f) && (t2 <= 1.0f)) {
        // ExitWound
        intersection = seg_start + (seg_end_minus_start * t2);
        return true;
    }

    // no intersection: FallShort, Past or CompletelyInside
    return false;
}

// normalizes this vector
template <typename T>
void Vector2<T>::normalize()
{
    *this /= length();
}

// returns the normalized vector
template <typename T>
Vector2<T> Vector2<T>::normalized() const
{
    return *this/length();
}

// reflects this vector about n
template <typename T>
void Vector2<T>::reflect(const Vector2<T> &n)
{
    const Vector2<T> orig(*this);
    project(n);
    *this = *this*2 - orig;
}

// projects this vector onto v
template <typename T>
void Vector2<T>::project(const Vector2<T> &v)
{
    *this= v * (*this * v)/(v*v);
}

// returns this vector projected onto v
template <typename T>
Vector2<T> Vector2<T>::projected(const Vector2<T> &v)
{
    return v * (*this * v)/(v*v);
}

// extrapolate position given bearing (in degrees) and distance
template <typename T>
void Vector2<T>::offset_bearing(float bearing, float distance)
{
    x += cosf(math::radians(bearing)) * distance;
    y += sinf(math::radians(bearing)) * distance;
}

// given a position pos_delta and a velocity v1 produce a vector
// perpendicular to v1 maximising distance from p1
template <typename T>
Vector2<T> Vector2<T>::perpendicular(const Vector2<T> &pos_delta, const Vector2<T> &v1)
{
    const Vector2<T> perpendicular1 = Vector2<T>(-v1[1], v1[0]);
    const Vector2<T> perpendicular2 = Vector2<T>(v1[1], -v1[0]);
    const T d1 = perpendicular1 * pos_delta;
    const T d2 = perpendicular2 * pos_delta;
    if (d1 > d2) {
        return perpendicular1;
    }
    return perpendicular2;
}

/*
 * Returns the point closest to p on the line segment (v,w).
 *
 * Comments and implementation taken from
 * http://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
 */
template <typename T>
Vector2<T> Vector2<T>::closest_point(const Vector2<T> &p, const Vector2<T> &v, const Vector2<T> &w)
{
    // length squared of line segment
    const float l2 = (v - w).length_squared();
    if (l2 < FLT_EPSILON) {
        // v == w case
        return v;
    }
    // Consider the line extending the segment, parameterized as v + t (w - v).
    // We find projection of point p onto the line.
    // It falls where t = [(p-v) . (w-v)] / |w-v|^2
    // We clamp t from [0,1] to handle points outside the segment vw.
    const float t = ((p - v) * (w - v)) / l2;
    if (t <= 0) {
        return v;
    } else if (t >= 1) {
        return w;
    } else {
        return v + (w - v)*t;
    }
}

/*
 * Returns the point closest to p on the line segment (0,w).
 *
 * this is a simplification of closest point with a general segment, with v=(0,0)
 */
template <typename T>
Vector2<T> Vector2<T>::closest_point(const Vector2<T> &p, const Vector2<T> &w)
{
    // length squared of line segment
    const float l2 = w.length_squared();
    if (l2 < FLT_EPSILON) {
        // v == w case
        return w;
    }
    const float t = (p * w) / l2;
    if (t <= 0) {
        return Vector2<T>(0,0);
    } else if (t >= 1) {
        return w;
    } else {
        return w*t;
    }
}

// closest distance between a line segment and a point
// https://stackoverflow.com/questions/2824478/shortest-distance-between-two-line-segments
template <typename T>
float Vector2<T>::closest_distance_between_line_and_point_squared(const Vector2<T> &w1,
                                                                  const Vector2<T> &w2,
                                                                  const Vector2<T> &p)
{
    return closest_distance_between_radial_and_point_squared(w2-w1, p-w1);
}

// w1 and w2 define a line segment
// p is a point
// returns the closest distance between the line segment and the point
template <typename T>
float Vector2<T>::closest_distance_between_line_and_point(const Vector2<T> &w1,
                                                          const Vector2<T> &w2,
                                                          const Vector2<T> &p)
{
    return sqrtf(closest_distance_between_line_and_point_squared(w1, w2, p));
}

// a1->a2 and b2->v2 define two line segments
// returns the square of the closest distance between the two line segments
// see https://stackoverflow.com/questions/2824478/shortest-distance-between-two-line-segments
template <typename T>
float Vector2<T>::closest_distance_between_lines_squared(const Vector2<T> &a1,
                                                         const Vector2<T> &a2,
                                                         const Vector2<T> &b1,
                                                         const Vector2<T> &b2)
{
    const float dist1 = Vector2<T>::closest_distance_between_line_and_point_squared(b1,b2,a1);
    const float dist2 = Vector2<T>::closest_distance_between_line_and_point_squared(b1,b2,a2);
    const float dist3 = Vector2<T>::closest_distance_between_line_and_point_squared(a1,a2,b1);
    const float dist4 = Vector2<T>::closest_distance_between_line_and_point_squared(a1,a2,b2);
    const float m1 = std::fmin(dist1,dist2);
    const float m2 = std::fmin(dist3,dist4);
    return std::fmin(m1,m2);
}

// w defines a line segment from the origin
// p is a point
// returns the square of the closest distance between the radial and the point
template <typename T>
float Vector2<T>::closest_distance_between_radial_and_point_squared(const Vector2<T> &w,
                                                                   const Vector2<T> &p)
{
    const Vector2<T> closest = closest_point(p, w);
    return (closest - p).length_squared();
}

// w defines a line segment from the origin
// p is a point
// returns the closest distance between the radial and the point
template <typename T>
float Vector2<T>::closest_distance_between_radial_and_point(const Vector2<T> &w,
                                                            const Vector2<T> &p)
{
    return sqrtf(closest_distance_between_radial_and_point_squared(w,p));
}

// rotate vector by angle in radians
template <typename T>
void Vector2<T>::rotate(float angle_rad)
{
    const float cs = cosf(angle_rad);
    const float sn = sinf(angle_rad);
    float rx = x * cs - y * sn;
    float ry = x * sn + y * cs;
    x = rx;
    y = ry;
}

// only define for float
template class Vector2<float>;

// define some ops for int and long
template bool Vector2<long>::operator ==(const Vector2<long> &v) const;
template bool Vector2<long>::operator !=(const Vector2<long> &v) const;
template bool Vector2<int>::operator ==(const Vector2<int> &v) const;
template bool Vector2<int>::operator !=(const Vector2<int> &v) const;
