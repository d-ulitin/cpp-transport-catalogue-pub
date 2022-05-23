#include "geo.h"

namespace transport_catalogue {

// struct Coordinates

Coordinates::Coordinates(double latitude, double longitude) noexcept :
    lat(latitude), lng(longitude)
{
    assert(latitude >= -90 && latitude <= 90);
    assert(longitude >= -180 && longitude <= 180);
}

bool Coordinates::operator==(const Coordinates& other) const noexcept {
    return lat == other.lat && lng == other.lng;
}

bool Coordinates::operator!=(const Coordinates& other) const noexcept {
    return !(*this == other);
}

double Coordinates::Distance(Coordinates to) {
    return ComputeDistance(*this, to);
}

double ComputeDistance(Coordinates from, Coordinates to) {
    using namespace std;
    if (from == to) {
        return 0;
    }

    static const double dr = 3.14159265358979323846 / 180.;
    static const double earth_radius = 6371000.;
    return acos(sin(from.lat * dr) * sin(to.lat * dr)
                + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
        * earth_radius;
}


}