#pragma once

/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки. 
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */

#include <cassert>
#include <set>
#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "geo.h"

namespace tcat::domain {

using namespace geo;

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



} // namespace tcat::domain