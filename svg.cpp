#include "svg.h"

// Добавляет в svg-документ объект-наследник svg::Object
namespace svg {

using namespace std::literals;

std::ostream& operator << (std::ostream& out, const StrokeLineCap& val) {
    switch (val) {
    case StrokeLineCap::BUTT:
        out << "butt"sv;
        break;
    case StrokeLineCap::ROUND:
        out << "round"sv;
        break;
    case StrokeLineCap::SQUARE:
        out << "square"sv;
        break;
    default:
        break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& os, const StrokeLineJoin& join) {
    switch (join) {
    case StrokeLineJoin::ARCS:      os << "arcs"sv; break;
    case StrokeLineJoin::BEVEL:    os << "bevel"sv; break;
    case StrokeLineJoin::MITER:    os << "miter"sv; break;
    case StrokeLineJoin::MITER_CLIP: os << "miter-clip"sv; break;
    case StrokeLineJoin::ROUND:    os << "round"sv; break;
    default:
        break;
    }
    return os;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
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
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    RenderAttrs(out);
    out << "/>"sv;
}

//-----------Polyline----------------

Polyline &Polyline::AddPoint(Point point)
{
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext &context) const
{
    auto& out = context.out;
    out << "<polyline points=\""sv;
    if (points_.size() > 0){

        out << points_[0].x << ',' << points_[0].y;
        for (size_t i = 1; i < points_.size(); ++i) {
            out << ' ' << points_[i].x << ',' << points_[i].y;
        }
    }
    out << "\"";
    RenderAttrs(out);
    out << " />";
}

//------------Text-------------------

Text &Text::SetPosition(Point pos)
{
    pos_ = pos;
    return *this;
}

Text &Text::SetOffset(Point offset)
{
    offset_ = offset;
    return *this;
}

Text &Text::SetFontSize(uint32_t size)
{
    size_ = size;
    return *this;
}

Text &Text::SetFontFamily(std::string font_family)
{
    font_family_ = font_family;
    return *this;
}

Text &Text::SetFontWeight(std::string font_weight)
{
    font_weight_ = font_weight;
    return *this;
}

Text &Text::SetData(std::string data)
{
    auto lamb = [&](const std::string& val){
        for (auto ch: val) {
            data_.push_back(ch);
        }
    };
    for (auto ch: data) {
        switch (ch) {
        case '\"':
            lamb(QUOT_);
            break;
        case '\'':
            lamb(APOS_);
            break;
        case '<':
            lamb(LT_);
            break;
        case '>':
            lamb(GT_);
            break;
        case '&':
            lamb(AMP_);
            break;
        default:
            data_.push_back(ch);
            break;
        }
    }
    return *this;
}

void Text::RenderObject(const RenderContext &context) const
{
    auto& out = context.out;
    out << "<text x=\"" << pos_.x << "\" y=\"" << pos_.y;
    out << "\" dx=\"" << offset_.x << "\" dy=\"" << offset_.y;
    out << "\" font-size=\"" << size_ << "\"";
    if (font_family_ != "") {
        out << " font-family=\"" << font_family_ << "\"";
    }
    if (font_weight_ != "") {
        out << " font-weight=\"" << font_weight_ << "\"";
    }
    RenderAttrs(out);
    out << ">" << data_ << "</text>";
}





void Document::AddPtr(std::unique_ptr<Object> &&obj)
{
    data_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream &out) const
{
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    RenderContext ctx(out, 2, 2);
    for(auto &val: data_) {
        val->Render(ctx);
    }
    out << "</svg>"sv;
}

std::ostream& operator << (std::ostream& out, const Rgb& rgb){
    return out << "rgb("sv << static_cast<int>(rgb.red)
               << ',' << static_cast<int>(rgb.green)
               << ',' << static_cast<int>(rgb.blue) <<')';
}

std::ostream& operator << (std::ostream& out, const Rgba& rgba){
    return out << "rgba("sv << static_cast<int>(rgba.red)
               << ',' << static_cast<int>(rgba.green)
               << ',' << static_cast<int>(rgba.blue)
               <<',' << rgba.opacity <<')';
}

void ColorPrinter::operator()(std::monostate)
{
    out << std::get<std::string>(NoneColor);
}

void ColorPrinter::operator()(const std::string& color){
    out << color;
}

void ColorPrinter::operator()(const Rgb& color){
    out << color;
}

void ColorPrinter::operator()(const Rgba& color){
    out << color;
}

}  // namespace svg
