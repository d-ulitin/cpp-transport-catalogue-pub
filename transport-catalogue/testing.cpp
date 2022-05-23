#include "testing.h"

#include <cassert>
#include <cmath>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include "geo.h"
#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

namespace transport_catalogue::testing {

using namespace std;

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Test>
void RunTestImpl(Test func, const string& func_name) {
    func();
    cerr << func_name << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)

ostream& operator<<(ostream& os, const Coordinates crd) {
    os << "(" << crd.lat << ", " << crd.lng << ")";
    return os;
}

void TestCoordinates() {
    Coordinates c1(0,0);
    Coordinates c2(c1);
    ASSERT_EQUAL(c1, c2);
    Coordinates c3(90, 0.0); // north pole
    Coordinates c4(-90, 0.0); // south pole
    ASSERT(c3 != c4);
    static const double earth_radius = 6371. * 1e3;
    static const double large_dist_eps = 100 * 1e3; // accuracy 100km
    static const double small_dist_eps = 1e-3;
    ASSERT(std::abs(ComputeDistance(c3, c4) - 3.14 * earth_radius) < large_dist_eps);
    Coordinates c5(0,-180);
    Coordinates c6(0,+180); // the same point
    ASSERT(c5 != c6);
    ASSERT(std::abs(ComputeDistance(c5, c6)) < small_dist_eps);
    Coordinates c7(90,0);
    Coordinates c8(90,10); // the same point
    ASSERT(c7 != c8);
    ASSERT(std::abs(ComputeDistance(c7, c8)) < small_dist_eps);
    Coordinates london(51.5287336, -0.382471);
    Coordinates sydney(-33.8473551,150.651095);
    ASSERT(std::abs(ComputeDistance(london, sydney) - 17000e3) < large_dist_eps);
}

void TestStop() {
    Stop s1("stop1", {10, 20});
    ASSERT_EQUAL(s1.Name(), "stop1");
    ASSERT_EQUAL(s1.GetCoordinates(), Coordinates(10, 20));
    Stop s11 = move(s1);
    ASSERT_EQUAL(s11.Name(), "stop1");
    ASSERT_EQUAL(s11.GetCoordinates(), Coordinates(10, 20));
}

void TestStopPointerHasher() {
    Stop s1("stop1", {10, 20});
    ASSERT(StopPointerHasher()(&s1) == StopPointerHasher()(&s1));
    Stop s1_copy = s1;
    ASSERT(StopPointerHasher()(&s1) != StopPointerHasher()(&s1_copy));
}

void TestPairOfStopsPointersHasher() {
    Stop s1("stop1", {10, 20});
    Stop s2("stop2", {10, 20});
    ASSERT(PairOfStopsPointersHasher()(make_pair(&s1, &s2)) == PairOfStopsPointersHasher()(make_pair(&s1, &s2)));
    ASSERT(PairOfStopsPointersHasher()(make_pair(&s1, &s2)) != PairOfStopsPointersHasher()(make_pair(&s2, &s1)));
    ASSERT(PairOfStopsPointersHasher()(make_pair(&s1, &s2)) != PairOfStopsPointersHasher()(make_pair(&s1, &s1)));
    ASSERT(PairOfStopsPointersHasher()(make_pair(&s1, &s2)) != PairOfStopsPointersHasher()(make_pair(&s2, &s2)));
}

void TestBus() {
    {
        Stop s1("stop1", {10, 20});
        Stop s2("stop2", {11, 21});
        Stop s3("stop3", {12, 22});
        vector<Stop*> stops = {&s1, &s2, &s3};
        Bus bus("bus", stops.begin(), stops.end(), true);
        ASSERT_EQUAL(bus.Name(), "bus");
        ASSERT_EQUAL(bus.Stops().size(), 3U);
        ASSERT_EQUAL(bus.Stops().at(0), stops.at(0));
        ASSERT_EQUAL(bus.Stops().at(1), stops.at(1));
        ASSERT_EQUAL(bus.Stops().at(2), stops.at(2));
        ASSERT_EQUAL(bus.StopsNumber(), 5u);
        double dist = ComputeDistance(s1.GetCoordinates(), s2.GetCoordinates());
        dist += ComputeDistance(s2.GetCoordinates(), s3.GetCoordinates());
        ASSERT(std::abs(bus.GeoLength() - 2 * dist) < 1e-3);
        ASSERT_EQUAL(bus.Linear(), true);
    }
    {
        Stop s1("stop1", {10, 20});
        Stop s2("stop2", {11, 21});
        Stop s3("stop3", {12, 22});
        vector<Stop*> stops = {&s1, &s2, &s3, &s1};
        Bus bus("bus", stops.begin(), stops.end(), false);
        ASSERT_EQUAL(bus.Name(), "bus");
        ASSERT_EQUAL(bus.Stops().size(), 4u);
        ASSERT_EQUAL(bus.Stops().at(0), stops.at(0));
        ASSERT_EQUAL(bus.Stops().at(1), stops.at(1));
        ASSERT_EQUAL(bus.Stops().at(2), stops.at(2));
        ASSERT_EQUAL(bus.StopsNumber(), 4u);
        double dist = ComputeDistance(s1.GetCoordinates(), s2.GetCoordinates());
        dist += ComputeDistance(s2.GetCoordinates(), s3.GetCoordinates());
        dist += ComputeDistance(s3.GetCoordinates(), s1.GetCoordinates());
        ASSERT(std::abs(bus.GeoLength() - dist) < 1e-3);
        ASSERT_EQUAL(bus.Linear(), false);
    }
}

// use these operators only for testing purposes

bool operator==(const Stop& s1, const Stop& s2) {
    return s1.Name() == s2.Name()
        && s1.GetCoordinates() == s2.GetCoordinates();
}

bool operator!=(const Stop& s1, const Stop& s2) {
    return !(s1 == s2);
}

namespace transport_catalogue {

void TestAddStop() {
    TransportCatalogue tc;
    Stop s1("stop1", {10, 20});
    const Stop* s1a = tc.AddStop(s1);
    ASSERT_EQUAL(s1.Name(), s1a->Name());
    ASSERT_EQUAL(s1.GetCoordinates(), s1a->GetCoordinates());
    auto& s1b = *tc.GetStop("stop1");
    ASSERT_EQUAL(s1.Name(), s1b.Name());
    ASSERT_EQUAL(s1.GetCoordinates(), s1b.GetCoordinates());

    Stop s2("stop2", {-10, -20});
    tc.AddStop(s2);
    auto& s21 = *tc.GetStop("stop2");
    ASSERT_EQUAL(s2.Name(), s21.Name());
    ASSERT_EQUAL(s2.GetCoordinates(), s21.GetCoordinates());
}

void TestAddBus() {
    TransportCatalogue tc;
    Stop s1("stop1", {10, 20});
    Stop s2("stop2", {-10, -20});
    vector<Stop*> stops = {&s1, &s2};
    Bus b1("b1", stops.begin(), stops.end(), false);
    tc.AddBus(b1);
    auto& b11 = *tc.GetBus("b1");
    ASSERT_EQUAL(b11.Name(), "b1");
    ASSERT_EQUAL(b11.Stops().size(), 2u);
    ASSERT_EQUAL(b11.Stops().at(0)->Name(), "stop1");
    ASSERT_EQUAL(b11.Stops().at(1)->Name(), "stop2");
    ASSERT_EQUAL(b11.Linear(), false);
}

void TestGetBusesForStop() {
    TransportCatalogue tc;
    const Stop *s0 = tc.AddStop(Stop{"stop0", {0, 1}});
    const Stop *s1 = tc.AddStop(Stop{"stop1", {10, 11}});
    const Stop *s2 = tc.AddStop(Stop{"stop2", {20, 21}});
    const Stop *s3 = tc.AddStop(Stop{"stop3", {30, 31}});
    vector<const Stop*> stops = {s1, s2, s3};
    const Bus *b1 = tc.AddBus(Bus{"bus1", stops.begin(), stops.end(), true});
    const Bus *b2 = tc.AddBus(Bus{"bus2", next(stops.begin()), stops.end(), true});
    const Bus *b3 = tc.AddBus(Bus{"bus3", stops.begin(), prev(stops.end()), true});
    ASSERT(tc.GetBusesForStop(s0) == TransportCatalogue::BusSet{});
    ASSERT((tc.GetBusesForStop(s1) == TransportCatalogue::BusSet{b1, b3}));
    ASSERT((tc.GetBusesForStop(s2) == TransportCatalogue::BusSet{b1, b2, b3}));
    ASSERT((tc.GetBusesForStop(s3) == TransportCatalogue::BusSet{b1, b2}));
}

void TestDistance() {
    TransportCatalogue tc;
    const Stop *s1 = tc.AddStop(Stop{"stop1", {10, 11}});
    const Stop *s2 = tc.AddStop(Stop{"stop2", {20, 21}});
    const Stop *s3 = tc.AddStop(Stop{"stop3", {30, 31}});
    tc.AddDistance(s1, s2, 1);
    ASSERT_EQUAL(tc.GetDistance(s1, s2), 1u);
    ASSERT_EQUAL(tc.GetDistance(s2, s1), 1u);
    tc.AddDistance(s2, s3, 2u);
    tc.AddDistance(s3, s2, 2u);
    ASSERT_EQUAL(tc.GetDistance(s2, s3), 2u);
    ASSERT_EQUAL(tc.GetDistance(s3, s2), 2u);
    tc.AddDistance(s1, s3, 3u);
    tc.AddDistance(s3, s1, 10u);
    ASSERT_EQUAL(tc.GetDistance(s1, s3), 3u);
    ASSERT_EQUAL(tc.GetDistance(s3, s1), 10u);
}

void TestRouteLength() {
    {
        TransportCatalogue tc;
        const Stop *s1 = tc.AddStop(Stop{"stop1", {10, 11}});
        const Stop *s2 = tc.AddStop(Stop{"stop2", {20, 21}});
        const Stop *s3 = tc.AddStop(Stop{"stop3", {30, 31}});
        const Stop *s4 = tc.AddStop(Stop{"stop4", {40, 41}});
        tc.AddDistance(s1, s2, 1);
        tc.AddDistance(s2, s3, 2);
        tc.AddDistance(s3, s2, 2);
        tc.AddDistance(s3, s4, 3);
        tc.AddDistance(s4, s3, 30);
        vector<const Stop*> stops = {s1, s2, s3, s4};
        const Bus *b1 = tc.AddBus(Bus{"bus1", stops.begin(), stops.end(), false});
        const Bus *b2 = tc.AddBus(Bus{"bus2", stops.rbegin(), stops.rend(), false});
        const Bus *b3 = tc.AddBus(Bus{"bus3", stops.begin(), stops.end(), true});
        const Bus *b4 = tc.AddBus(Bus{"bus4", stops.rbegin(), stops.rend(), true});
        ASSERT_EQUAL(tc.RouteLength(b1), 6u);
        ASSERT_EQUAL(tc.RouteLength(b2), 33u);
        ASSERT_EQUAL(tc.RouteLength(b3), 39u);
        ASSERT_EQUAL(tc.RouteLength(b4), 39u);
    }
    {
        TransportCatalogue tc;
        const Stop *s1 = tc.AddStop(Stop{"stop1", {10, 11}});
        const Stop *s2 = tc.AddStop(Stop{"stop2", {20, 21}});
        const Stop *s3 = tc.AddStop(Stop{"stop3", {30, 31}});
        tc.AddDistance(s1, s2, 1);
        tc.AddDistance(s2, s2, 2); // the same stop
        tc.AddDistance(s2, s3, 3);
        tc.AddDistance(s3, s2, 5);
        vector<const Stop*> stops = {s1, s2, s2, s3};
        const Bus *b1 = tc.AddBus(Bus{"bus1", stops.begin(), stops.end(), false});
        const Bus *b2 = tc.AddBus(Bus{"bus2", stops.rbegin(), stops.rend(), false});
        const Bus *b3 = tc.AddBus(Bus{"bus3", stops.begin(), stops.end(), true});
        const Bus *b4 = tc.AddBus(Bus{"bus4", stops.rbegin(), stops.rend(), true});
        ASSERT_EQUAL(tc.RouteLength(b1), 6u);
        ASSERT_EQUAL(tc.RouteLength(b2), 8u);
        ASSERT_EQUAL(tc.RouteLength(b3), 14u);
        ASSERT_EQUAL(tc.RouteLength(b4), 14u);

    }
}

} // namespace transport_catalogue

void TestInputBatch() {
    TransportCatalogue tc;
    static const char *text = 
        "4\n"
        "Stop Waterloo Station: 51.5039062,-0.1216578\n"
        "Stop Paddington Station: 51.5166747,-0.2460996\n"
        "Bus Bus 1: Waterloo Station - Paddington Station\n"
        "Bus Bus 2: Paddington Station > Waterloo Station\n";
    istringstream input(text);
    io::InputBatch(input, tc);
    auto& stop1 = *tc.GetStop("Waterloo Station");
    ASSERT((stop1.GetCoordinates() == Coordinates{51.5039062,-0.1216578}));
    auto& stop2 = *tc.GetStop("Paddington Station");
    ASSERT((stop2.GetCoordinates() == Coordinates{51.5166747,-0.2460996}));
    auto& bus1 = *tc.GetBus("Bus 1");
    ASSERT(bus1.Name() == "Bus 1");
    ASSERT(bus1.Linear() == true);
    ASSERT(bus1.Stops().size() == 2);
    auto& bus2 = *tc.GetBus("Bus 2");
    ASSERT(bus2.Name() == "Bus 2");
    ASSERT(bus2.Linear() == false);
    ASSERT(bus2 .Stops().size() == 2);
}

void TestExampleC() {
    TransportCatalogue tc;
    static const char *input =
        "13\n"
        "Stop Tolstopaltsevo: 55.611087, 37.20829, 3900m to Marushkino\n"
        "Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino\n"
        "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
        "Bus 750: Tolstopaltsevo - Marushkino - Marushkino - Rasskazovka\n"
        "Stop Rasskazovka: 55.632761, 37.333324, 9500m to Marushkino\n"
        "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam\n"
        "Stop Biryusinka: 55.581065, 37.64839, 750m to Universam\n"
        "Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya\n"
        "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656, 1300m to Biryulyovo Passazhirskaya\n"
        "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164, 1200m to Biryulyovo Zapadnoye\n"
        "Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n"
        "Stop Rossoshanskaya ulitsa: 55.595579, 37.605757\n"
        "Stop Prazhskaya: 55.611678, 37.603831\n"
        "6\n"
        "Bus 256\n"
        "Bus 750\n"
        "Bus 751\n"
        "Stop Samara\n"
        "Stop Prazhskaya\n"
        "Stop Biryulyovo Zapadnoye\n";
    istringstream sin(input);
    io::InputBatch(sin, tc);
    ostringstream sout;
    io::StatBatch(sin, tc, sout);
    static const char *output =
        "Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.36124 curvature\n"
        "Bus 750: 7 stops on route, 3 unique stops, 27400 route length, 1.30853 curvature\n"
        "Bus 751: not found\n"
        "Stop Samara: not found\n"
        "Stop Prazhskaya: no buses\n"
        "Stop Biryulyovo Zapadnoye: buses 256 828\n";
    ASSERT_EQUAL(sout.str(), output);
}

void TestAll() {
    RUN_TEST(TestCoordinates);
    RUN_TEST(TestStop);
    RUN_TEST(TestStopPointerHasher);
    RUN_TEST(TestPairOfStopsPointersHasher);
    RUN_TEST(TestBus);
    RUN_TEST(transport_catalogue::TestAddStop);
    RUN_TEST(transport_catalogue::TestAddBus);
    RUN_TEST(transport_catalogue::TestGetBusesForStop);
    RUN_TEST(transport_catalogue::TestDistance);
    RUN_TEST(transport_catalogue::TestRouteLength);
    RUN_TEST(TestInputBatch);
    RUN_TEST(TestExampleC);
}


} // namespace transport_catalogue::testing