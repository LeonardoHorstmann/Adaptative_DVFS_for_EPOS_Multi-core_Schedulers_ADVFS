// EPOS Geometry Utility Declarations

#include <utility/math.h>

__BEGIN_UTIL

template<typename T, unsigned int dimensions>
struct Point;

template<typename T>
struct Point<T, 2>
{
private:
    typedef typename IF<EQUAL<char, T>::Result, int, T>::Result Print_Type;

public:
    typedef T Distance;

    Point(const T & xi = 0, const T & yi = 0): x(xi), y(yi) {}

    Distance operator-(const Point<T, 2> & p) const {
        T xx = p.x - x;
        T yy = p.y - y;
        return sqrt(xx*xx + yy*yy);
    }

    bool operator==(const Point & p) const { return x == p.x and y == p.y; }
    bool operator!=(const Point & p) const { return !(*this == p); }

    friend OStream & operator<<(OStream & os, const Point<T, 2> & c) {
        os << "{" << static_cast<Print_Type>(c.x) << "," << static_cast<Print_Type>(c.y) << "}";
        return os;
    }

    T x, y;
};

template<typename T>
struct Point<T, 3>
{
private:
    typedef typename IF<EQUAL<char, T>::Result, int, T>::Result Print_Type;

public:
    typedef T Distance;

    Point(const T & xi = 0, const T & yi = 0, const T & zi = 0): x(xi), y(yi), z(zi) {}

    Distance operator-(const Point & p) const {
        T xx = p.x - x;
        T yy = p.y - y;
        T zz = p.z - z;
        return sqrt(xx*xx + yy*yy + zz*zz);
    }

    bool operator==(const Point & p) const { return x == p.x and y == p.y and z == p.z; }
    bool operator!=(const Point & p) const { return !(*this == p); }

    friend OStream & operator<<(OStream & os, const Point & c) {
        os << "(" << Print_Type(c.x) << "," << Print_Type(c.y) << "," << static_cast<Print_Type>(c.z) << ")";
        return os;
    }

    T x, y, z;
};

template<typename T>
struct Sphere
{
private:
    typedef typename IF<EQUAL<char, typename Point<T, 3>::Distance>::Result, int, T>::Result Print_Type;

public:
    typedef Point<T, 3> Center;
    typedef typename Point<T, 3>::Distance Distance;
    typedef typename Point<T, 3>::Distance Radius;

    Sphere() {}
    Sphere(const Center & c, const Radius & r = 0): center(c), radius(r) { }

    bool contains(const Center & c) const { return (center - c) <= radius; }

    friend OStream & operator<<(OStream & os, const Sphere & s) {
        os << "{" << "c=" << s.center << ",r=" << static_cast<Print_Type>(s.radius) << "}";
        return os;
    }

    Center center;
    Radius radius;
};

__END_UTIL
