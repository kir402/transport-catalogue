#pragma once
#include"json_reader.h"
#include"transport_router.h"
#include "json_builder.h"

namespace request_handler {

class RequestHandler{
public:
    RequestHandler(TransportCatalogue& catalogue,
                   const json_reader::JSONReader& reader);

    void ProcessRequests(const json::Node &node, std::ostream& out);

private:
    const TransportCatalogue& catalogue_;
    map_renderer::MapRenderer map_renderer_;
    const std::set<std::string_view>& bus_names_;
    const std::set<std::string_view>& stop_names_;
    transport_router::TransportRouter router_;

    json::Node RouteNode(const json::Dict& node) const;
    void AddStopNode(const json::Dict& node, json::Builder& rezult) const;
    void AddBusNode(const json::Dict& node, json::Builder& rezult) const;
    void AddMapNode(const json::Dict& node, json::Builder& rezult);
};


}
