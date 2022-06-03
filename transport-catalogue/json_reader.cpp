#include "json_reader.h"

#include <sstream>

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace tcat::io {

using namespace std;
using namespace tcat::db;

//
// InputError
//

InputError::InputError(const string& msg)
    : runtime_error(msg) {
}

InputError::InputError(const char* msg)
    : runtime_error(msg) {
}

//
// JsonRequestReader
//

JsonRequestReader::JsonRequestReader(TransportCatalogue& tc)
    : tc_(tc) {

}

/*
    Описание остановки — словарь с ключами:
    type — строка, равная "Stop". Означает, что словарь описывает остановку;
    name — название остановки;
    latitude и longitude — широта и долгота остановки — числа с плавающей запятой;
    road_distances — словарь, задающий дорожное расстояние от этой остановки до соседних.
        Каждый ключ в этом словаре — название соседней остановки, значение —целочисленное
        расстояние в метрах.

    {
        "type": "Stop",
        "name": "Электросети",
        "latitude": 43.598701,
        "longitude": 39.730623,
        "road_distances": {
            "Улица Докучаева": 3000,
            "Улица Лизы Чайкиной": 4300
        }
    }
*/

void JsonRequestReader::ReadStop(const json::Node& stop, DistancesQueue& distances) {
    
    // don't check for errors, caller must handle possible exceptions

    const auto& map = stop.AsMap();

    try {
        if (map.at("type"s) != "Stop"s)
            throw InputError("not bus stop data");
        const string& name = map.at("name"s).AsString();
        double lat = map.at("latitude"s).AsDouble();
        double lng = map.at("longitude"s).AsDouble();
        const Stop *added_stop = tc_.AddStop(Stop(name, {lat, lng}));
        if (auto distances_node = map.find("road_distances"s); distances_node != map.end()) {
            for (const auto& dist_pair : distances_node->second.AsMap()) {
                const string& other_stop = dist_pair.first;
                Distance distance = dist_pair.second.AsInt();
                distances.emplace_back(make_tuple(added_stop, other_stop, distance));
            }
        }
    } catch (const out_of_range& e) {  // std::map
        throw InputError("bus stop json error");
    }
}

/*
    Описание автобусного маршрута — словарь с ключами:
    type — строка "Bus". Означает, что словарь описывает автобусный маршрут;
    name — название маршрута;
    stops — массив с названиями остановок, через которые проходит маршрут. У кольцевого маршрута
        название последней остановки дублирует название первой.
        Например: ["stop1", "stop2", "stop3", "stop1"];
    is_roundtrip — значение типа bool. true, если маршрут кольцевой.

    {
    "type": "Bus",
    "name": "14",
    "stops": [
        "Улица Лизы Чайкиной",
        "Электросети",
        "Улица Докучаева",
        "Улица Лизы Чайкиной"
    ],
    "is_roundtrip": true
    } 
 */

void JsonRequestReader::ReadBus(const json::Node& bus) {

    const auto& map = bus.AsMap();

    try {
        if (map.at("type"s) != "Bus"s)
            throw InputError("not bus data");
        const string& name = map.at("name"s).AsString();
        bool is_roundtrip = map.at("is_roundtrip").AsBool();
        vector<const Stop*> stops;
        const auto& stops_node = map.at("stops"s).AsArray();
        for (const auto& stop_node : stops_node) {
            const Stop *stop = tc_.GetStop(stop_node.AsString());
            stops.push_back(stop);
        }
        tc_.AddBus(Bus(name, begin(stops), end(stops), !is_roundtrip));
    } catch (const out_of_range& e) { // std::map
        throw InputError("bus json error");
    }
}

/*
    Получение информации о маршруте

    Формат запроса:
    {
        "id": 12345678,
        "type": "Bus",
        "name": "14"
    } 

    Ключ type имеет значение “Bus”. По нему можно определить, что это запрос на получение информации о маршруте.
    Ключ name задаёт название маршрута, для которого приложение должно вывести статистическую информацию.

    Ответ на этот запрос должен быть дан в виде словаря:
    {
        "curvature": 2.18604,
        "request_id": 12345678,
        "route_length": 9300,
        "stop_count": 4,
        "unique_stop_count": 3
    } 

    Ключи словаря:
    curvature — извилистость маршрута. Она равна отношению длины дорожного расстояния маршрута к длине географического расстояния. Число типа double;
    request_id — должен быть равен id соответствующего запроса Bus. Целое число;
    route_length — длина дорожного расстояния маршрута в метрах, целое число;
    stop_count — количество остановок на маршруте;
    unique_stop_count — количество уникальных остановок на маршруте.

    Например, на кольцевом маршруте с остановками A, B, C, A четыре остановки. Три из них уникальные.
    
    На некольцевом маршруте с остановками A, B и C пять остановок (A, B, C, B, A). Три из них уникальные.

    Если в справочнике нет маршрута с указанным названием, ответ должен быть таким:
    {
    "request_id": 12345678,
    "error_message": "not found"
    } 
*/

json::Node
JsonRequestReader::BusStat(const json::Node& bus_request) {

    const auto& map = bus_request.AsMap();

    try {
        if (map.at("type"s) != "Bus"s)
            throw InputError("request type isn't Bus");
        const int id = map.at("id"s).AsInt();
        const string& name = map.at("name"s).AsString();
        const Bus *bus = tc_.GetBus(name);

        json::Dict stat;
        stat.insert({"request_id"s, json::Node{id}});
        if (bus) {
            const auto route_length = tc_.RouteLength(bus);
            stat.insert({"route_length"s, static_cast<int>(route_length)});
            stat.insert({"stop_count"s, static_cast<int>(bus->StopsNumber())});
            stat.insert({"unique_stop_count"s, static_cast<int>(bus->UniqueStops().size())});
            stat.insert({"curvature"s, static_cast<double>(route_length) / bus->GeoLength()});
        } else {
            stat.insert({"error_message"s, "not found"s});
        }
        return {stat};
    } catch (const out_of_range& e) { // std::map
        throw InputError("bus request error");
    }
}

/*
    Получение информации об остановке

    Формат запроса:
    {
        "id": 12345,
        "type": "Stop",
        "name": "Улица Докучаева"
    } 

    Ключ name задаёт название остановки.

    Ответ на запрос:
    {
        "buses": [
            "14", "22к"
        ],
        "request_id": 12345
    } 

    Значение ключей ответа:
        buses — массив названий маршрутов, которые проходят через эту остановку. Названия
        отсортированы в лексикографическом порядке.
        request_id — целое число, равное id соответствующего запроса Stop.
    
    Если в справочнике нет остановки с переданным названием, ответ на запрос должен быть такой:
    {
        "request_id": 12345,
        "error_message": "not found"
    } 
 */

json::Node
JsonRequestReader::StopStat(const json::Node& stop_request) {

    const auto& map = stop_request.AsMap();

    try {
        if (map.at("type"s) != "Stop"s)
            throw InputError("request type isn't Stop");
        const int id = map.at("id"s).AsInt();
        const string& name = map.at("name"s).AsString();
        const Stop *stop = tc_.GetStop(name);

        json::Dict stat;
        stat.insert({"request_id"s, json::Node{id}});
        if (stop) {
            const auto& buses = tc_.GetBuses(stop);
            json::Array buses_array;
            for (const auto& bus : buses) {
                buses_array.emplace_back(bus->Name());
            }
            stat.insert({"buses"s, json::Node{buses_array}});
        } else {
            stat.insert({"error_message"s, "not found"s});
        }
        return {stat};
    } catch (const out_of_range& e) { // std::map
        throw InputError("stop request error");
    }
}

/*
    Запрос на рендеринг карты вида
    {
        "type": "Map",
        "id": 11111
    }

    Ответ на этот запрос отдаётся в виде словаря с ключами request_id и map:
    {
        "map": "<?xml...",
        "request_id": 11111
    } 
 */
json::Node
JsonRequestReader::MapStat(const json::Node& map_request, const MapRendererSettings& settings) {
    try {
        const auto& map = map_request.AsMap();

        if (map.at("type"s) != "Map"s)
            throw InputError("request type must be Map");

        const int id = map.at("id"s).AsInt();

        tcat::io::MapRenderer map_renderer(tc_, settings);
        svg::Document svg_doc = map_renderer.Render();
        ostringstream svg_text;
        svg_doc.Render(svg_text);

        json::Dict map_dict;
        map_dict.insert({"request_id"s, json::Node{id}});
        map_dict.insert({"map"s, json::Node{svg_text.str()}});
        return json::Node{map_dict};
    } catch (const out_of_range& e) { // std::map
        throw InputError("map request error");
    } catch (const json::ParsingError& e) {
        throw InputError("JSON parsing error: "s + e.what());
    }
}


/*
    Массив base_requests содержит элементы двух типов: маршруты и остановки. Они перечисляются
    в произвольном порядке.
 */
void JsonRequestReader::ReadBase(const json::Document& doc) {

    try {

        const json::Node& base_requests = doc.GetRoot().AsMap().at("base_requests"s);

        DistancesQueue distances;

        // add stops
        for (const auto& node : base_requests.AsArray()) {
            const auto& map = node.AsMap();
            const string& type = map.at("type"s).AsString();
            if (type == "Stop"s) {
                ReadStop(node, distances);
            }
        }

        // add distances
        while (!distances.empty()) {
            auto d = distances.front();
            const Stop* stop1 = get<0>(d);
            const Stop* stop2 = tc_.GetStop(get<1>(d));
            Distance distance = get<2>(d);
            tc_.AddDistance(stop1, stop2, distance);
            distances.pop_front();
        }

        // add buses
        for (const auto& node : base_requests.AsArray()) {
            const auto& map = node.AsMap();
            const string& type = map.at("type"s).AsString();
            if (type == "Bus"s) {
                ReadBus(node);
            }
        }

    } catch (const out_of_range& e) {  // std::map
        throw InputError("failed to read base_requests");
    } catch (const json::ParsingError& e) {
        throw InputError("JSON parsing error: "s + e.what());
    }
}

/*
    Формат запросов к транспортному справочнику и ответов на них

    Запросы хранятся в массиве stat_requests. В ответ на них программа должна вывести в stdout JSON-массив ответов:
    [
        { ответ на первый запрос },
        { ответ на второй запрос },
        ...
        { ответ на последний запрос }
    ] 
    Каждый запрос — словарь с обязательными ключами id и type. Они задают уникальный числовой идентификатор запроса и его тип. В словаре могут быть и другие ключи, специфичные для конкретного типа запроса.

    В выходном JSON-массиве на каждый запрос stat_requests должен быть ответ в виде словаря с обязательным ключом request_id. Значение ключа должно быть равно id соответствующего запроса. В словаре возможны и другие ключи, специфичные для конкретного типа ответа.

    Порядок следования ответов на запросы в выходном массиве должен совпадать с порядком запросов в массиве stat_requests.
 */

json::Node
JsonRequestReader::ReadStat(const json::Document& doc) {
    try {
        const json::Node& stat_requests = doc.GetRoot().AsMap().at("stat_requests"s);

        json::Array result;

        const json::Array& array = stat_requests.AsArray();
        for (const auto& node : array) {
            const string& request_type = node.AsMap().at("type"s).AsString();
            if (request_type == "Bus") {
                result.push_back(BusStat(node));
            } else if (request_type == "Stop") {
                result.push_back(StopStat(node));
            } else if (request_type == "Map") {
                MapRendererSettings settings = ReadRendererSettings(doc);
                result.push_back(MapStat(node, settings));
            }
            else {
                throw InputError("unknown stat request type"s);
            }
        }

        return {result};

    } catch (const out_of_range& e) {  // std::map
        throw InputError("failed to read stat_requests");
    } catch (const json::ParsingError& e) {
        throw InputError("JSON parsing error: "s + e.what());
    }
}

MapRendererSettings
JsonRequestReader::ReadRendererSettings(const json::Document& doc) {

    try {
        const json::Node& render_settings = doc.GetRoot().AsMap().at("render_settings"s);
        auto map = render_settings.AsMap();

        MapRendererSettings settings;
        settings.width = map.at("width"s).AsDouble();
        settings.height = map.at("height"s).AsDouble();
        settings.padding = map.at("padding"s).AsDouble();
        settings.line_width = map.at("line_width").AsDouble();
        settings.stop_radius = map.at("stop_radius").AsDouble();
        settings.bus_label_font_size = map.at("bus_label_font_size").AsInt();
        settings.bus_label_offset[0] = map.at("bus_label_offset").AsArray().at(0).AsDouble();
        settings.bus_label_offset[1] = map.at("bus_label_offset").AsArray().at(1).AsDouble();
        settings.stop_label_font_size = map.at("stop_label_font_size").AsInt();
        settings.stop_label_offset[0] = map.at("stop_label_offset").AsArray().at(0).AsDouble();
        settings.stop_label_offset[1] = map.at("stop_label_offset").AsArray().at(1).AsDouble();
        settings.underlayer_color = ReadColor(map.at("underlayer_color"));
        settings.underlayer_width = map.at("underlayer_width").AsDouble();
        settings.color_palette = ReadColorPallete(map.at("color_palette"));
        return settings;
    } catch (const out_of_range& e) {
        throw InputError("failed to read render_settings");
    } catch (const json::ParsingError& e) {
        throw InputError("JSON parsing error: "s + e.what());
    }
}

/*
    Цвет можно указать в одном из следующих форматов:
    в виде строки, например, "red" или "black";
    в массиве из трёх целых чисел диапазона [0, 255]. Они определяют r, g и b компоненты цвета
    в формате svg::Rgb. Цвет [255, 16, 12] нужно вывести в SVG как rgb(255,16,12);
    в массиве из четырёх элементов: три целых числа в диапазоне от [0, 255] и одно
    вещественное число в диапазоне от [0.0, 1.0]. Они задают составляющие
    red, green, blue и opacity цвета формата svg::Rgba. Цвет, заданный как [255, 200, 23, 0.85],
    должен быть выведен в SVG как rgba(255,200,23,0.85).
 */
svg::Color
JsonRequestReader::ReadColor(const json::Node& color_node) {
    svg::Color color;
    if (color_node.IsString()) {
        color = color_node.AsString();
    } else if (color_node.IsArray()) {
        if (color_node.AsArray().size() == 3) {
            auto array = color_node.AsArray();
            color = svg::Rgb{static_cast<uint8_t>(array.at(0).AsInt()),
                             static_cast<uint8_t>(array.at(1).AsInt()),
                             static_cast<uint8_t>(array.at(2).AsInt())};
        } else if (color_node.AsArray().size() == 4) {
            auto array = color_node.AsArray();
            color = svg::Rgba{static_cast<uint8_t>(array.at(0).AsInt()),
                              static_cast<uint8_t>(array.at(1).AsInt()),
                              static_cast<uint8_t>(array.at(2).AsInt()),
                              array.at(3).AsDouble()};
        } else {
            throw InputError("unknown color");
        }
    } else {
        throw InputError("unknown color");
    }
    return color;
}

vector<svg::Color>
JsonRequestReader::ReadColorPallete(const json::Node& pallete_node) {
    vector<svg::Color> pallete;
    for (auto& color_node : pallete_node.AsArray()) {
        pallete.push_back(ReadColor(color_node));
    }
    return pallete;
}


} // namespace tcat::io
