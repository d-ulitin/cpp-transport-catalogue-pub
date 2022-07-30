#pragma once

#include <cassert>
#include <cstdlib>
#include <deque>
#include <limits>
#include <set>
#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <algorithm>

#include "geo.h"
#include "domain.h"

namespace tcat::db {

using namespace tcat::domain;

using Distance = uint32_t;

static_assert(std::numeric_limits<Distance>::max() >= 1000000);

class TransportCatalogue {

public:
    const Stop* AddStop(Stop&& stop);
    const Stop* AddStop(const Stop& stop);
    const Stop* GetStop(std::string_view name) const;

    auto StopsIterators() const {
        return std::make_pair(stops_.cbegin(), stops_.cend());
    }

    const Bus* AddBus(Bus&& bus);
    const Bus* AddBus(const Bus& bus);
    const Bus* GetBus(std::string_view name) const;

    auto BusesIterators() const {
        return std::make_pair(buses_.cbegin(), buses_.cend());
    }

    auto StopsDistancesIterators() const {
        return std::make_pair(stops_distances_.cbegin(), stops_distances_.cend());
    }

private:
    struct BusLessByName {
        bool operator()(const Bus* b1, const Bus* b2) const noexcept;
    };

public:
    using Buses = std::set<const Bus*, BusLessByName>;

    const Buses& GetBuses(const Stop* stop) const;

    void AddDistance(const Stop* stop1, const Stop* stop2, Distance distance);
    Distance GetDistance(const Stop* stop1, const Stop* stop2) const;
    Distance RouteLength(const Bus* bus) const;

private:
    // Storage
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;

    // Indexes
    std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, Bus*> busname_to_bus_;
    std::unordered_map<const Stop*, Buses> stop_to_buses_;

    // Distances
    struct PairOfStopsHasher {
        size_t operator() (const std::pair<const Stop*, const Stop*> stops) const noexcept;
    };

    std::unordered_map<
        std::pair<const Stop*, const Stop*>, Distance, PairOfStopsHasher> stops_distances_;
};

} // namespace tcat::db