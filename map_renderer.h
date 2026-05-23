#pragma once

#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"

#include <algorithm>
#include<cassert>
inline const double EPSILON = 1e-6;
bool IsZero(double value);

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

namespace map_renderer{
struct Parameters{
    double width = 0;
    double height = 0;
    double padding = 0;
    double line_width = 0;
    double stop_radius = 0;
    int bus_label_font_size = 0;
    svg::Point bus_label_offset;
    int stop_label_font_size = 0;
    svg::Point stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width = 0;
    std::vector<svg::Color> color_palette;
};

class MapRenderer{
public:
    MapRenderer(const Parameters& parameters);
    MapRenderer(Parameters&& parameters);

    void BuildMap(const TransportCatalogue& catalogue, const std::set<std::string_view>& bus_names, const std::set<std::string_view>& stop_names);
    void DrawMap (std::ostream& out) const;

private:
    Parameters parameters_;
    SphereProjector* sphere_projector_ = nullptr;
    svg::Document doc_;

    void SetSphereProjector(SphereProjector* sphere_projector);
    void AddStopToBusRoute(svg::Polyline &route, const geo::Coordinates &stop) const;
    void SetColorToBusRoute(svg::Polyline & route, const int index) const;
    void SetParamToBusRoute(svg::Polyline & route) const;
    void SetColorToBusName(svg::Text& bus_name, const int index) const;
    void AddObjectToDraw(std::unique_ptr<svg::Object>&& obj);
    void SetTextParametrs(svg::Text &bus_backing, svg::Text &bus_name, const std::string& name) const;
    void SetTextPosition(svg::Text& text, const geo::Coordinates& coordinate) const;
    void SetCircleParametrs(svg::Circle& circle, const geo::Coordinates& coordinate) const;
    void SetTextParametrsForStop(svg::Text &stop_backing, svg::Text &stop_name, const std::string& name, const geo::Coordinates& coordinate) const;
    void SetGeneralTextParametrs(svg::Text &text, const std::string& bus_name) const;
    void SetGeneralTextParametrsForStop(svg::Text &text, const std::string& stop_name, const geo::Coordinates& coordinate) const;
    void SetTextParametrsForBacking(svg::Text& text) const;
    void AddBusRoutes(const TransportCatalogue& catalogue, const std::set<std::string_view>& bus_names);
    void AddBusNames(const TransportCatalogue& catalogue, const std::set<std::string_view>& bus_names);
    void AddStops(const TransportCatalogue& catalogue, const std::set<std::string_view>& stop_names);
    void AddStopNames(const TransportCatalogue& catalogue, const std::set<std::string_view>& stop_names);
};
}
