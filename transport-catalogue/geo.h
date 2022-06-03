#pragma once

namespace tcat::geo {

inline const double EARTH_RADIUS = 6371000.;

struct Coordinates {
    double lat;
    double lng;

    Coordinates(double latitude, double longitude) noexcept;
    bool operator==(const Coordinates& other) const;
    bool operator!=(const Coordinates& other) const;
    double Distance(Coordinates to) const;
};

double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace tcat::geo