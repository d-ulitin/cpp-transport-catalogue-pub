#include "domain.h"

/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области
 * (domain) вашего приложения и не зависят от транспортного справочника. Например Автобусные
 * маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */

#include <functional>
#include <cassert>
#include <stdexcept>
#include <numeric>

namespace tcat::domain {

using namespace std;

// class Stop

Stop::Stop(const string& name, Coordinates coordinates) :
    Stop(string(name), coordinates)
{
    assert(!name.empty());
}

Stop::Stop(string&& name, Coordinates coordinates) noexcept :
    name_(move(name)), coordinates_(coordinates)
{}

const string& Stop::Name() const {
    return name_;
}

Coordinates
Stop::GetCoordinates() const {
    return coordinates_;
}

// StopPointerHasher

size_t
StopPointerHasher::operator () (const Stop *stop) const noexcept {
    return std::hash<const void*>()(stop);
}

// class Bus

const string&
Bus::Name() const {
    return name_;
}

const vector<const Stop*>&
Bus::Stops() const {
    return stops_;
}

bool
Bus::Linear() const {
    return linear_;
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
            return geo::ComputeDistance(stop1->GetCoordinates(), stop2->GetCoordinates());
        });
    return linear_ ? 2 * distance : distance;
}

} // namespace tcat::domain
