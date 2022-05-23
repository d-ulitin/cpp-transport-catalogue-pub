#pragma once

#include <cmath>
#include <cassert>

namespace transport_catalogue {

struct Coordinates;
double ComputeDistance(Coordinates from, Coordinates to);

struct Coordinates {
    double lat;
    double lng;

    Coordinates(double latitude, double longitude) noexcept :
        lat(latitude), lng(longitude)
    {
        assert(latitude >= -90 && latitude <= 90);
        assert(longitude >= -180 && longitude <= 180);
    }

    bool operator==(const Coordinates& other) const noexcept {
        return lat == other.lat && lng == other.lng;
    }

    bool operator!=(const Coordinates& other) const noexcept {
        return !(*this == other);
    }

    double Distance(Coordinates to) {
        return ComputeDistance(*this, to);
    }
};


static_assert(std::is_default_constructible<Coordinates>::value == false);
static_assert(std::is_trivially_copyable<Coordinates>::value);
static_assert(std::is_trivially_move_assignable<Coordinates>::value);
static_assert(sizeof(Coordinates) <= sizeof(int) * 4); // must be small enouth to pass by value


inline double ComputeDistance(Coordinates from, Coordinates to) {
    using namespace std;
    if (from == to) {
        return 0;
    }

    static const double dr = 3.14159265358979323846 / 180.;
    return acos(sin(from.lat * dr) * sin(to.lat * dr)
                + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
        * 6371000;
}

} // namespace transport_catalogue