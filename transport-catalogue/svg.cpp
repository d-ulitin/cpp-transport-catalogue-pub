#include "svg.h"

#include <string_view>
#include <cmath>


namespace svg {

using namespace std;

// ------------ Object ------------

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

string_view GetStrokeLineCapText(StrokeLineCap stroke_line_cap) {
    using namespace std::literals;

    switch (stroke_line_cap) {
    case StrokeLineCap::BUTT:
        return "butt"sv;
    case StrokeLineCap::ROUND:
        return "round"sv;
    case StrokeLineCap::SQUARE:
        return "square"sv;
    default:
        throw logic_error("unknown StrokeLineCap");
    }
}

string_view GetStrokeLineJoinText(StrokeLineJoin stroke_line_join) {
    using namespace std::literals;

    switch (stroke_line_join) {
    case StrokeLineJoin::ARCS:
        return "arcs"sv;
    case StrokeLineJoin::BEVEL:
        return "bevel"sv;
    case StrokeLineJoin::MITER:
        return "miter"sv;
    case StrokeLineJoin::MITER_CLIP:
        return "miter-clip"sv;
    case StrokeLineJoin::ROUND:
        return "round"sv;
    default:
        throw logic_error("unknown StrokeLineJoin");
    }
}

ostream& operator<<(ostream& out, StrokeLineCap stroke_line_cap) {
    out << GetStrokeLineCapText(stroke_line_cap);
    return out;
}

ostream& operator<<(ostream& out, StrokeLineJoin stroke_line_join) {
    out << GetStrokeLineJoinText(stroke_line_join);
    return out;
}


// -------------- RenderColor ------------

void RenderColor::operator()(std::monostate) const {
    out_ << "none"sv;
}

void RenderColor::operator()(std::string c) const {
    out_ << c;
}

void RenderColor::operator()(Rgb c) const {
    out_ << "rgb("sv << +c.red <<',' << +c.green << ',' << +c.blue  << ')';
}

void RenderColor::operator()(Rgba c) const {
    out_ << "rgba("sv << +c.red <<',' << +c.green << ',' << +c.blue << ',' << c.opacity  << ')';
}

ostream& operator<<(ostream& out, const Color& color) {
    std::visit(RenderColor(out), color);
    return out;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    // example: <circle cx="20" cy="20" r="10" />
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    PathProps::RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(move(point));
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    // example: <polyline points="20,40 22.9389,45.9549 29.5106,46.9098 24.7553,51.5451 25.8779,58.0902 20,55 14.1221,58.0902 15.2447,51.5451 10.4894,46.9098 17.0611,45.9549 20,40" />
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool first = true;
    for (auto point : points_) {
        if (!first)
            out << ' ';
        out << point.x << ',' << point.y;
        first = false;
    }
    out << "\""sv;
    PathProps::RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Text ------------------

// Задаёт координаты опорной точки (атрибуты x и y)
Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

// Задаёт размеры шрифта (атрибут font-size)
Text& Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

// Задаёт название шрифта (атрибут font-family)
Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = move(font_family);
    return *this;
}

// Задаёт толщину шрифта (атрибут font-weight)
Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = move(font_weight);
    return *this;
}

// Задаёт текстовое содержимое объекта (отображается внутри тега text)
Text& Text::SetData(std::string data) {
    data_ = move(data);
    return *this;
}

std::string_view Text::Escape(char c) const {
    switch (c) {
    case '"':
        return "&quot;"sv;
    case '\'':
        return "&apos;"sv;
    case '<':
        return "&lt;"sv;
    case '>':
        return "&gt;"sv;
    case '&':
        return "&amp;"sv;
    default:
        return {};
    }
}

void Text::RenderObject(const RenderContext& context) const {
    // example: <text x="35" y="20" dx="0" dy="6" font-size="12" font-family="Verdana" font-weight="bold">Hello C++</text>

    auto& out = context.out;
    out << "<text";
    PathProps::RenderAttrs(out);
    out << " x=\"" << position_.x << "\"";
    out << " y=\"" << position_.y << "\"";
    out << " dx=\"" << offset_.x << "\"";
    out << " dy=\"" << offset_.y << "\"";
    out << " font-size=\"" << size_ << "\"";
    if (font_family_)
        out << " font-family=\"" << *font_family_ << "\"";
    if (font_weight_)
        out << " font-weight=\"" << *font_weight_ << "\"";
    out << '>';
    for (char c : data_) {
        auto escaped = Escape(c);
        if (escaped.size() > 0)
            out << escaped;
        else
            out << c;
    }
    out << "</text>";
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(move(obj));
}

void Document::Render(std::ostream& out) const {
    out << R"""(<?xml version="1.0" encoding="UTF-8" ?>)""" << '\n';
    out << R"""(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)""" << '\n';
    RenderContext rc(out, 2);
    RenderContext rc1 = rc.Indented();
    RenderObjects(rc1);
    out << "</svg>";
}

void Document::RenderObjects(const RenderContext& context) const {
    for (const auto& obj : objects_) {
        obj->Render(context);
    }
}

} // namespace svg