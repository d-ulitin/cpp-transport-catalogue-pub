#include "serialization.h"
#include "ranges.h"
#include "transport_catalogue.h"
#include "svg.h"
#include "ranges.h"

#include <variant>
#include <cassert>
#include <unordered_map>
#include <vector>
#include <memory>

#include <transport_catalogue.pb.h>


namespace tcat::io::serialization {

using namespace std;
using namespace tcat::domain;

// use pointers as id of stops and buses
static_assert(sizeof(uint64_t) >= sizeof(void*));

void AddStops(const db::TransportCatalogue& tc, proto::TransportCatalogue& tc_msg) {
    for(const auto& stop : ranges::AsRange(tc.StopsIterators())) {
        auto *proto_stop = tc_msg.add_stop();
        // user pointer as stop id
        proto_stop->set_id(reinterpret_cast<uint64_t>(&stop));
        // name
        proto_stop->set_name(stop.Name());
        // coordinates
        auto *proto_coordinates = proto_stop->mutable_coordinates();
        auto coordinates = stop.GetCoordinates();
        proto_coordinates->set_lat(coordinates.lat);
        proto_coordinates->set_lng(coordinates.lng);
    }
}

void AddStopsDistances(const db::TransportCatalogue& tc, proto::TransportCatalogue& tc_msg) {
    for (const auto& stops_distance : ranges::AsRange(tc.StopsDistancesIterators())) {
        auto *proto_stops_distance = tc_msg.add_stops_distance();
        proto_stops_distance->set_from_stop_id(reinterpret_cast<uintptr_t>(stops_distance.first.first));
        proto_stops_distance->set_to_stop_id(reinterpret_cast<uintptr_t>(stops_distance.first.second));
        proto_stops_distance->set_distance(stops_distance.second);
    }
}

void AddBuses(const db::TransportCatalogue& tc, proto::TransportCatalogue& tc_msg) {
    for (const auto& bus: ranges::AsRange(tc.BusesIterators())) {
        auto *proto_bus = tc_msg.add_bus();
        // use pointer as id
        proto_bus->set_id(reinterpret_cast<uintptr_t>(&bus));
        proto_bus->set_name(bus.Name());
        for (const auto* stop : bus.Stops()) {
            proto_bus->add_stop_id(reinterpret_cast<uintptr_t>(stop));
        }
        if (bus.Linear()) {
            proto_bus->set_route_type(proto::Bus_RouteType::Bus_RouteType_LINEAR);
        }
        else {
            proto_bus->set_route_type(proto::Bus_RouteType::Bus_RouteType_CIRCULAR);
        }
    }
}

void FillMessage(const db::TransportCatalogue& tc, proto::TransportCatalogue& message) {
    AddStops(tc, message);    
    AddStopsDistances(tc, message);
    AddBuses(tc, message);
}

struct SetColorVisitor {

    SetColorVisitor(proto::Color& color_msg) : color_msg_(color_msg) {};

    void operator()(std::monostate) { assert(false); }

    void operator()(const std::string& str) { color_msg_.set_str(str); }

    void operator()(const svg::Rgb& rgb) {
        proto::Color_Rgb& rgb_msg = *color_msg_.mutable_rgb();
        rgb_msg.set_red(rgb.red);
        rgb_msg.set_green(rgb.green);
        rgb_msg.set_blue(rgb.blue);
    }

    void operator()(const svg::Rgba& rgba) {
        proto::Color_Rgba& rgba_msg = *color_msg_.mutable_rgba();
        rgba_msg.set_red(rgba.red);
        rgba_msg.set_green(rgba.green);
        rgba_msg.set_blue(rgba.blue);
        rgba_msg.set_opacity(rgba.opacity);
    }

    proto::Color& color_msg_;
};

void FillMessage(const io::MapRendererSettings& settings, proto::RenderSettings& message) {
    message.set_width(settings.width);
    message.set_height(settings.height);
    message.set_padding(settings.padding);
    message.set_line_width(settings.line_width);
    message.set_stop_radius(settings.stop_radius);
    message.set_bus_label_font_size(settings.bus_label_font_size);

    auto& bus_label_offset = *message.mutable_bus_label_offset();
    bus_label_offset.set_dx(settings.bus_label_offset[0]);
    bus_label_offset.set_dy(settings.bus_label_offset[1]);

    message.set_stop_label_font_size(settings.stop_label_font_size);

    auto& stop_label_offset = *message.mutable_stop_label_offset();
    stop_label_offset.set_dx(settings.stop_label_offset[0]);
    stop_label_offset.set_dy(settings.stop_label_offset[1]);

    std::visit(SetColorVisitor(*message.mutable_underlayer_color()), settings.underlayer_color);

    message.set_underlayer_width(settings.underlayer_width);

    for (const svg::Color& color : settings.color_palette) {
        std::visit(SetColorVisitor(*message.add_color_palette()), color);
    }
}

void FillMessage(const db::RoutingSettings& settings, proto::RoutingSettings& message) {
    message.set_bus_wait_time(settings.bus_wait_time);
    message.set_bus_velocity(settings.bus_velocity);
}

void FillMessage(const db::TransportRouter::Graph& graph, proto::Graph& message) {
    // edge index is edge id
    message.set_vertex_count(graph.GetVertexCount());
    for (auto edge : ranges::AsRange(graph.EdgesIterators())) {
        auto& edge_msg = *message.add_edge();
        edge_msg.set_from(edge.from);
        edge_msg.set_to(edge.to);
        edge_msg.set_weight(edge.weight);
    }
    assert(graph.GetEdgeCount() == message.edge_size());
}

void FillMessage(const db::TransportRouter::Router& router, proto::Router& message) {
    const auto& internal_data = router.InternalData();
    for (const auto& data_row : internal_data) {
        auto& data_row_msg = *message.add_data_row();
        for (const auto& data : data_row) {
            auto& opt_data = *data_row_msg.add_opt_data();
            if (data.has_value()) {
                auto& data_msg = *opt_data.add_data();        // optional<RouteInternalData>
                data_msg.set_weight(data->weight);
                if (data->prev_edge.has_value()) {
                    data_msg.add_prev_edge(*data->prev_edge); // optional<EdgeId> prev_edge
                }
            }
        }
    }
}

void FillMessage(const db::TransportRouter& router, proto::TransportRouter& message) {
    // Graph
    FillMessage(router.InternalGraph(), *message.mutable_graph());

    // Router
    FillMessage(router.InternalRouter(), *message.mutable_router());

    // StopVertices stop_vertices_
    const auto& stop_to_vertex = router.InternalStopToVertex();
    message.mutable_vertex_to_stop_id()->Resize(stop_to_vertex.size(), 0);
    for (const auto& stop_vertex : router.InternalStopToVertex()) {
        message.set_vertex_to_stop_id(stop_vertex.second,
                                      reinterpret_cast<uint64_t>(stop_vertex.first));
    }

    // Edges edges_
    for (const auto& edge : router.InternalEdges()) {
        auto *edge_msg = message.add_edge();
        edge_msg->set_wait(edge.wait);
        edge_msg->set_from_stop(reinterpret_cast<uint64_t>(edge.from));
        edge_msg->set_to_stop(reinterpret_cast<uint64_t>(edge.to));
        edge_msg->set_span(edge.span);
        edge_msg->set_bus(reinterpret_cast<uint64_t>(edge.bus));
    }
}

void FillMessage(const Base& base, proto::Base& base_msg) {

    FillMessage(base.transport_catalogue, *base_msg.mutable_transport_catalogue());
    FillMessage(base.render_settings, *base_msg.mutable_render_settings());
    FillMessage(base.routing_settings, *base_msg.mutable_routing_settings());
    assert(base.transport_router);
    FillMessage(*base.transport_router, *base_msg.mutable_transport_router());
}

bool Serialize(const Base& base, std::ostream& output) {
    proto::Base base_msg;
    FillMessage(base, base_msg);
    bool success = base_msg.SerializeToOstream(&output);
    return success;
}

using StopIdMap = unordered_map<uint64_t, const Stop*>;
using BusIdMap = unordered_map<uint64_t, const Bus*>;

std::tuple<StopIdMap, BusIdMap>
Parse(const proto::TransportCatalogue& tc_msg, db::TransportCatalogue& tc) {
    // stops
    StopIdMap id_to_stop;
    for (const auto& stop_msg : tc_msg.stop()) {
        assert(stop_msg.id() > 0);
        assert(!stop_msg.name().empty());
        assert(stop_msg.has_coordinates());
        geo::Coordinates coords(stop_msg.coordinates().lat(), stop_msg.coordinates().lng());
        const auto* added_stop = tc.AddStop(domain::Stop(stop_msg.name(), coords));
        id_to_stop[stop_msg.id()] = added_stop;
    }

    // distances
    for (const auto& distance : tc_msg.stops_distance()) {
        assert(distance.from_stop_id() > 0);
        assert(id_to_stop.count(distance.from_stop_id()) > 0);
        assert(distance.to_stop_id() > 0);
        assert(id_to_stop.count(distance.to_stop_id()) > 0);
        assert(distance.distance() > 0);
        tc.AddDistance(
            id_to_stop[distance.from_stop_id()],
            id_to_stop[distance.to_stop_id()],
            distance.distance());
    }

    // buses
    BusIdMap id_to_bus;
    for (const auto& bus : tc_msg.bus()) {
        assert(bus.id() > 0);
        assert(!bus.name().empty());
        assert(bus.stop_id_size() > 0);
        vector<const Stop*> stops;
        for (const auto& stop_id : bus.stop_id()) {
            assert(id_to_stop.count(stop_id) > 0);
            stops.push_back(id_to_stop[stop_id]);
        }
        const Bus* added_bus = tc.AddBus(Bus(bus.name(), stops.begin(), stops.end(),
        bus.route_type() == proto::Bus_RouteType::Bus_RouteType_LINEAR));
        id_to_bus[bus.id()] = added_bus;
    }

    return {move(id_to_stop), move(id_to_bus)};
}

svg::Color Parse(const proto::Color msg) {
    switch (msg.color_case()) {
        case proto::Color::kStr:{
            assert (!msg.str().empty());
            return {msg.str()};
        }
        case proto::Color::kRgb: {
            const auto& rgb = msg.rgb();
            return {svg::Rgb{(uint8_t) rgb.red(), (uint8_t) rgb.green(), (uint8_t) rgb.blue()}};
        }
        case proto::Color::kRgba: {
            const auto& rgba = msg.rgba();
            return {svg::Rgba{(uint8_t) rgba.red(), (uint8_t) rgba.green(), (uint8_t) rgba.blue(),
                            rgba.opacity()}};
        }
    }

    assert(false);
    std::terminate();
}

void Parse(const proto::RenderSettings& message, io::MapRendererSettings& settings) {
    settings.width = message.width();
    settings.height = message.height();
    settings.padding = message.padding();
    settings.line_width = message.line_width();
    settings.stop_radius = message.stop_radius();
    settings.bus_label_font_size = message.bus_label_font_size();
    settings.bus_label_offset[0] = message.bus_label_offset().dx();
    settings.bus_label_offset[1] = message.bus_label_offset().dy();
    settings.stop_label_font_size = message.stop_label_font_size();
    settings.stop_label_offset[0] = message.stop_label_offset().dx();
    settings.stop_label_offset[1] = message.stop_label_offset().dy();
    settings.underlayer_color = Parse(message.underlayer_color());
    settings.underlayer_width = message.underlayer_width();
    settings.color_palette.clear();
    for (const auto& color_msg : message.color_palette()) {
        settings.color_palette.push_back(Parse(color_msg));
    }
}

void Parse(const proto::RoutingSettings& message, db::RoutingSettings& settings) {
    assert(message.bus_wait_time() > 0);
    assert(message.bus_velocity() > 0);
    settings.bus_wait_time = message.bus_wait_time();
    settings.bus_velocity = message.bus_velocity();
}

unique_ptr<db::TransportRouter::Graph>
Parse(const proto::Graph& graph_msg) {
    auto graph_ptr = make_unique<db::TransportRouter::Graph>(graph_msg.vertex_count());
    auto& graph = *graph_ptr;
    for (const auto& edge_msg : graph_msg.edge()) {
        graph.AddEdge({edge_msg.from(), edge_msg.to(), edge_msg.weight()});
    }
    assert(graph.GetEdgeCount() == graph_msg.edge_size());
    return graph_ptr;
}

unique_ptr<db::TransportRouter::Router>
Parse(const proto::Router& router_msg, const db::TransportRouter::Graph& graph) {

    using RoutesInternalData = db::TransportRouter::Router::RoutesInternalData;
    using RouteInternalData = db::TransportRouter::Router::RouteInternalData;

    size_t vertex_count = router_msg.data_row_size();
    assert(vertex_count == graph.GetVertexCount());
    RoutesInternalData internal_data{vertex_count,
                                     std::vector<std::optional<RouteInternalData>>(vertex_count)};
    size_t i1 = 0;
    for (const auto& data_row_msg : router_msg.data_row()) {
        assert(data_row_msg.opt_data_size() == vertex_count); // must be N*N
        size_t i2 = 0;
        for (const auto& opt_data_msg : data_row_msg.opt_data()) {
            if (opt_data_msg.data_size() > 0) {
                // has value
                assert(opt_data_msg.data_size() == 1);
                const auto& data_msg = opt_data_msg.data(0);
                RouteInternalData data;
                data.weight = data_msg.weight();
                if (data_msg.prev_edge_size() > 0) {
                    assert(data_msg.prev_edge_size() == 1);
                    data.prev_edge = data_msg.prev_edge(0);
                } else {
                    assert(!data.prev_edge.has_value());
                }
                internal_data[i1][i2] = std::move(data);
            }
            ++i2;
        }
        ++i1;
    }
    return make_unique<db::TransportRouter::Router>(graph, std::move(internal_data));
}

unique_ptr<db::TransportRouter> Parse(const db::TransportCatalogue& tc,
db::RoutingSettings settings,
const StopIdMap& id_to_stop, const BusIdMap& id_to_bus,
const proto::TransportRouter& transport_router_msg) {

    auto graph = Parse(transport_router_msg.graph());
    auto router = Parse(transport_router_msg.router(), *graph);

    db::TransportRouter::StopVertices stop_vertices; // Stop* to vertix id
    int vertex_id = 0;
    for (const auto stop_id : transport_router_msg.vertex_to_stop_id()) {
        const Stop* stop = id_to_stop.at(stop_id);
        stop_vertices[stop] = vertex_id;
        ++vertex_id;
    }

    db::TransportRouter::Edges edges;
    for (const auto& edge_msg : transport_router_msg.edge()) {
        db::TransportRouter::EdgeData edge_data;
        edge_data.wait = edge_msg.wait();
        edge_data.from = id_to_stop.at(edge_msg.from_stop());
        edge_data.to = id_to_stop.at(edge_msg.to_stop());
        edge_data.span = edge_msg.span();
        edge_data.bus = id_to_bus.at(edge_msg.bus());
        edges.push_back(move(edge_data));
    }

    return make_unique<db::TransportRouter>(tc, move(settings), move(graph), move(router),
    move(stop_vertices), move(edges));
}

bool Deserialize(std::istream& input, Base& base) {
    proto::Base base_msg;
    bool success = base_msg.ParseFromIstream(&input);
    assert(success);
    if (!success)
        return false;

    assert(base_msg.has_transport_catalogue());
    auto [id_to_stop, id_to_bus] = Parse(base_msg.transport_catalogue(), base.transport_catalogue);

    assert(base_msg.has_render_settings());
    Parse(base_msg.render_settings(), base.render_settings);

    assert(base_msg.has_routing_settings());
    Parse(base_msg.routing_settings(), base.routing_settings);

    assert(base_msg.has_transport_router());
    base.transport_router = Parse(base.transport_catalogue, base.routing_settings,
                                  id_to_stop, id_to_bus, base_msg.transport_router());

    return true;
}

} // tcat::io::serialization