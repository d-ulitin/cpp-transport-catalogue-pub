#include <string>
#include <iostream>

#include "json_reader.h"
#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"

int main() {
    /*
     * Примерная структура программы:
     *
     * Считать JSON из stdin
     * Построить на его основе JSON базу данных транспортного справочника
     * Выполнить запросы к справочнику, находящиеся в массиве "stat_requests", построив JSON-массив
     * с ответами.
     * Вывести в stdout ответы в виде JSON
     */

    using namespace std;

    tcat::db::TransportCatalogue tc;
    auto document = json::Load(std::cin);
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