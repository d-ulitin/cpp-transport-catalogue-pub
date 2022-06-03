#pragma once

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

#include <deque>
#include <stdexcept>
#include <string>
#include <tuple>

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

namespace tcat::io {

using namespace tcat::db;

class InputError : public std::runtime_error {
public:
    explicit InputError(const std::string& msg);
    explicit InputError(const char* msg);
};

class JsonRequestReader final {
public:
    JsonRequestReader(TransportCatalogue& tc);

    void ReadBase(const json::Document& doc);
    json::Node ReadStat(const json::Document& doc);
    MapRendererSettings ReadRendererSettings(const json::Document& doc);

private:
    using DistancesQueue = std::deque<std::tuple<const Stop*, std::string, Distance>>;

    void ReadStop(const json::Node& stop, DistancesQueue& distances);
    void ReadBus(const json::Node& bus);
    json::Node BusStat(const json::Node& bus_request);
    json::Node StopStat(const json::Node& stop_request);
    json::Node MapStat(const json::Node& map_request, const MapRendererSettings& settings);
    svg::Color ReadColor(const json::Node& color_node);
    std::vector<svg::Color> ReadColorPallete(const json::Node& pallete_node);

    TransportCatalogue& tc_;
};

} // namespace tcat::io
