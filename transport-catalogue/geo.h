#pragma once

#include <cmath>
#include <cassert>

namespace transport_catalogue {

struct Coordinates {
    double lat;
    double lng;

    Coordinates(double latitude, double longitude) noexcept;
    bool operator==(const Coordinates& other) const noexcept;
    bool operator!=(const Coordinates& other) const noexcept;
    double Distance(Coordinates to);
};

static_assert(std::is_default_constructible<Coordinates>::value == false);
static_assert(std::is_trivially_copyable<Coordinates>::value);
static_assert(std::is_trivially_move_assignable<Coordinates>::value);
static_assert(sizeof(Coordinates) <= sizeof(int) * 4); // must be small enouth to pass by value

double ComputeDistance(Coordinates from, Coordinates to);

} // namespace transport_catalogue