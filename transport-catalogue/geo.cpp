#define _USE_MATH_DEFINES
#include "geo.h"

#include <cmath>
#include <cassert>

namespace tcat::geo {

//
// Coordinates
//

Coordinates::Coordinates(double latitude, double longitude) noexcept :
    lat(latitude), lng(longitude)
{
    assert(latitude >= -90 && latitude <= 90);
    assert(longitude >= -180 && longitude <= 180);
}

bool Coordinates::operator==(const Coordinates& other) const {
    return lat == other.lat && lng == other.lng;
}

bool Coordinates::operator!=(const Coordinates& other) const {
    return !(*this == other);
}

double Coordinates::Distance(Coordinates to) const {
    return ComputeDistance(*this, to);
}

double ComputeDistance(Coordinates from, Coordinates to) {

    using namespace std;

    if (from == to) {
        return 0;
    }

    static const double dr = M_PI / 180.;
    return acos(sin(from.lat * dr) * sin(to.lat * dr)
                + cos(from.lat * dr) * cos(to.lat * dr) * cos(std::abs(from.lng - to.lng) * dr))
        * EARTH_RADIUS;
}

}  // namespace tcat::geo