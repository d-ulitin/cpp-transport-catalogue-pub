#include <string>
#include <iostream>
#include <fstream>
#include <cstring>

#include "json_reader.h"
#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"

int main(int argc, char *argv[]) {

    using namespace std;

    istream *in = &std::cin;
    unique_ptr<ifstream> in_file;
    if (argc == 2) {
        in_file = make_unique<ifstream>(argv[1]);
        if (!in_file->good())
            throw invalid_argument("Can't open file "s + argv[1] + ": " + strerror(errno));
        in = in_file.get();
    }

    tcat::db::TransportCatalogue tc;
    auto document = json::Load(*in);
    tcat::io::JsonRequestReader json_reader(tc);
    json_reader.ReadBase(document);
    auto stat = json_reader.ReadStat(document);
    json::Document stat_document{stat};
    json::Print(stat_document, std::cout);
    // tcat::io::MapRendererSettings settings = json_reader.ReadRendererSettings(document);
    // tcat::io::MapRenderer map_renderer(tc, settings);
    // svg::Document doc = map_renderer.Render();
    // doc.Render(std::cout);
}