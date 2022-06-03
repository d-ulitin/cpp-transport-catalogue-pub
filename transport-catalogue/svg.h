#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace svg {

using std::literals::operator "" sv;
using std::literals::operator "" s;


struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

class Object {
public:
    void Render(const RenderContext& context) const;
    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

struct Rgb {
    uint8_t red = 0u, green = 0u, blue = 0u;
    Rgb() = default;
    Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}
};

struct Rgba {
    uint8_t red = 0u, green = 0u, blue = 0u;
    double opacity = 1.0;

    Rgba() = default;
    Rgba(uint8_t r, uint8_t g, uint8_t b, double op)
        : red(r), green(g), blue(b), opacity(op) {}
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

inline const Color NoneColor{"none"s};

struct RenderColor {
    RenderColor(std::ostream& out) : out_(out) {}

    void operator()(std::monostate) const;
    void operator()(std::string c) const;
    void operator()(Rgb c) const;
    void operator()(Rgba c) const;

private:
    std::ostream& out_;
};

std::ostream& operator<<(std::ostream& out, const Color& color);

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& out, StrokeLineCap stroke_line_cap);

std::ostream& operator<<(std::ostream& out, StrokeLineJoin stroke_line_join);

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap stroke_line_cap) {
        stroke_line_cap_ = stroke_line_cap;
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin stroke_line_join) {
        stroke_line_join_ = stroke_line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (!std::holds_alternative<std::monostate>(fill_color_)) {
            out << " fill=\""sv;;
            std::visit(RenderColor(out), fill_color_);
            out << "\""sv;
        }
        if (!std::holds_alternative<std::monostate>(stroke_color_)) {
            out << " stroke=\""sv;
            std::visit(RenderColor(out), stroke_color_);
            out << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\"" << *stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv;
            out << *stroke_line_cap_;
            out << "\""sv;
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv;
            out << *stroke_line_join_;
            out << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner — наследник PathProps
        return static_cast<Owner&>(*this);
    }

    Color fill_color_ = {};
    Color stroke_color_ = {};
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_ = {0.0, 0.0};
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;

    std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

private:
    std::string_view Escape(char c) const;
    void RenderObject(const RenderContext& context) const override;

    Point position_ = {0, 0};
    Point offset_ = {0, 0};
    uint32_t size_ = 1;
    std::optional<std::string> font_family_;
    std::optional<std::string> font_weight_;
    std::string data_;
};

struct ObjectContainer;

struct Drawable {
    virtual void Draw(ObjectContainer& container) const = 0;

    virtual ~Drawable() = default;
};

struct ObjectContainer {

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

    template <typename T>
    void Add(T& object) {
        AddPtr(std::make_unique<T>(object));
    }

    template <typename T>
    void Add(T&& object) {
        AddPtr(std::make_unique<T>(std::move(object)));
    }

    virtual ~ObjectContainer() = default;
};

class Document : public ObjectContainer {
public:
    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;

private:
    void RenderObjects(const RenderContext& context) const;

    std::vector<std::unique_ptr<Object>> objects_;
};

} // namespace svg