#include "input_reader.h"

#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>

namespace transport_catalogue::io {

using namespace std;

namespace detail {

string_view TrimWhitespace(string_view sv) {
    auto i1 = sv.find_first_not_of(' ');
    if (i1 != sv.npos)
        sv = sv.substr(i1);
    auto i2 = sv.find_last_not_of(' ');
    if (i2 != sv.npos)
        sv = sv.substr(0, i2 + 1);
    return sv;
}

using DistancesQueue = deque<tuple<const Stop*, string, Distance>>;

void InputStop(istream& input, TransportCatalogue& tc, DistancesQueue& distances) {
    // Format:
    // Stop X: latitude, longitude, D1m to stop1, D2m to stop2, ...

    string name;
    input >> std::ws;
    getline(input, name, ':');
    if (!input)
        throw runtime_error("invalid Stop query");
    input >> std::ws;
    double lat, lng;
    char comma;
    input >> lat >> comma >> lng;
    if (!input || comma != ',')
        throw runtime_error("invalid Stop query");
    const Stop *added_stop = tc.AddStop(Stop(move(name), {lat, lng}));
    char c;
    while (input >> c) {
        if (c != ',') {
            input.unget();
            break;
        }
        char suffix;
        Distance distance;
        string to;
        input >> distance >> suffix >> to >> std::ws;
        string to_stop;
        while (input.good()) {
            input.get(c);
            if (!input)
                break;
            if (c == ',' || c == '\n') {
                input.unget();
                break;
            }
            to_stop.push_back(c);
        }
        if (suffix != 'm' || to != "to" || to_stop.empty())
            throw runtime_error("invalid Stop query");
        distances.push_back(make_tuple(added_stop, move(to_stop), distance));
    }
}

void InputBus(istream& input, TransportCatalogue& tc) {
    // Format:
    // Bus X: stop1 - stop2 - ... stopN
    // Bus X: stop1 > stop2 > ... > stopN > stop1
    string bus_name;
    input >> std::ws;
    getline(input, bus_name, ':');
    string stop_name;
    vector<const Stop*> stops;
    char route_type = 0;
    input >> std::ws;
    while (true) {
        char c;
        input.get(c); // unformatted input, no skipping whitespace
        if (input.fail() || c == '>' || c == '-' || c == '\n') {
            if (!input.fail() && (c == '>' || c == '-')) {
                if (route_type == 0)
                    route_type = c;
                else if (route_type != c)
                    throw runtime_error("invalid route type");
            }

            if (stop_name.empty())
                throw runtime_error("invalid stop name");
            string_view sv = stop_name;
            sv = TrimWhitespace(sv);
            const Stop *stop = tc.GetStop(sv);
            if (!stop)
                throw runtime_error("stop \""s + string(sv) + "\" not found"s);
            stops.push_back(stop);
            stop_name.clear();
            
            if (c == '\n' || input.fail())
                break;
        } else {
            stop_name += c;
        }
    }
    tc.AddBus(Bus(move(bus_name), stops.begin(), stops.end(), route_type == '-'));
}

} // namespace detail

void InputBatch(istream& input, TransportCatalogue& tc) {
    int queries_number;
    input >> queries_number;
    if (!input)
        throw runtime_error("can't read number of queries");

    detail::DistancesQueue distances;
    deque<string> buses;
    for (int i = 0; i < queries_number; ++i) {
        string command;
        input >> command;
        if (command == "Stop") {
            detail::InputStop(input, tc, distances);
        } else if (command == "Bus") {
            input >> ws;
            string input_bus;
            getline(input, input_bus);
            buses.push_back(move(input_bus));
        } else {
            throw runtime_error("unknown query command \""s + command + '"');
        }
    }

    // input stops distances
    while (!distances.empty()) {
        auto d = distances.front();
        const Stop* stop2 = tc.GetStop(get<1>(d));
        tc.AddDistance(get<0>(d), stop2, get<2>(d));
        distances.pop_front();
    }

    // input buses
    while (!buses.empty()) {
        istringstream input(buses.front());
        detail::InputBus(input, tc);
        buses.pop_front();
    }
}


} // namespace transport_catalogue::io