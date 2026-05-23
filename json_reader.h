#pragma once
#include <iostream>

#include"transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"


namespace json_reader {

struct PairHashForVertexMap {
    size_t operator()(const std::pair<std::string, std::string_view>& p) const {
        size_t h1 = std::hash<std::string>{}(p.first);
        size_t h2 = std::hash<std::string_view>{}(p.second);
        return h1 + (h2 * 37);
    }
};

struct Setting{
    double wait_time = 0;
    double velocity = 0;
};

class JSONReader{
public:
    JSONReader(TransportCatalogue& catalogue);

    json::Document ReadJsonRequests(std::istream &in);
    std::set<std::string_view>& GetBusNames();
    const std::set<std::string_view>& GetBusNames() const;
    std::set<std::string_view>& GetStopNames();
    const std::set<std::string_view>& GetStopNames() const;
    map_renderer::Parameters GetParameters();
    const map_renderer::Parameters GetParameters() const;
    Setting GetRouteSettings();
    const Setting GetRouteSettings() const;

private:
    TransportCatalogue& catalogue_;
    std::set<std::string_view> bus_names_;
    std::set<std::string_view> stop_names_;
    map_renderer::Parameters map_parameters_;
    Setting settings_;

    void WriteInCatalogue(const json::Node& node);
    void AddStops(const json::Array& nodes);
    void AddBases(const json::Array& nodes);
    void AddStopToStopLength(const std::string_view stop, const json::Dict& map);
    void ParseRendererParameters(const json::Node &node);
    svg::Color GetColorFromNode(const json::Node& node);
    void GetRouteSettingsFromNode(const json::Node& node);
};

}
