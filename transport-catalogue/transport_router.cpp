#include "transport_router.h"

namespace tcat::db {

using namespace std;

// Simple forward-and-backward iterator
// Iterate elements forward to the end by ordinary iterator
// and then backward by reverse iterator
// Primary usage for iterating over linear (non-round-trip) bus
// Forward iterator requirements:
// [input.iterators]:
//   a != b
//   *a
//   a->m
//   ++r
// [forward.iterators]
//   r++ -> const X&
//
// Iterator must be forward iterator
template <typename Container,
          typename Iterator,
          typename T = enable_if_t<std::is_base_of_v<forward_iterator_tag,
              typename std::iterator_traits<Iterator>::iterator_category>>>
class ForwardAndBackIterator {

public:

    using iterator_type = Iterator;

    using iterator_category = std::forward_iterator_tag;
    using value_type = typename iterator_traits<iterator_type>::value_type;
    using difference_type = typename iterator_traits<iterator_type>::difference_type;
    using pointer = typename iterator_traits<iterator_type>::pointer;
    using reference = typename iterator_traits<iterator_type>::reference;

    using reverse_iterator = typename std::reverse_iterator<Iterator>;
    using const_iterator = typename Container::const_iterator;

    ForwardAndBackIterator(Container& cont, iterator_type iter)
    : forward_(iter), rbegin_(cont.rbegin()), back_(cont.rbegin()) {}

    ForwardAndBackIterator(Container& cont, reverse_iterator iter)
    : forward_(cont.rbegin().base()), rbegin_(cont.rbegin()), back_(prev(iter)) {}

    bool operator==(iterator_type i) const {
        return back_ == rbegin_ && forward_ == i;
    }

    bool operator!=(iterator_type i) const {
        return !(*this == i);
    }

    bool operator==(reverse_iterator i) const {
        assert(i != rbegin_);
        return forward_ == rbegin_.base() && next(back_) == i;
    }

    bool operator!=(reverse_iterator i) const {
        return !(*this == i);
    }

    bool operator==(const ForwardAndBackIterator& other) const {
        return forward_ == other.forward_ && rbegin_ == other.rbegin_ && back_ == other.back_;
    }

    bool operator!=(const ForwardAndBackIterator& other) const {
        return !(*this == other);
    }

    reference operator*() const {
        if (forward_ != rbegin_.base())
            return *forward_;
        else {
            return *next(back_);
        }
    }

    pointer operator->() const {
        if (forward_ != rbegin_.base())
            return forward_.operator->();
        else {
            return next(back_).operator->();
        }
    }

    ForwardAndBackIterator& operator++() {
        if (forward_ != rbegin_.base()) {
            ++forward_;
        } else {
            ++back_;
        }
        return *this;
    }

    ForwardAndBackIterator operator++(int) {
        auto temp = *this;
        ++(*this);
        return temp;
    }

private:
    iterator_type forward_;         // points to current item during forward iterating
    reverse_iterator rbegin_;       // rbegin().base() == end()
    reverse_iterator back_;         // points to _previous_ item during backward iterating
};

TransportRouter::TransportRouter(const TransportCatalogue& tc, const RoutingSettings& settings)
    : tcat_(tc), settings_(settings) {
    InitializeGraph();
}

optional<TransportRouter::RouteResult> TransportRouter::Route(const Stop* from, const Stop* to) {
    if (!router_) {
        assert(graph_);
        router_ = make_unique<Router>(*graph_);        
    }
    auto route = router_->BuildRoute(GetStopVertex(from),
                                     GetStopVertex(to));
    if (!route.has_value()) {
        return nullopt;
    }
    RouteResult result;
    result.total_time = route->weight;

    for (const auto edge_id : route->edges) {
        const EdgeData& edge_data = edges_[edge_id];
        const Edge& edge = graph_->GetEdge(edge_id);

        assert(edge_data.from != nullptr);
        assert(edge_data.to != nullptr);
        assert(edge_data.span > 0);
        assert(edge_data.bus != nullptr);
        assert(edge_data.wait > 0);
        // edge is wait + bus activity
        result.activities.push_back(WaitActivity{edge_data.from, edge_data.wait});
        result.activities.push_back(BusActivity({edge_data.bus, edge_data.from,
                                                 edge_data.span, edge.weight - edge_data.wait}));
    }
    return result;
}

void TransportRouter::InitializeGraph() {
    const Weight bus_velocity = settings_.bus_velocity * 1000.0 / 60.0; // [meter/minute]
    const Weight bus_wait_time = settings_.bus_wait_time;    // [minute]

    // create stop vertices
    graph::VertexId vertex_id = 0;
    const auto [stops_begin, stops_end] = tcat_.StopsIterators();
    for (auto stop_it = stops_begin; stop_it != stops_end; ++stop_it) {
        stop_vertices_.insert({&*stop_it, vertex_id++});
    }

    // create graph
    graph_ = make_unique<Graph>(stop_vertices_.size());

    // add edges for buses
    const auto [buses_begin, buses_end] = tcat_.BusesIterators();
    for (auto bus_it = buses_begin; bus_it != buses_end; ++bus_it) {
        InitializeGraphAddBus(&*bus_it, bus_wait_time, bus_velocity);
    }
}

void TransportRouter::InitializeGraphAddBus(const Bus* bus, Weight bus_wait_time,
Weight bus_velocity) {
    const auto& stops = bus->Stops();
    assert(stops.size() > 1);

    assert(bus->Linear() || stops.back() == stops.front());

    // use ForwardAndBackIterator to iterate forward and then backward
    // over linear (non-round-trip) bus
    ForwardAndBackIterator stops_begin(stops, stops.begin());
    ForwardAndBackIterator stops_end =
        bus->Linear() ? ForwardAndBackIterator(stops, prev(stops.rend()))
                      : ForwardAndBackIterator(stops, prev(stops.end()));

    // add edges between all stop pairs of the bus
    for (auto from_it = stops_begin; from_it != stops_end; ++from_it) {
        VertexId from_vertex = GetStopVertex(*from_it);
        Distance distance = 0;
        int span = 1;
        for (auto to_it = from_it; to_it != stops_end; ++to_it) {
            VertexId to_vertex = GetStopVertex(*next(to_it));
            distance += tcat_.GetDistance(*to_it, *next(to_it));
            // edge weight is time in minutes
            auto transfer_edge_id = graph_->AddEdge({from_vertex,
                                                     to_vertex,
                                                     distance / bus_velocity + bus_wait_time});
            assert(transfer_edge_id == edges_.size());
            (void) transfer_edge_id; // remove warning: unused variable
            edges_.push_back({bus_wait_time, *from_it, *to_it, span, bus});
            ++span;
        }
    }
}

} // namespace tcat::db


