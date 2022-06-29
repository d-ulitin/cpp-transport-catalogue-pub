#include "map_renderer.h"

#include <set>
#include <string>
#include <string_view>
#include <unordered_map>

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */


namespace tcat::io
{

using namespace std;

inline const double EPSILON = 1e-6;

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};


//
// MapRenderer
//

MapRenderer::MapRenderer(db::TransportCatalogue& tc, const MapRendererSettings& settings)
    : tc_(tc), settings_(settings) {
}

svg::Document MapRenderer::Render() {

    // get stops coordinates with at least one bus
    // and stop names sorted by lexicographic order
    vector<geo::Coordinates> stops_coords;
    set<string_view> stops;
    auto [stops_begin, stops_end] = tc_.StopsIterators();
    for (auto stop_it = stops_begin; stop_it != stops_end; ++stop_it) {
        auto& stop = *stop_it;
        if (!tc_.GetBuses(&stop).empty()) {
            stops_coords.push_back(stop.GetCoordinates());
            stops.insert(stop.Name());
        }
    }

    SphereProjector projector(stops_coords.begin(), stops_coords.end(),
        settings_.width, settings_.height, settings_.padding);
    
    // get bus names sorted by lexicographic excluding buses with no stops 
    set<string_view> buses;
    auto [buses_begin, buses_end] = tc_.BusesIterators();
    for (auto bus_it = buses_begin; bus_it != buses_end; ++bus_it) {
        auto& bus = *bus_it;
        if (bus.StopsNumber() > 0)
            buses.insert(bus.Name());
    }

    // Карта состоит из четырёх типов объектов. Порядок их вывода в SVG-документ:
    // - ломаные линии маршрутов,
    // - названия маршрутов,
    // - круги, обозначающие остановки,
    // - названия остановок.

    vector<unique_ptr<svg::Object>> bus_lines;
    vector<unique_ptr<svg::Object>> bus_names;

    size_t bus_counter = 0;
    for (const string_view bus_name : buses) {
        size_t color_idx = bus_counter % settings_.color_palette.size();
        const svg::Color& color = settings_.color_palette.at(color_idx);
        auto bus = tc_.GetBus(bus_name);
        RenderBusLines(bus, projector, color, back_inserter(bus_lines));
        RenderBusName(bus, projector, color, back_inserter(bus_names));
        ++bus_counter;
    }

    vector<unique_ptr<svg::Object>> stop_symbols;
    vector<unique_ptr<svg::Object>> stop_names;

    for (const string_view stop_name : stops) {
        auto stop = tc_.GetStop(stop_name);
        RenderStopSymbol(stop, projector, back_inserter(stop_symbols));
        RenderStopName(stop, projector, back_inserter(stop_names));
    }

    // create svg document
    svg::Document doc;

    for (auto& obj : bus_lines)
        doc.AddPtr(move(obj));
    bus_lines.clear(); // all objects have unspecified values, clear just in case

    for (auto& obj : bus_names)
        doc.AddPtr(move(obj));
    bus_names.clear();

    for (auto& obj : stop_symbols)
        doc.AddPtr(move(obj));
    stop_symbols.clear();

    for (auto& obj : stop_names)
        doc.AddPtr(move(obj));
    stop_names.clear();

    return doc;
}

template <typename BackInsertIter>
void MapRenderer::RenderBusLines(const domain::Bus* bus, const SphereProjector& projector,
    svg::Color color, BackInsertIter it) {

    auto stops = bus->Stops();

    if (stops.empty())
        return;

    auto polyline = make_unique<svg::Polyline>();
    polyline->SetStrokeColor(color)
             .SetFillColor(svg::NoneColor)
             .SetStrokeWidth(settings_.line_width)
             .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
             .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    for (auto stop : stops) {
        polyline->AddPoint(projector(stop->GetCoordinates()));
    }
    if (bus->Linear()) {
        for (auto it = next(stops.rbegin()); it != stops.rend(); ++it)
            polyline->AddPoint(projector((*it)->GetCoordinates()));
    }
    it = move(polyline);
}

template <typename BackInsertIter>
void MapRenderer::RenderBusName(const domain::Bus* bus, const SphereProjector& projector,
    svg::Color color, BackInsertIter it) {

    auto stops = bus->Stops();
    
    if (stops.empty())
        return;

    /*    
        Название маршрута должно отрисовываться у каждой из его конечных остановок.
        В кольцевом маршруте — когда "is_roundtrip": true — конечной считается первая
        остановка маршрута. А в некольцевом — первая и последняя.
        Названия маршрутов должны быть нарисованы в алфавитном порядке. Для каждого маршрута
        сначала выводится название для его первой конечной остановки, а затем, если маршрут
        некольцевой и конечные не совпадают, — для второй конечной.
        Если остановок у маршрута нет, его название выводиться не должно.
    */
   
    auto pos_front = projector(stops.front()->GetCoordinates());
    RenderBusNameLabel(bus, pos_front, color, it);
    if (bus->Linear() && stops.size() > 1 && stops.front() != stops.back()) {
        auto pos_back = projector(stops.back()->GetCoordinates());
        RenderBusNameLabel(bus, pos_back, color, it);
    }
}
    
template <typename BackInsertIter>
void MapRenderer::RenderBusNameLabel(const domain::Bus* bus, svg::Point position,
    svg::Color color, BackInsertIter it) {

    svg::Text base;
    base.SetPosition(position)
        .SetOffset(svg::Point{settings_.bus_label_offset[0], settings_.bus_label_offset[1]})
        .SetFontSize(static_cast<uint32_t>(settings_.bus_label_font_size))
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetData(bus->Name());

    auto back = make_unique<svg::Text>(base);
    back->SetFillColor(settings_.underlayer_color)
         .SetStrokeColor(settings_.underlayer_color)
         .SetStrokeWidth(settings_.underlayer_width)
         .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
         .SetStrokeLineCap(svg::StrokeLineCap::ROUND);

    auto text = make_unique<svg::Text>(base);
    text->SetFillColor(color);

    it = move(back);
    it = move(text);
}

template <typename BackInsertIter>
void MapRenderer::RenderStopSymbol(const domain::Stop * stop, const SphereProjector& projector,
    BackInsertIter it) {

    auto circle = make_unique<svg::Circle>();
    circle->SetCenter(projector(stop->GetCoordinates()))
            .SetRadius(settings_.stop_radius)
            .SetFillColor("white"s);
    it = move(circle);
}

template <typename BackInsertIter>
void MapRenderer::RenderStopName(const domain::Stop * stop, const SphereProjector& projector,
    BackInsertIter it) {

    auto position = projector(stop->GetCoordinates());

    svg::Text base;
    base.SetPosition(position)
        .SetOffset(svg::Point{settings_.stop_label_offset[0], settings_.stop_label_offset[1]})
        .SetFontSize(static_cast<uint32_t>(settings_.stop_label_font_size))
        .SetFontFamily("Verdana"s)
        .SetData(stop->Name());

    auto back = make_unique<svg::Text>(base);
    back->SetFillColor(settings_.underlayer_color)
         .SetStrokeColor(settings_.underlayer_color)
         .SetStrokeWidth(settings_.underlayer_width)
         .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
         .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    auto text = make_unique<svg::Text>(base);
    text->SetFillColor("black"s);

    it = move(back);
    it = move(text);
}

} // namespace tcat::io