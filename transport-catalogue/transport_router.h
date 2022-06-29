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

private:
    const TransportCatalogue & tcat_;
    const RoutingSettings settings_;

    using Weight = double;
    using Edge = graph::Edge<Weight>;
    using Graph = graph::DirectedWeightedGraph<Weight>;
    using Router = graph::Router<Weight>;
    using VertexId = graph::VertexId;

    std::unique_ptr<Graph> graph_;
    std::unique_ptr<Router> router_;

    void InitializeGraph();
    void InitializeGraphAddBus(const Bus* bus, Weight bus_wait_time, Weight bus_velocity);

    std::unordered_map<const Stop*, VertexId> stop_vertices_;

    inline graph::VertexId GetStopVertex(const Stop* stop) const {
        auto it = stop_vertices_.find(stop);
        assert(it != stop_vertices_.end());
        return it->second;
    }

    struct EdgeData {
        double wait;
        const Stop* from;
        const Stop* to;
        int span;
        const Bus* bus;
    };
    std::vector<EdgeData> edges_;
};

} // namespace tcat::db