#include <string>
#include <string_view>
#include <iostream>
#include <fstream>
#include <cstring>

#include "json_reader.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "json.h"
#include "serialization.h"

using namespace std::literals;
using namespace tcat;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {

    using namespace std;

    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    istream *in = &std::cin;
    unique_ptr<ifstream> in_file;
    if (argc == 3) {
        in_file = make_unique<ifstream>(argv[2]);
        if (!in_file->good())
            throw invalid_argument("Can't open file "s + argv[2] + ": " + strerror(errno));
        in = in_file.get();
    }

    if (mode == "make_base"sv) {

        db::TransportCatalogue transport_catalogue;

        // read base_requests
        auto document = json::Load(*in);
        io::JsonRequestReader json_reader(transport_catalogue);
        json_reader.ReadBase(document);

        // read settings
        auto renderer_settings = json_reader.ReadRendererSettings(document);
        auto routing_settings = json_reader.ReadRoutingSettings(document);

        // create transport router
        auto transport_router = std::make_unique<db::TransportRouter>(transport_catalogue, routing_settings);

        // serialize
        const auto serialization_settings = json_reader.ReadSerializationSettings(document);
        ofstream out(serialization_settings.file, ios::binary);
        assert(out.good());
        io::serialization::Base base{transport_catalogue,
                                     renderer_settings,
                                     routing_settings,
                                     transport_router};
        io::serialization::Serialize(base, out);

    } else if (mode == "process_requests"sv) {

        db::TransportCatalogue transport_catalogue;
        io::MapRendererSettings render_settings;
        db::RoutingSettings routing_settings;
        std::unique_ptr<db::TransportRouter> transport_router;

        // load json
        auto document = json::Load(*in);
        io::JsonRequestReader json_reader(transport_catalogue);

        // open input strem
        const auto serialization_settings = json_reader.ReadSerializationSettings(document);
        ifstream input(serialization_settings.file, ios::binary);
        assert(input.good());

        // deserialize
        io::serialization::Base base{transport_catalogue,
                                     render_settings,
                                     routing_settings,
                                     transport_router};
        io::serialization::Deserialize(input, base);
        assert(transport_router);

        // process stat_requests
        auto stat = json_reader.ReadStat(document, render_settings, *transport_router);
        json::Document stat_document{stat};
        json::Print(stat_document, std::cout);

    } else {
        PrintUsage();
        return 1;
    }
}