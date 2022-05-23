#include "transport_catalogue.h"

#include <functional>
#include <cassert>
#include <stdexcept>
#include <numeric>

namespace transport_catalogue {

using namespace std;

size_t
Stop::hash(const Stop& stop) {
    const size_t prime = 47;
    size_t hash = std::hash<string>()(stop.name_);
    hash = prime * hash + std::hash<int>()(stop.GetCoordinates().lat);
    hash = prime * hash + std::hash<int>()(stop.GetCoordinates().lng);
    return hash;
}

size_t
Bus::StopsNumber() const {
    return linear_ ? stops_.size() * 2 - 1 : stops_.size();
}

unordered_set<const Stop*, StopPointerHasher>
Bus::UniqueStops() const {
    return {stops_.begin(), stops_.end()};
}

double
Bus::GeoLength() const {
    assert(!stops_.empty());
    if (stops_.size() == 1)
        return 0;
    auto distance = std::transform_reduce(
        next(stops_.begin()), stops_.end(),
        stops_.begin(),
        0.0, // must be double or float
        plus<>{},
        [](const Stop* stop1, const Stop* stop2) {
            return ComputeDistance(stop1->GetCoordinates(), stop2->GetCoordinates());
        });
    return linear_ ? 2 * distance : distance;
}

const Stop*
TransportCatalogue::AddStop(Stop&& stop) {
    if (stopname_to_stop_.count(stop.Name()) > 0)
        throw invalid_argument("duplicate stop "s + stop.Name());
    stops_.push_back(move(stop));
    Stop& added_stop = stops_.back();
    stopname_to_stop_.emplace(make_pair(string_view(added_stop.Name()), &added_stop));
    return &added_stop;
}

const Stop*
TransportCatalogue::GetStop(string_view name) const {
    auto it = stopname_to_stop_.find(name);
    if (it == stopname_to_stop_.end())
        return nullptr;
    else
        return it->second;
}

const Bus*
TransportCatalogue::AddBus(Bus&& bus) {
    if (busname_to_bus_.count(bus.Name()) > 0)
        throw invalid_argument("duplicate bus "s + bus.Name());
    buses_.push_back(move(bus));
    Bus& added_bus = buses_.back();
    busname_to_bus_.emplace(make_pair(string_view(added_bus.Name()), &added_bus));
    for (const Stop* stop : added_bus.Stops()) {
        stop_to_buses_[stop].insert(&added_bus);
    }
    return &added_bus;
}

const Bus*
TransportCatalogue::GetBus(string_view name) const {
    auto it = busname_to_bus_.find(name);
    if (it == busname_to_bus_.end())
        return nullptr;
    else
        return it->second;
}

const TransportCatalogue::BusSet&
TransportCatalogue::GetBusesForStop(const Stop* stop) const{
    static BusSet empty;
    auto it = stop_to_buses_.find(stop);
    if (it == stop_to_buses_.end())
        return empty;
    else
        return it->second;
}

void
TransportCatalogue::AddDistance(const Stop* stop1, const Stop* stop2, Distance distance) {
    stops_distances_.insert({make_pair(stop1, stop2), distance});
}

Distance TransportCatalogue::GetDistance(const Stop* stop1, const Stop* stop2) const {
    auto it = stops_distances_.find(make_pair(stop1, stop2));
    if (it == stops_distances_.end())
        it = stops_distances_.find(make_pair(stop2, stop1));
    if (it == stops_distances_.end())
        throw runtime_error("unknown distance between "s + stop1->Name() + " and "s + stop2->Name());
    return it->second;
}

Distance TransportCatalogue::RouteLength(const Bus* bus) const {
    auto stops = bus->Stops();
    if (stops.empty())
        throw runtime_error("bus "s + bus->Name() + " has no stops");
    Distance distance = transform_reduce(
        next(stops.begin()), stops.end(),
        stops.begin(),
        static_cast<Distance>(0), // this parameter is used for type deduction of result
        plus<>(),
        [this](const Stop* stop1, const Stop* stop2) {
            return GetDistance(stop2, stop1); // order is important
        }
    );
    if (bus->Linear()) {
        // drive in reverse order
        distance += transform_reduce(
            next(stops.rbegin()), stops.rend(),
            stops.rbegin(),
            static_cast<Distance>(0),
            plus<>(),
            [this](const Stop* stop1, const Stop* stop2) {
                return GetDistance(stop2, stop1); // order is important
            }
        );
    }
    return distance;
}


} // namespace transport_catalogue