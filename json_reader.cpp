#include "json_reader.h"

#include <iterator>
namespace json_reader {

static const std::string TYPE_G = "type";
static const std::string NAME_G = "name";

JSONReader::JSONReader(TransportCatalogue &catalogue): catalogue_(catalogue){}

json::Document JSONReader::ReadJsonRequests(std::istream &in)
{
    json::Document doc = json::Load(in);
    WriteInCatalogue(doc.GetRoot());
    ParseRendererParameters(doc.GetRoot());
    GetRouteSettingsFromNode(doc.GetRoot());
    return doc;
}

std::set<std::string_view> &JSONReader::GetBusNames()
{
    return bus_names_;
}

const std::set<std::string_view> &JSONReader::GetBusNames() const
{
    return bus_names_;
}

std::set<std::string_view> &JSONReader::GetStopNames()
{
    return stop_names_;
}

const std::set<std::string_view> &JSONReader::GetStopNames() const
{
    return stop_names_;
}

map_renderer::Parameters JSONReader::GetParameters()
{
    return map_parameters_;
}

const map_renderer::Parameters JSONReader::GetParameters() const
{
    return map_parameters_;
}

Setting JSONReader::GetRouteSettings()
{
    return settings_;
}

const Setting JSONReader::GetRouteSettings() const
{
    return settings_;
}


void JSONReader::WriteInCatalogue(const json::Node &node)
{
    static const std::string REQUEST_WRITE = "base_requests";

    assert(node.IsDict());
    auto map = node.AsDict();
    auto nodes = map[REQUEST_WRITE];
    assert(nodes.IsArray());
    AddStops(nodes.AsArray());
    AddBases(nodes.AsArray());
}

void JSONReader::AddStops(const json::Array &nodes)
{
    static const std::string TYPE_STOP = "Stop";
    static const std::string LAT = "latitude";
    static const std::string LNG = "longitude";
    static const std::string ROAD_DISTANCES = "road_distances";

    for (auto& val: nodes){
        auto val_map = val.AsDict();
        if (val_map[TYPE_G].AsString() != TYPE_STOP) {
            continue;
        }
        TransportStop stop;
        stop.name = val_map[NAME_G].AsString();
        stop.coordinates.lat = val_map[LAT].AsDouble();
        stop.coordinates.lng = val_map[LNG].AsDouble();
        catalogue_.AddStop(std::move(stop));
    }
    for (auto& val: nodes){
        auto val_map = val.AsDict();
        if (val_map[TYPE_G].AsString() != TYPE_STOP) {
            continue;
        }
        AddStopToStopLength(val_map[NAME_G].AsString(), val_map[ROAD_DISTANCES].AsDict());
    }
}

void JSONReader::AddBases(const json::Array &nodes)
{
    static const std::string TYPE_BUS = "Bus";
    static const std::string IS_ROUNDTRIP = "is_roundtrip";
    static const std::string BUS_STOPS = "stops";

    for (auto& val: nodes) {
        auto val_map = val.AsDict();
        if (val_map[TYPE_G].AsString() != TYPE_BUS) {
            continue;
        }
        TransportBus bus;
        bus.name = val_map[NAME_G].AsString();
        bus.is_roundtrip = val_map[IS_ROUNDTRIP].AsBool();
        auto& stop_array = val_map[BUS_STOPS].AsArray();
        for (size_t i = 0; i < stop_array.size(); ++i) {
            TransportStop* stop = catalogue_.GetStop(stop_array[i].AsString());
            assert(stop != nullptr);
            bus.stops.push_back(stop);
            stop_names_.insert(stop->name);
        }
        if (!bus.is_roundtrip) {
            bus.stops.insert(bus.stops.end(), std::next(bus.stops.rbegin()), bus.stops.rend());
        }
        catalogue_.AddBus(bus);
        if (!bus.stops.empty()) {
            auto bus_ptr = catalogue_.GetBus(bus.name);
            bus_names_.insert(bus_ptr->name);
        }
    }
}

void JSONReader::AddStopToStopLength(const std::string_view stop, const json::Dict &map)
{
    for (auto &val: map){
        catalogue_.AddStopToStopLength(catalogue_.GetStop(stop)->name,
                                      catalogue_.GetStop(val.first)->name,
                                      val.second.AsInt());
    }
}

void JSONReader::ParseRendererParameters(const json::Node &node)
{
    static const std::string RENDER_SETTINGS = "render_settings";
    static const std::string WIDTH = "width";
    static const std::string HEIGHT = "height";
    static const std::string PADDING = "padding";
    static const std::string LINE_WIDTH = "line_width";
    static const std::string STOP_RADIUS = "stop_radius";
    static const std::string BUS_LABEL_FONT_SIZE = "bus_label_font_size";
    static const std::string BUS_LABEL_OFFSET = "bus_label_offset";
    static const std::string STOP_LABEL_FONT_SIZE = "stop_label_font_size";
    static const std::string STOP_LABEL_OFFSET = "stop_label_offset";
    static const std::string UNDERLAYER_COLOR = "underlayer_color";
    static const std::string UNDERLAYER_WIDTH = "underlayer_width";
    static const std::string COLOR_PALETTE = "color_palette";

    assert(node.IsDict());
    auto map = node.AsDict();
    auto nodes = map[RENDER_SETTINGS];
    assert(nodes.IsDict());
    auto settings = nodes.AsDict();
    map_parameters_.width = settings[WIDTH].AsDouble();
    map_parameters_.height = settings[HEIGHT].AsDouble();
    map_parameters_.padding = settings[PADDING].AsDouble();
    map_parameters_.line_width = settings[LINE_WIDTH].AsDouble();
    map_parameters_.stop_radius = settings[STOP_RADIUS].AsDouble();
    map_parameters_.bus_label_font_size = settings[BUS_LABEL_FONT_SIZE].AsInt();
    map_parameters_.stop_label_font_size = settings[STOP_LABEL_FONT_SIZE].AsInt();
    map_parameters_.underlayer_width = settings[UNDERLAYER_WIDTH].AsDouble();
    auto label_offset = settings[BUS_LABEL_OFFSET].AsArray();
    assert(label_offset.size() == 2);
    map_parameters_.bus_label_offset = svg::Point{label_offset[0].AsDouble(), label_offset[1].AsDouble()};
    label_offset = settings[STOP_LABEL_OFFSET].AsArray();
    assert(label_offset.size() == 2);
    map_parameters_.stop_label_offset = svg::Point{label_offset[0].AsDouble(), label_offset[1].AsDouble()};
    map_parameters_.underlayer_color = GetColorFromNode(settings[UNDERLAYER_COLOR]);
    auto color_palette = settings[COLOR_PALETTE].AsArray();
    for (auto &val: color_palette) {
        map_parameters_.color_palette.push_back(GetColorFromNode(val));
    }
}

svg::Color JSONReader::GetColorFromNode(const json::Node &node)
{
    if(node.IsString()) {
        return node.AsString();
    }
    assert(node.IsArray());
    auto rgb = node.AsArray();
    if (rgb.size() == 3) {
        return svg::Rgb{static_cast<uint8_t>(rgb[0].AsInt()),
                        static_cast<uint8_t>(rgb[1].AsInt()),
                        static_cast<uint8_t>(rgb[2].AsInt())};
    }
    assert(rgb.size() == 4);
    return svg::Rgba{static_cast<uint8_t>(rgb[0].AsInt()),
                     static_cast<uint8_t>(rgb[1].AsInt()),
                     static_cast<uint8_t>(rgb[2].AsInt()),
                     rgb[3].AsDouble()};
}

void JSONReader::GetRouteSettingsFromNode(const json::Node &node)
{
    static std::string ROUTING_SETTINGS = "routing_settings";
    static std::string BUS_VELOCITY = "bus_velocity";
    static std::string BUS_WAIT_TIME = "bus_wait_time";

    auto settings = node.AsDict().at(ROUTING_SETTINGS);
    settings_.velocity = settings.AsDict().at(BUS_VELOCITY).AsDouble();
    settings_.wait_time = settings.AsDict().at(BUS_WAIT_TIME).AsDouble();
}

}//namespace json_reader


