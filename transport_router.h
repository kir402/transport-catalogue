#pragma once
#include "router.h"
#include"transport_catalogue.h"

#include <memory>

namespace transport_router {

struct RouteSettings{
    double wait_time = 0;
    double velocity = 0;
};

enum class RouteType{
    route_wait,
    route_bus,
};

struct RouteWeight {
    double weigth = 0;
    std::string_view name;
    int span_count = 0;
    RouteType type;

    RouteWeight operator + (const RouteWeight& other) const;
    auto operator <=>(const RouteWeight& other) const;
};

struct RouteInfo{
    double total_time = 0;
    std::vector<RouteWeight> route;
};

class TransportRouter {
public:
    TransportRouter(const TransportCatalogue& catalogue, const RouteSettings settings);
    const std::optional<RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

private:
    using GraphType = graph::DirectedWeightedGraph<RouteWeight>;
    const TransportCatalogue& catalogue_;
    std::unique_ptr<GraphType> graph_;
    std::unique_ptr<graph::Router<RouteWeight>> router_;
    std::unordered_map<std::string_view, graph::VertexId> stop_name_id_;
    const double WAIT_TYME_;
    const double VELOCITY_;
    static constexpr double TO_METERS_PER_MINUTE_ = 100.0 / 6.0;

    std::vector<double> GetDistancesBetweenStops (const std::vector<TransportStop*> &stops) const;
    void AddEdge(const std::vector<TransportStop *>& stop_array,
                 const std::vector<double>& distances,
                 std::string_view name,
                 size_t index_from,
                 size_t index_to);
    void AddEdgeForRoundtripBus(const std::vector<TransportStop *>& stop_array,
                                const std::vector<double>& distances,
                                std::string_view name);
    void AddEdgeForNonRoundtripBus(const std::vector<TransportStop *>& stop_array,
                                   const std::vector<double>& distances,
                                   std::string_view name);
};

}//namespace transport_router
