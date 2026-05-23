#include "request_handler.h"

#include <sstream>

namespace request_handler {
static const std::string NAME_G = "name";
static const std::string REQ_ID_G = "request_id";
static const std::string ID_G = "id";
static const std::string ERROR_KEY_G = "error_message";

transport_router::RouteSettings ToRouteSettings(const json_reader::Setting& settings) {
    transport_router::RouteSettings res;
    res.wait_time = settings.wait_time;
    res.velocity = settings.velocity;
    return res;
}

json::Node BusNode(const json::Dict& node, const TransportCatalogue &catalogue){
    static const std::string CURVATURE = "curvature";
    static const std::string ROUTE_LENGTH = "route_length";
    static const std::string STOP_COUNT = "stop_count";
    static const std::string UNIQ_STOP_COUNT = "unique_stop_count";

    auto info = catalogue.GetBusInfo(node.at(NAME_G).AsString());
    assert(info.has_value());
    assert(info.value().curvature_denominator > 0);

    return json::Builder{}.StartDict()
            .Key(REQ_ID_G).Value(node.at(ID_G).AsInt())
            .Key(CURVATURE).Value(info.value().route_length / info.value().curvature_denominator)
            .Key(ROUTE_LENGTH).Value(info.value().route_length)
            .Key(STOP_COUNT).Value(info.value().stops_on_route)
            .Key(UNIQ_STOP_COUNT).Value(info.value().unique_stops)
            .EndDict().Build();
}

json::Node ErrorNode(const json::Dict& node){
    static const std::string ERROR_KEY = "error_message";
    static const std::string ERROR_VAL = "not found";

    return json::Builder{}.StartDict()
            .Key(REQ_ID_G).Value(node.at(ID_G).AsInt())
            .Key(ERROR_KEY).Value(ERROR_VAL)
            .EndDict().Build();
}

json::Node ErrorStopNode(const json::Dict& node){
    return ErrorNode(node);
}

json::Node ErrorBusNode(const json::Dict& node){
    return ErrorNode(node);
}

json::Node ErrorNoBuses (const json::Dict& node)
{
    static const std::string ERROR_VAL = "no buses";
    static const std::string ERROR_KEY = "error_message";

    return json::Builder{}.StartDict()
            .Key(REQ_ID_G).Value(node.at(ID_G).AsInt())
            .Key(ERROR_KEY).Value(ERROR_VAL)
            .EndDict().Build();
}

json::Node StopNode(const json::Dict& node, const TransportCatalogue &catalogue){
    static const std::string BUSES = "buses";
    auto info = catalogue.GetBusesForStop(node.at(NAME_G).AsString());
    assert(info.has_value());
    json::Builder stop_node;
    stop_node.StartDict()
        .Key(REQ_ID_G).Value(node.at(ID_G).AsInt())
        .Key(BUSES).StartArray();

    if (info.value() != nullptr){
        for (auto &val: *info.value()){
            stop_node.Value(std::string(val));
        }
    }
    stop_node.EndArray().EndDict();

    return stop_node.Build();
}

json::Node ErrorRoute(const json::Dict& node){
    static const std::string ERROR_VAL = "not found";

    return json::Builder{}.StartDict()
        .Key(REQ_ID_G).Value(node.at(ID_G).AsInt())
        .Key(ERROR_KEY_G).Value(ERROR_VAL)
        .EndDict().Build();
}

void Print(const json::Node &node, std::ostream &out)
{
    json::Print(json::Document(node), out);
}

void RequestHandler::ProcessRequests(const json::Node &node, std::ostream& out){
    static const std::string REQUEST_READ = "stat_requests";
    static const std::string TYPE = "type";
    static const std::string TYPE_STOP = "Stop";
    static const std::string TYPE_BUS = "Bus";
    static const std::string TYPE_MAP = "Map";
    static const std::string TYPE_ROUTE = "Route";
    static const std::string TYPE_MAP_FOR_ANSWER = "map";

    assert(node.IsDict());
    auto map = node.AsDict();
    auto nodes = map[REQUEST_READ];
    assert(nodes.IsArray());
    json::Builder res;
    res.StartArray();
    for (auto& val: nodes.AsArray()) {
        auto val_map = val.AsDict();
        if(val_map[TYPE].AsString() == TYPE_STOP) {
            AddStopNode(val_map, res);
        } else if (val_map[TYPE].AsString() == TYPE_BUS) {
            AddBusNode(val_map, res);
        } else if (val_map[TYPE].AsString() == TYPE_MAP) {
            AddMapNode(val_map, res);
        } else if (val_map[TYPE].AsString() == TYPE_ROUTE){
            res.Value(RouteNode(val_map));
        } else {
            assert(false);
        }
    }
    Print(res.EndArray().Build(), out);
}



RequestHandler::RequestHandler(TransportCatalogue& catalogue, const json_reader::JSONReader& reader):
    catalogue_(catalogue),
    map_renderer_(reader.GetParameters()),
    bus_names_(reader.GetBusNames()),
    stop_names_(reader.GetStopNames()),
    router_(catalogue_,ToRouteSettings(reader.GetRouteSettings()))
{}
//{reader.GetRouteSettings().wait_time, reader.GetRouteSettings().velocity}
json::Node RequestHandler::RouteNode(const json::Dict &node) const
{
    static const std::string FROM = "from";
    static const std::string TO = "to";
    static const std::string TOTAL_TIME = "total_time";
    static const std::string ITEMS = "items";
    static const std::string TYPE = "type";
    static const std::string TYPE_WAIT = "Wait";
    static const std::string TYPE_BUS = "Bus";
    static const std::string TIME = "time";
    static const std::string BUS = "bus";
    static const std::string SPAN_COUNT = "span_count";
    static const std::string STOP_NAME = "stop_name";

    auto route = router_.BuildRoute(node.at(FROM).AsString(), node.at(TO).AsString());
    if (!route.has_value()) {
        return ErrorRoute(node);
    }

    json::Builder route_node;
    route_node.StartDict()
        .Key(REQ_ID_G).Value(node.at(ID_G).AsInt())
        .Key(TOTAL_TIME).Value(route.value().total_time)
        .Key(ITEMS).StartArray();

    for(auto info: route.value().route) {
        route_node.StartDict()
            .Key(TIME).Value(info.weigth);
        if (info.type == transport_router::RouteType::route_wait) {
            route_node
            .Key(STOP_NAME).Value(std::string(info.name))
            .Key(TYPE).Value(TYPE_WAIT);
        } else {
            route_node
            .Key(BUS).Value(std::string(info.name))
            .Key(SPAN_COUNT).Value(info.span_count)
            .Key(TYPE).Value(TYPE_BUS);
        }
        route_node.EndDict();
    }
    route_node.EndArray().EndDict();
    return route_node.Build();
}

void RequestHandler::AddStopNode(const json::Dict& node, json::Builder& rezult) const
{
    auto stop = catalogue_.GetStop(node.at(NAME_G).AsString());
    if (stop == nullptr) {
        rezult.Value(ErrorStopNode(node));
    } else {
        rezult.Value(StopNode(node, catalogue_));
    }
}

void RequestHandler::AddBusNode(const json::Dict &node, json::Builder &rezult) const
{
    auto bus = catalogue_.GetBus(node.at(NAME_G).AsString());
    if (bus == nullptr) {
        rezult.Value(ErrorBusNode(node));
    } else {
        rezult.Value(BusNode(node, catalogue_));
    }
}

void RequestHandler::AddMapNode(const json::Dict &node, json::Builder &rezult)
{
    static const std::string TYPE_MAP_FOR_ANSWER = "map";

    std::ostringstream map_in_str;
    map_renderer_.BuildMap(catalogue_, bus_names_, stop_names_);
    map_renderer_.DrawMap(map_in_str);
    rezult.StartDict();
    rezult.Key(REQ_ID_G).Value(node.at(ID_G).AsInt())
        .Key(TYPE_MAP_FOR_ANSWER).Value(map_in_str.str())
        .EndDict();
}



}//namespace request_handler
