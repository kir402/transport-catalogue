#include "map_renderer.h"

#include <cassert>

namespace map_renderer {

static const std::string FONT_FAMILY_G = "Verdana";

MapRenderer::MapRenderer(const Parameters &parameters):parameters_(parameters){}

MapRenderer::MapRenderer(Parameters &&parameters):parameters_(std::move(parameters)){}

void MapRenderer::BuildMap(const TransportCatalogue &catalogue, const std::set<std::string_view> &bus_names, const std::set<std::string_view> &stop_names)
{
    std::vector<geo::Coordinates> coordinates;
    for (auto& val: bus_names){
        auto bus = catalogue.GetBus(val);
        for (auto& stop: bus->stops){
            coordinates.push_back(stop->coordinates);
        }
    }
    SphereProjector sphere_projector(coordinates.begin(),
                                     coordinates.end(),
                                     parameters_.width,
                                     parameters_.height,
                                     parameters_.padding);
    SetSphereProjector(&sphere_projector);

    AddBusRoutes(catalogue, bus_names);
    AddBusNames(catalogue, bus_names);
    AddStops(catalogue, stop_names);
    AddStopNames(catalogue, stop_names);

    SetSphereProjector(nullptr);
}

void MapRenderer::SetColorToBusRoute(svg::Polyline &route, const int index) const
{
    route.SetStrokeColor(parameters_.color_palette[index % parameters_.color_palette.size()]);
}

void MapRenderer::SetParamToBusRoute(svg::Polyline &route) const
{
    route.SetFillColor(svg::NoneColor)
        .SetStrokeWidth(parameters_.line_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

void MapRenderer::SetColorToBusName(svg::Text &bus_name, const int index) const
{
    bus_name.SetFillColor(parameters_.color_palette[index % parameters_.color_palette.size()]);
}

void MapRenderer::AddObjectToDraw(std::unique_ptr<svg::Object>&& obj)
{
    doc_.AddPtr((std::move(obj)));
}

void MapRenderer::DrawMap(std::ostream &out) const
{
    doc_.Render(out);
}

void MapRenderer::SetSphereProjector(SphereProjector *sphere_projector)
{
    sphere_projector_ = sphere_projector;
}


void MapRenderer::SetTextParametrs(svg::Text &bus_backing, svg::Text &bus_name, const std::string& name) const
{
    SetGeneralTextParametrs(bus_backing, name);
    SetGeneralTextParametrs(bus_name, name);
    SetTextParametrsForBacking(bus_backing);
}

void MapRenderer::SetTextPosition(svg::Text &text, const geo::Coordinates &coordinate) const
{
    assert(sphere_projector_ != nullptr);
    text.SetPosition(sphere_projector_->operator()(coordinate));
}

void MapRenderer::SetCircleParametrs(svg::Circle &circle, const geo::Coordinates &coordinate) const
{
    static const std::string FILL_COLOR = "white";

    assert(sphere_projector_ != nullptr);
    circle.SetCenter(sphere_projector_->operator()(coordinate));
    circle.SetRadius(parameters_.stop_radius);
    circle.SetFillColor(FILL_COLOR);
}

void MapRenderer::SetTextParametrsForStop(svg::Text &stop_backing, svg::Text &stop_name, const std::string &name, const geo::Coordinates &coordinate) const
{
    static const std::string FILL_COLOR = "black";

    SetGeneralTextParametrsForStop(stop_backing, name, coordinate);
    SetGeneralTextParametrsForStop(stop_name, name, coordinate);
    SetTextParametrsForBacking(stop_backing);
    stop_name.SetFillColor(FILL_COLOR);
}

void MapRenderer::SetGeneralTextParametrs(svg::Text &text, const std::string &bus_name) const
{
    static const std::string FONT_WEIGHT = "bold";

    text.SetOffset(parameters_.bus_label_offset)
    .SetFontSize(parameters_.bus_label_font_size)
    .SetFontFamily(FONT_FAMILY_G)
    .SetFontWeight(FONT_WEIGHT)
    .SetData(bus_name);
}

void MapRenderer::SetGeneralTextParametrsForStop(svg::Text &text, const std::string &stop_name, const geo::Coordinates& coordinate) const
{
    static const std::string FONT_WEIGHT = "";
    assert(sphere_projector_ != nullptr);
    text.SetData(stop_name)
    .SetPosition(sphere_projector_->operator()(coordinate))
    .SetOffset(parameters_.stop_label_offset)
    .SetFontSize(parameters_.stop_label_font_size)
    .SetFontFamily(FONT_FAMILY_G)
        .SetFontWeight(FONT_WEIGHT);
}

void MapRenderer::SetTextParametrsForBacking(svg::Text &text) const
{
    text.SetFillColor(parameters_.underlayer_color)
    .SetStrokeColor(parameters_.underlayer_color)
        .SetStrokeWidth(parameters_.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

void MapRenderer::AddBusRoutes(const TransportCatalogue &catalogue, const std::set<std::string_view> &bus_names)
{
    int count = 0;
    for (auto& val: bus_names){
        auto bus = catalogue.GetBus(val);
        svg::Polyline bus_route;
        assert(!bus->stops.empty());
        for (auto& stop: bus->stops){
            AddStopToBusRoute(bus_route, stop->coordinates);
        }
        SetColorToBusRoute(bus_route, count++);
        SetParamToBusRoute(bus_route);
        AddObjectToDraw(std::make_unique<svg::Polyline>(std::move(bus_route)));
    }
}

void MapRenderer::AddBusNames(const TransportCatalogue &catalogue, const std::set<std::string_view> &bus_names)
{
    int count = 0;
    for (auto& val: bus_names) {
        auto bus = catalogue.GetBus(val);
        assert(!bus->stops.empty());
        svg::Text bus_backing;
        svg::Text bus_name;
        SetTextParametrs(bus_backing, bus_name, bus->name);
        SetTextPosition(bus_backing, bus->stops[0]->coordinates);
        SetTextPosition(bus_name, bus->stops[0]->coordinates);
        SetColorToBusName(bus_name, count++);
        int index = bus->stops.size() / 2;
        if (bus->is_roundtrip || bus->stops[0] == bus->stops[index]) {
            AddObjectToDraw(std::make_unique<svg::Text>(std::move(bus_backing)));
            AddObjectToDraw(std::make_unique<svg::Text>(std::move(bus_name)));
        } else {
            svg::Text bus_backing_second = bus_backing;
            svg::Text bus_name_second = bus_name;
            SetTextPosition(bus_backing_second, bus->stops[index]->coordinates);
            SetTextPosition(bus_name_second, bus->stops[index]->coordinates);
            AddObjectToDraw(std::make_unique<svg::Text>(std::move(bus_backing)));
            AddObjectToDraw(std::make_unique<svg::Text>(std::move(bus_name)));
            AddObjectToDraw(std::make_unique<svg::Text>(std::move(bus_backing_second)));
            AddObjectToDraw(std::make_unique<svg::Text>(std::move(bus_name_second)));
        }
    }
}

void MapRenderer::AddStops(const TransportCatalogue &catalogue, const std::set<std::string_view> &stop_names)
{
    for (auto& val:stop_names){
        auto stop = catalogue.GetStop(val);
        assert(catalogue.GetBusesForStop(stop->name).value() != nullptr);
        svg::Circle stop_in_map;
        SetCircleParametrs(stop_in_map, stop->coordinates);
        AddObjectToDraw(std::make_unique<svg::Circle>(std::move(stop_in_map)));
    }
}

void MapRenderer::AddStopNames(const TransportCatalogue &catalogue, const std::set<std::string_view> &stop_names)
{
    for (auto& val:stop_names){
        auto stop = catalogue.GetStop(val);
        svg::Text stop_backing;
        svg::Text stop_name;
        SetTextParametrsForStop(stop_backing, stop_name, stop->name, stop->coordinates);
        AddObjectToDraw(std::make_unique<svg::Text>(std::move(stop_backing)));
        AddObjectToDraw(std::make_unique<svg::Text>(std::move(stop_name)));
    }
}

void MapRenderer::AddStopToBusRoute(svg::Polyline &route, const geo::Coordinates &stop) const
{
    assert(sphere_projector_ != nullptr);
    route.AddPoint(sphere_projector_->operator()(stop));
}

}//namespace map_renderer

bool IsZero(double value)
{
    return std::abs(value) < EPSILON;
}
