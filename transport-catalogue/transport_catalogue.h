#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <cstddef>
#include <utility>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <set>
#include <cstdlib> // uint32_t
#include <limits>

#include "geo.h"

namespace transport_catalogue {

class Stop {
public:
    Stop(const std::string& name, Coordinates coordinates);
    Stop(std::string&& name, Coordinates coordinates) noexcept;

    const std::string& Name() const;
    Coordinates GetCoordinates() const;

private:
    std::string name_;
    Coordinates coordinates_;
};

struct StopPointerHasher {
    size_t operator () (const Stop *stop) const noexcept;
};

struct PairOfStopsPointersHasher {
    size_t operator () (const std::pair<const Stop*, const Stop*> pair_of_stops) const noexcept;
};

class Bus {
public:
    template <typename InputIt>
    Bus(const std::string& name, InputIt stops_first, InputIt stops_last, bool linear) :
        Bus(std::string(name), stops_first, stops_last, linear)
    { }

    template <typename InputIt>
    Bus(std::string&& name, InputIt stops_first, InputIt stops_last, bool linear);

    const std::string& Name() const;
    std::vector<const Stop*> Stops() const;
    bool Linear() const;

    size_t StopsNumber() const;

    std::unordered_set<const Stop*, StopPointerHasher>
    UniqueStops() const;

    double GeoLength() const;

private:
    std::string name_;
    std::vector<const Stop*> stops_;
    bool linear_;
};

struct BusLessByName {
    bool operator()(const Bus* b1, const Bus* b2) const;
};

template <typename InputIt>
Bus::Bus(std::string&& name, InputIt stops_first, InputIt stops_last, bool linear) :
    name_(std::move(name)),
    stops_(stops_first, stops_last),
    linear_(linear)
{
    assert(name.empty()); // "undefined", but empty in practice
    assert(!name_.empty());
    assert(std::distance(stops_first, stops_last) > 0);
}

using Distance = uint32_t;

static_assert(std::numeric_limits<Distance>::max() >= 1000000);

class TransportCatalogue {
public:

    const Stop* AddStop(Stop&& stop);
    const Stop* AddStop(const Stop& stop);
    const Stop* GetStop(std::string_view name) const;

    const Bus* AddBus(Bus&& bus);
    const Bus* AddBus(const Bus& bus);
    const Bus* GetBus(std::string_view name) const;

    using BusSet = std::set<const Bus*, BusLessByName>;

    const BusSet& GetBusesForStop(const Stop* stop) const;

    void AddDistance(const Stop* stop1, const Stop* stop2, Distance distance);

    Distance GetDistance(const Stop* stop1, const Stop* stop2) const;

    Distance RouteLength(const Bus* bus) const;

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Stop *> stopname_to_stop_;
    std::unordered_map<std::string_view, Bus *> busname_to_bus_;
    std::unordered_map<const Stop*, BusSet> stop_to_buses_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, Distance, PairOfStopsPointersHasher>
        stops_distances_;
};

} // namespace transport_catalogue