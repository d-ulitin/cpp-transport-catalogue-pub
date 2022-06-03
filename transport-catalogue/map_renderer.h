#pragma once

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

#include "transport_catalogue.h"
#include "geo.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>


namespace tcat::io
{

using std::literals::operator "" s;
using std::literals::operator "" sv;

class SphereProjector;

struct MapRendererSettings {
    // ширина и высота изображения в пикселях. Вещественное число в диапазоне от 0 до 100000.
    double width = 1200.0;
    double height = 1200.0;

    // отступ краёв карты от границ SVG-документа. Вещественное число не меньше 0 и меньше
    // min(width, height)/2
    double padding = 50.0;

    // толщина линий, которыми рисуются автобусные маршруты. Вещественное число в диапазоне
    // от 0 до 100000.
    double line_width = 14.0;

    // радиус окружностей, которыми обозначаются остановки. Вещественное число в диапазоне
    // от 0 до 100000
    double stop_radius = 5.0;

    // размер текста, которым написаны названия автобусных маршрутов. Целое число в диапазоне
    // от 0 до 100000
    int bus_label_font_size = 20;

    // смещение надписи с названием маршрута относительно координат конечной остановки на карте.
    // Массив из двух элементов типа double. Задаёт значения свойств dx и dy SVG-элемента <text>.
    // Элементы массива — числа в диапазоне от –100000 до 100000
    double bus_label_offset[2] = {7.0, 15.0};

    // размер текста, которым отображаются названия остановок. Целое число в диапазоне
    // от 0 до 100000
    int stop_label_font_size = 20;

    // смещение названия остановки относительно её координат на карте. Массив из двух элементов
    // типа double. Задаёт значения свойств dx и dy SVG-элемента <text>. Числа в диапазоне
    // от –100000 до 100000
    double stop_label_offset[2] = {7.0, -3.0};

    // цвет подложки под названиями остановок и маршрутов. Формат хранения цвета будет ниже
    svg::Color underlayer_color = svg::Rgba({255, 255, 255, 0.85});

    // толщина подложки под названиями остановок и маршрутов. Задаёт значение атрибута stroke-width
    // элемента <text>. Вещественное число в диапазоне от 0 до 100000
    double underlayer_width = 3.0;

    // цветовая палитра. Непустой массив
    std::vector<svg::Color> color_palette = {
        "green"s,
        svg::Rgb{255, 160, 0},
        "red"s
    };
};

class MapRenderer final {
public:
    MapRenderer(db::TransportCatalogue& tc, const MapRendererSettings& settings);

    svg::Document Render();

private:
    template <typename BackInsertIter>
    void RenderBusLines(const domain::Bus* bus, const SphereProjector& projector,
    svg::Color color, BackInsertIter it);

    template <typename BackInsertIter>
    void RenderBusName(const domain::Bus* bus, const SphereProjector& projector,
    svg::Color color, BackInsertIter it);

    template <typename BackInsertIter>
    void RenderBusNameLabel(const domain::Bus* bus, svg::Point position,
    svg::Color color, BackInsertIter it);

    template <typename BackInsertIter>
    void RenderStopSymbol(const domain::Stop * stop, const SphereProjector& projector,
    BackInsertIter it);

    template <typename BackInsertIter>
    void RenderStopName(const domain::Stop * stop, const SphereProjector& projector,
    BackInsertIter it);

    db::TransportCatalogue& tc_;
    MapRendererSettings settings_;
};
    
} // namespace tcat::io
