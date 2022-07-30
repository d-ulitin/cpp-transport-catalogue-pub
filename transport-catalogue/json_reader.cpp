#include "json_reader.h"
#include "json_builder.h"

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

        if (bus) {
            const auto route_length = tc_.RouteLength(bus);
            json::Node node = json::Builder()
                .StartDict()
                    .Key("request_id"s).Value({id})
                    .Key("route_length"s).Value(static_cast<double>(route_length))
                    .Key("stop_count"s).Value(static_cast<int>(bus->StopsNumber()))
                    .Key("unique_stop_count"s).Value(static_cast<int>(bus->UniqueStops().size()))
                    .Key("curvature"s).Value(static_cast<double>(route_length) / bus->GeoLength())
                .EndDict()
                .Build();
            return node;
        } else {
            json::Node node = json::Builder()
                .StartDict()
                    .Key("request_id"s).Value({id})
                    .Key("error_message"s).Value("not found"s)
                .EndDict()
                .Build();
            return node;
        }

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

        if (stop) {
            const auto& buses = tc_.GetBuses(stop);
            json::Array buses_array;
            for (const auto& bus : buses) {
                buses_array.emplace_back(bus->Name());
            }
            auto node = json::Builder()
                .StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("buses"s).Value(buses_array)
                .EndDict()
                .Build();
            return node;
        } else {
            auto node = json::Builder()
                .StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s)
                .EndDict()
                .Build();
            return node;
        }
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

        auto node = json::Builder()
            .StartDict()
                .Key("request_id"s).Value(id)
                .Key("map"s).Value(svg_text.str())
            .EndDict()
            .Build();
        return node;

    } catch (const out_of_range& e) { // std::map
        throw InputError("map request error");
    } catch (const json::ParsingError& e) {
        throw InputError("JSON parsing error: "s + e.what());
    }
}

/*
    Запрос маршрута. Словарь:

    "type": "Route"
    from — остановка, где нужно начать маршрут.
    to — остановка, где нужно закончить маршрут.

    Пример
    {
        "type": "Route",
        "from": "Biryulyovo Zapadnoye",
        "to": "Universam",
        "id": 4
    }

    Ответ на запрос Route устроен следующим образом:
    {
        "request_id": <id запроса>,
        "total_time": <суммарное время>,
        "items": [
            <элементы маршрута>
        ]
    }

    total_time — суммарное время в минутах, которое требуется для прохождения маршрута, выведенное
    в виде вещественного числа.

    items — список элементов маршрута, каждый из которых описывает непрерывную активность
    пассажира, требующую временных затрат.

    Если маршрута между указанными остановками нет, выведите результат в следующем формате:
    {
        "request_id": <id запроса>,
        "error_message": "not found"
    }

 */
json::Node
JsonRequestReader::RouteStat(const json::Node& route_request, TransportRouter& router) {

    const auto& map = route_request.AsMap();

    try {
        if (map.at("type"s) != "Route"s)
            throw InputError("request type isn't Route");
        const int id = map.at("id"s).AsInt();
        const Stop *from = tc_.GetStop(map.at("from"s).AsString());
        const Stop *to = tc_.GetStop(map.at("to"s).AsString());

        if (from && to) {
            auto route = router.Route(from, to);
            if (route) {
                auto node = json::Builder()
                    .StartDict()
                        .Key("request_id"s).Value(id)
                        .Key("total_time"s).Value(route->total_time)
                        .Key("items"s).Value(RouteActivities(*route))
                    .EndDict()
                    .Build();
                return node;
            }
        }

        return json::Builder()
               .StartDict()
                   .Key("request_id"s).Value(id)
                   .Key("error_message"s).Value("not found"s)
               .EndDict()
               .Build();
    } catch (const out_of_range& e) { // std::map
        throw InputError("stop request error");
    }}


/*
    Wait — подождать нужное количество минут (в нашем случае всегда bus_wait_time) на указанной
    остановке

    {
        "type": "Wait",
        "stop_name": "Biryulyovo",
        "time": 6
    }

    Bus — проехать span_count остановок (перегонов между остановками) на автобусе bus, потратив
    указанное количество минут:
    {
        "type": "Bus",
        "bus": "297",
        "span_count": 2,
        "time": 5.235
    } 

 */
json::Node
JsonRequestReader::RouteActivities(const TransportRouter::RouteResult& route) {

    struct ActivityVisitor {
        json::Node operator()(const TransportRouter::WaitActivity& activity) const {
            auto node = json::Builder()
                .StartDict()
                    .Key("type"s).Value("Wait"s)
                    .Key("stop_name"s).Value(activity.stop->Name())
                    .Key("time"s).Value(activity.time)
                .EndDict()
                .Build();
            return node;
        }

        json::Node operator()(const TransportRouter::BusActivity& activity) const {
            auto node = json::Builder()
                .StartDict()
                    .Key("type"s).Value("Bus"s)
                    .Key("bus"s).Value(activity.bus->Name())
                    .Key("span_count"s).Value(activity.span)
                    .Key("time").Value(activity.time)
                .EndDict()
                .Build();
            return node;
        }
    };

    json::Array items;
    for (auto & activity : route.activities) {
        auto node = visit(ActivityVisitor{}, activity);
        items.push_back(node);
    }
    
    return json::Node(items);
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
JsonRequestReader::ReadStat(const json::Document& doc,
                            const MapRendererSettings& render_settings,
                            TransportRouter& router) {
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
                result.push_back(MapStat(node, render_settings));
            } else if (request_type == "Route") {
                result.push_back(RouteStat(node, router));
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
        MapRendererSettings settings;
        const json::Dict& root_map = doc.GetRoot().AsMap();
        auto iter = root_map.find("render_settings"s);
        if (iter != root_map.end()) {
            auto map = iter->second.AsMap();
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
        }
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

/*
    ключ routing_settings — словарь с двумя ключами:

    bus_wait_time — время ожидания автобуса на остановке, в минутах.
    Значение — целое число от 1 до 1000.

    bus_velocity — скорость автобуса, в км/ч. Значение — вещественное число от 1 до 1000.

    Пример:
    "routing_settings": {
      "bus_wait_time": 6,
      "bus_velocity": 40
    } 
 */
RoutingSettings
JsonRequestReader::ReadRoutingSettings(const json::Document& doc) {
    try {
        RoutingSettings settings;
        const json::Dict& root_map = doc.GetRoot().AsMap();
        auto iter = root_map.find("routing_settings"s);
        if (iter != root_map.end()) {
            auto map = iter->second.AsMap();
            settings.bus_wait_time = map.at("bus_wait_time").AsInt();
            settings.bus_velocity = map.at("bus_velocity").AsDouble();
        }
        return settings;
    } catch (const out_of_range& e) {
        throw InputError("failed to read render_settings");
    } catch (const json::ParsingError& e) {
        throw InputError("JSON parsing error: "s + e.what());
    }
}

serialization::Settings
JsonRequestReader::ReadSerializationSettings(const json::Document& doc) {
    try {
        const json::Node& node = doc.GetRoot().AsMap().at("serialization_settings"s);
        auto map = node.AsMap();

        serialization::Settings settings;
        settings.file = map.at("file").AsString();
        return settings;
    } catch (const out_of_range& e) {
        throw InputError("failed to read render_settings");
    } catch (const json::ParsingError& e) {
        throw InputError("JSON parsing error: "s + e.what());
    }
}


} // namespace tcat::io
