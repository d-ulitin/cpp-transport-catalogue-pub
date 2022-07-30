#pragma once

#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

#include <optional>
#include <variant>
#include <memory>

namespace tcat::db {

struct RoutingSettings {
    int bus_wait_time = 6;
    double bus_velocity = 60;
};

class TransportRouter {

public:

    using Weight = double;
    using Edge = graph::Edge<Weight>;
    using Graph = graph::DirectedWeightedGraph<Weight>;
    using Router = graph::Router<Weight>;
    using VertexId = graph::VertexId;

    TransportRouter(const TransportCatalogue& tc, const RoutingSettings& settings);

    struct WaitActivity {
        const Stop* stop;
        double time;
    };

    struct BusActivity {
        const Bus* bus;
        const Stop* from;
        int span;
        double time;
    };

    using Activity = std::variant<WaitActivity, BusActivity>;

    struct RouteResult {
        double total_time;
        std::vector<Activity> activities;
    };

    std::optional<RouteResult> Route(const Stop* from, const Stop* to);

    // internal types for serialization
    using StopVertices = std::unordered_map<const Stop*, VertexId>;
    struct EdgeData {
        double wait;
        const Stop* from;
        const Stop* to;
        int span;
        const Bus* bus;
    };
    using Edges = std::vector<EdgeData>;

    // accessors to internal fields
    const auto& InternalGraph() const { return *graph_; }
    const auto& InternalRouter() const { return *router_; }
    const auto& InternalStopToVertex() const { return stop_vertices_; }
    const auto& InternalEdges() const { return edges_; }

    // constructor with internal fields
    TransportRouter(const TransportCatalogue& tc, RoutingSettings&& settings,
    std::unique_ptr<Graph>&& graph, std::unique_ptr<Router>&& router,
    StopVertices&& stop_vertices, Edges&& edges);

private:
    const TransportCatalogue & tcat_;
    const RoutingSettings settings_;

    std::unique_ptr<Graph> graph_;
    std::unique_ptr<Router> router_;

    void InitializeGraph();
    void InitializeGraphAddBus(const Bus* bus, Weight bus_wait_time, Weight bus_velocity);

    StopVertices stop_vertices_;

    inline graph::VertexId GetStopVertex(const Stop* stop) const {
        auto it = stop_vertices_.find(stop);
        assert(it != stop_vertices_.end());
        return it->second;
    }

    Edges edges_;
};

} // namespace tcat::db