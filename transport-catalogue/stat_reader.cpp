#include "stat_reader.h"
#include "transport_catalogue.h"

#include <iostream>
#include <iomanip>
#include <string>

namespace transport_catalogue::io {

using namespace std;

namespace detail {

void StatBus(istream& input, TransportCatalogue& tc, ostream& out) {
    // Format:
    // Bus X: R stops on route, U unique stops, L route length, C curvature
    input >> std::ws;
    string bus_name;
    getline(input, bus_name);
    if (!input)
        throw runtime_error("invalid Bus query");
    out << "Bus " << bus_name << ": ";
    const Bus *bus = tc.GetBus(bus_name);
    if (bus) {
        Distance route_length = tc.RouteLength(bus);
        out << bus->StopsNumber() << " stops on route, "
        << bus->UniqueStops().size() << " unique stops, "
        << static_cast<double>(route_length) << " route length, "
        << route_length / bus->GeoLength() << " curvature";
    }
    else {
        out << "not found";
    }
    out << '\n';
}

void StatStop(istream& input, TransportCatalogue& tc, ostream& out) {
    input >> std::ws;
    string stop_name;
    getline(input, stop_name);
    if (!input)
        throw runtime_error("invalid Stop query");
    out << "Stop " << stop_name << ": ";
    const Stop *stop = tc.GetStop(stop_name);
    if (stop) {
        auto buses = tc.GetBusesForStop(stop);
        if (buses.empty())
            out << "no buses";
        else {
            out << "buses ";
            bool first = true;
            for (auto& bus : buses) {
                if (!first)
                    out << ' ';
                out << bus->Name();
                first = false;
            }
        }
    } else {
        out << "not found";
    }
    out << '\n';
}

} // namespace detail

void StatBatch(istream& input, TransportCatalogue& tc, ostream& out) {
    out << std::setprecision(6);
    int queries_number;
    input >> queries_number;
    if (input.fail())
        throw runtime_error("can't read number of queries");
    input >> std::ws;
    for (int i = 0; i < queries_number; ++i) {
        string command;
        input >> command;
        if (command == "Bus"s) {
            detail::StatBus(input, tc, out);
        } else if (command == "Stop"s) {
            detail::StatStop(input, tc, out);
        }
        else {
            throw runtime_error("unknown command \""s + command + '"');
        }
    }
}

} // namespace transport_catalogue::io