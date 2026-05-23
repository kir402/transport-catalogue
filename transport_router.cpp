#include "transport_router.h"

#include<numeric>

namespace transport_router {

RouteWeight RouteWeight::operator +(const RouteWeight &other) const
{
    RouteWeight sum;
    sum.weigth = weigth + other.weigth;
    return sum;
}
auto RouteWeight::operator <=>(const RouteWeight &other) const
{
    return weigth <=> other.weigth;
}

TransportRouter::TransportRouter(const TransportCatalogue& catalogue, const RouteSettings settings):
    catalogue_(catalogue),
    WAIT_TYME_(settings.wait_time),
    VELOCITY_(settings.velocity)
{

    auto buses_and_stops = catalogue_.GetActiveBusesAndStops();
    auto& stop_names = buses_and_stops.second;
    graph_ = std::make_unique<GraphType>(stop_names.size() * 2);
    for(auto stop_name: stop_names) {
        stop_name_id_[stop_name] = stop_name_id_.size() * 2;
    }

    graph::Edge<RouteWeight> edge;

    //Создание рёбер ожидания

    //Контанта времени ожидания, переведённого в метры
    const double WAIT_TIME_IN_METERS = WAIT_TYME_ * VELOCITY_ * TO_METERS_PER_MINUTE_;
    int id = 0;
    for (auto stop: stop_names) {
        edge.from = id;
        edge.to = id + 1;
        edge.weight.weigth = WAIT_TIME_IN_METERS;
        edge.weight.type = RouteType::route_wait;
        edge.weight.name = stop;
        graph_->AddEdge(edge);
        id += 2;
    }

    //создание рёбер маршрутов
    auto& bus_names = buses_and_stops.first;
    for (auto& val: bus_names) {
        auto bus = catalogue_.GetBus(val);
        auto& stop_array = bus->stops;
        if (stop_array.empty()) {
            continue;
        }
        bool is_roundtrip = bus->is_roundtrip;

        auto distances_between_stops = GetDistancesBetweenStops(stop_array);

        if (is_roundtrip) {
            AddEdgeForRoundtripBus(stop_array, distances_between_stops, bus->name);
        } else {
            AddEdgeForNonRoundtripBus(stop_array, distances_between_stops, bus->name);
        }
    }
    router_ = std::make_unique<graph::Router<RouteWeight>>(*graph_);
}

const std::optional<RouteInfo> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const
{
    auto it_from = stop_name_id_.find(from);
    auto it_to = stop_name_id_.find(to);
    if (it_from == stop_name_id_.end() || it_to == stop_name_id_.end()) {
        return std::nullopt;
    }
    auto route = router_->BuildRoute(it_from->second, it_to->second);
    if (!route.has_value()) {
        return std::nullopt;
    }
    const double BUS_VELOCITY = VELOCITY_ * TO_METERS_PER_MINUTE_;

    RouteInfo res;
    res.total_time = route.value().weight.weigth / BUS_VELOCITY;
    res.route.reserve(route.value().edges.size());
    for(auto id: route.value().edges) {
        res.route.push_back(graph_->GetEdge(id).weight);
        res.route.back().weigth /= BUS_VELOCITY;
    }
    return res;
}


std::vector<double> TransportRouter::GetDistancesBetweenStops(const std::vector<TransportStop *> &stops) const
{
    std::vector<double> res(stops.size() - 1);
    for (size_t i = 0; i < stops.size() - 1; ++i){
        res[i] = catalogue_.GetStopToStopLength(stops[i]->name, stops[i + 1]->name);
    }
    return res;
}

void TransportRouter::AddEdge(const std::vector<TransportStop *>& stop_array,
                              const std::vector<double>& distances,
                              std::string_view name,
                              size_t index_from,
                              size_t index_to)
{
    graph::Edge<RouteWeight> edge;
    edge.from = stop_name_id_.at(stop_array[index_from]->name) + 1;
    edge.to = stop_name_id_.at(stop_array[index_to]->name);
    edge.weight.weigth = std::accumulate(distances.begin() + index_from, distances.begin() + index_to, 0.0);
    edge.weight.name = catalogue_.GetBus(name)->name;
    edge.weight.span_count = index_to - index_from;
    edge.weight.type = RouteType::route_bus;
    graph_->AddEdge(edge);
}

void TransportRouter::AddEdgeForRoundtripBus(const std::vector<TransportStop *> &stop_array, const std::vector<double> &distances, std::string_view name)
{
    for (size_t from = 0; from < stop_array.size() - 1; ++from) {
        size_t end_to = from == 0 ? stop_array.size() - 1: stop_array.size();
        for (size_t to = from + 1; to < end_to; ++to) {
            AddEdge(stop_array, distances, name, from, to);
        }
    }
}

void TransportRouter::AddEdgeForNonRoundtripBus(const std::vector<TransportStop *> &stop_array, const std::vector<double> &distances, std::string_view name)
{
    size_t index = stop_array.size() / 2;
    for (size_t from = 0; from < index; ++from) {
        for (size_t to = from + 1; to <= index; ++to) {
            AddEdge(stop_array, distances, name, from, to);
        }
    }
    for (size_t from = index; from < stop_array.size() - 1; ++from){
        for (size_t to = from + 1; to < stop_array.size(); ++to) {
            AddEdge(stop_array, distances, name, from, to);
        }
    }
}

}//namespace transport_router
