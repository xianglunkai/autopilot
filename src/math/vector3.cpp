
#pragma GCC optimize("O2")
#include "vector3.h"
#include "Location.h"
#include "common_math.h"
#include "math_utils.h"

using namespace LOC;

// vector cross product
template <typename T>
Vector3<T> Vector3<T>::operator %(const Vector3<T> &v) const
{
    Vector3<T> temp(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
    return temp;
}

// dot product
template <typename T>
T Vector3<T>::operator *(const Vector3<T> &v) const
{
    return x*v.x + y*v.y + z*v.z;
}

template <typename T>
float Vector3<T>::length(void) const
{
    return norm(x, y, z);
}

template <typename T>
Vector3<T> &Vector3<T>::operator *=(const T num)
{
    x*=num; y*=num; z*=num;
    return *this;
}

template <typename T>
Vector3<T> &Vector3<T>::operator /=(const T num)
{
    x /= num; y /= num; z /= num;
    return *this;
}

template <typename T>
Vector3<T> &Vector3<T>::operator -=(const Vector3<T> &v)
{
    x -= v.x; y -= v.y; z -= v.z;
    return *this;
}

template <typename T>
Vector3<T> &Vector3<T>::operator +=(const Vector3<T> &v)
{
    x+=v.x; y+=v.y; z+=v.z;
    return *this;
}

template <typename T>
Vector3<T> Vector3<T>::operator /(const T num) const
{
    return Vector3<T>(x/num, y/num, z/num);
}

template <typename T>
Vector3<T> Vector3<T>::operator *(const T num) const
{
    return Vector3<T>(x*num, y*num, z*num);
}

template <typename T>
Vector3<T> Vector3<T>::operator -(const Vector3<T> &v) const
{
    return Vector3<T>(x-v.x, y-v.y, z-v.z);
}

template <typename T>
Vector3<T> Vector3<T>::operator +(const Vector3<T> &v) const
{
    return Vector3<T>(x+v.x, y+v.y, z+v.z);
}

template <typename T>
Vector3<T> Vector3<T>::operator -(void) const
{
    return Vector3<T>(-x,-y,-z);
}

template <typename T>
bool Vector3<T>::operator ==(const Vector3<T> &v) const
{
    return (is_equal(x,v.x) && is_equal(y,v.y) && is_equal(z,v.z));
}

template <typename T>
bool Vector3<T>::operator !=(const Vector3<T> &v) const
{
    return (!is_equal(x,v.x) || !is_equal(y,v.y) || !is_equal(z,v.z));
}

template <typename T>
float Vector3<T>::angle(const Vector3<T> &v2) const
{
    const float len = this->length() * v2.length();
    if (len <= 0) {
        return 0.0f;
    }
    const float cosv = ((*this)*v2) / len;
    if (fabsf(cosv) >= 1) {
        return 0.0f;
    }
    return acosf(cosv);
}


// extrapolate position given bearing and pitch (in degrees) and distance
template <typename T>
void Vector3<T>::offset_bearing(float bearing, float pitch, float distance)
{
    y += cosf(math::radians(pitch)) * sinf(math::radians(bearing)) * distance;
    x += cosf(math::radians(pitch)) * cosf(math::radians(bearing)) * distance;
    z += sinf(math::radians(pitch)) * distance;
}

// distance from the tip of this vector to a line segment specified by two vectors
template <typename T>
float Vector3<T>::distance_to_segment(const Vector3<T> &seg_start, const Vector3<T> &seg_end) const
{
    // triangle side lengths
    const float a = (*this-seg_start).length();
    const float b = (seg_start-seg_end).length();
    const float c = (seg_end-*this).length();

    // protect against divide by zero later
    if (is_zero(b)) {
        return 0.0f;
    }

    // semiperimeter of triangle
    const float s = (a+b+c) * 0.5f;

    float area_squared = s*(s-a)*(s-b)*(s-c);
    // area must be constrained above 0 because a triangle could have 3 points could be on a line and float rounding could push this under 0
    if (area_squared < 0.0f) {
        area_squared = 0.0f;
    }
    const float area = sqrtf(area_squared);
    return 2.0f*area/b;
}

// Shortest distance between point(p) to a point contained in the line segment defined by w1,w2
template <typename T>
float Vector3<T>::closest_distance_between_line_and_point(const Vector3<T> &w1, const Vector3<T> &w2, const Vector3<T> &p)
{    
    const Vector3<T> nearest = point_on_line_closest_to_other_point(w1, w2, p);
    const float dist = (nearest - p).length();
    return dist;
}

// Point in the line segment defined by w1,w2 which is closest to point(p)
// this is based on the explanation given here: www.fundza.com/vectors/point2line/index.html
template <typename T>
Vector3<T> Vector3<T>::point_on_line_closest_to_other_point(const Vector3<T> &w1, const Vector3<T> &w2, const Vector3<T> &p)
{   
    const Vector3<T> line_vec = w2-w1;
    const Vector3<T> p_vec = p - w1;
    
    const float line_vec_len = line_vec.length();
    // protection against divide by zero
    if(fabs(line_vec_len) < FLT_EPSILON) {
        return {0.0f, 0.0f, 0.0f};
    }

    const float scale = 1/line_vec_len;
    const Vector3<T> unit_vec = line_vec * scale;
    const Vector3<T> scaled_p_vec = p_vec * scale;

    float dot_product = unit_vec * scaled_p_vec;
    dot_product = math::Clamp(dot_product,0.0f,1.0f); 
 
    const Vector3<T> closest_point = line_vec * dot_product;
    return (closest_point + w1);
}

// Shortest distance between two line segments
// This implementation is borrowed from: http://geomalgorithms.com/a07-_distance.html
// INPUT: 4 points corresponding to start and end of two line segments
// OUTPUT: shortest distance, and closest point on segment 2, from segment 1, gets passed on reference as "intersection" 
template <typename T>
float Vector3<T>::segment_to_segment_dist(const Vector3<T>& seg1_start, const Vector3<T>& seg1_end, const Vector3<T>& seg2_start, const Vector3<T>& seg2_end, Vector3<T>& intersection)
{
    // direction vectors
    const Vector3<T> line1 = seg1_end - seg1_start;
    const Vector3<T> line2 = seg2_end - seg2_start;

    const Vector3<T> diff = seg1_start - seg2_start;

    const float a = line1*line1;
    const float b = line1*line2;
    const float c = line2*line2;
    const float d = line1*diff;
    const float e = line2*diff;

    const float discriminant = (a*c) - (b*b);
    float sc, sN, sD = discriminant;       // sc = sN / sD, default sD = D >= 0
    float tc, tN, tD = discriminant;       // tc = tN / tD, default tD = D >= 0 
    
    if (discriminant < FLT_EPSILON) {
        sN = 0.0;         // force using point seg1_start on line 1
        sD = 1.0;         // to prevent possible division by 0.0 later
        tN = e;
        tD = c;
    } else {                 
        // get the closest points on the infinite lines
        sN = (b*e - c*d);
        tN = (a*e - b*d);
        if (sN < 0.0) {        
            // sc < 0 => the s=0 edge is visible
            sN = 0.0;
            tN = e;
            tD = c;
        } else if (sN > sD) {  
            // sc > 1  => the s=1 edge is visible
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0.0) {            
        // tc < 0 => the t=0 edge is visible
        tN = 0.0;
        // recompute sc for this edge
        if (-d < 0.0) {
            sN = 0.0;
        } else if (-d > a) {
            sN = sD;
        } else {
            sN = -d;
            sD = a;
        }
    } else if (tN > tD) {      
        // tc > 1  => the t=1 edge is visible
        tN = tD;
        // recompute sc for this edge
        if ((-d + b) < 0.0) {
            sN = 0;
        } else if ((-d + b) > a) {
            sN = sD;
        } else {
            sN = (-d +  b);
            sD = a;
        }
    }
    // finally do the division to get sc and tc
    sc = (fabsf(sN) < FLT_EPSILON ? 0.0 : sN / sD);
    tc = (fabsf(tN) < FLT_EPSILON ? 0.0 : tN / tD);

    const Vector3<T> closest_line_segment = diff + (line1*sc) - (line2*tc);
    const float len = closest_line_segment.length();
    intersection = seg2_start + line2*tc;
    return len;
}

// define for float and double
template class Vector3<float>;
template class Vector3<double>;

// define needed ops for Vector3l
template Vector3<int32_t> &Vector3<int32_t>::operator +=(const Vector3<int32_t> &v);
