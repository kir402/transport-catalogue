#include "transport_catalogue.h"


#include <utility>
#include <unordered_set>

void TransportCatalogue::AddStop(const TransportStop &stop)
{
    stops_.push_back(stop);
    stopname_to_stop_.insert({stops_.back().name, &stops_.back()});
}

void TransportCatalogue::AddBus(const TransportBus &bus)
{
    buses_.push_back(bus);
    busname_to_bus_.insert({buses_.back().name, &buses_.back()});
    for(auto& stop: buses_.back().stops) {
        stops_on_bus_[stop->name].insert(buses_.back().name);
    }
}


TransportBus *TransportCatalogue::GetBus(std::string_view name)
{
    auto it = busname_to_bus_.find(name);
    return it == busname_to_bus_.end() ? nullptr: it->second;
}

const TransportBus *TransportCatalogue::GetBus(std::string_view name) const
{
    auto it = busname_to_bus_.find(name);
    return it == busname_to_bus_.end() ? nullptr: it->second;

}

TransportStop *TransportCatalogue::GetStop(std::string_view name)
{
    auto it = stopname_to_stop_.find(name);
    return it == stopname_to_stop_.end() ? nullptr: it->second;
}

const TransportStop *TransportCatalogue::GetStop(std::string_view name) const
{
    auto it = stopname_to_stop_.find(name);
    return it == stopname_to_stop_.end() ? nullptr: it->second;
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const
{
    auto dist_calc = [](const TransportStop* first, const TransportStop* last){
        return geo::ComputeDistance(first->coordinates, last->coordinates);
    };

    auto bus = GetBus(name);
    if (bus == nullptr) {
        return std::nullopt;
    }
    assert(bus->stops.size() > 1);
    BusInfo res;
    std::unordered_set<TransportStop*> set;
    res.name = bus->name;
    res.stops_on_route = bus->stops.size();
    set.insert(bus->stops[0]);
    for (size_t i = 1; i < bus->stops.size(); ++i) {
        set.insert(bus->stops[i]);
        res.curvature_denominator += dist_calc(bus->stops[i-1], bus->stops[i]);
        res.route_length += GetStopToStopLength(bus->stops[i-1]->name, bus->stops[i]->name);
    }
    res.unique_stops = set.size();
    return res;
}

std::optional<const std::set<std::string_view>*> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const
{
    if(GetStop(stop_name) == nullptr) {
        return std::nullopt;
    }
    auto it = stops_on_bus_.find(stop_name);
    if (it == stops_on_bus_.end()) {
            return nullptr;
    }
    return &it->second;
}

void TransportCatalogue::AddStopToStopLength(std::string_view stop1, std::string_view stop2, double length)
{
    stop_to_stop_length_[{stop1, stop2}] = length;
}


double TransportCatalogue::GetStopToStopLength(std::string_view stop1, std::string_view stop2) const
{
    auto it = stop_to_stop_length_.find({stop1, stop2});
    if (it != stop_to_stop_length_.end()) {
        return it->second;
    }
    it = stop_to_stop_length_.find({stop2, stop1});
    assert(it != stop_to_stop_length_.end());
    return it->second;
}

const std::pair<std::unordered_set<std::string_view>, std::unordered_set<std::string_view> > TransportCatalogue::GetActiveBusesAndStops() const
{
    std::unordered_set<std::string_view> active_buses;
    std::unordered_set<std::string_view> active_stops;
    for (auto val: busname_to_bus_) {
        if(!val.second->stops.empty()) {
            active_buses.insert(val.first);
            for (auto stop: val.second->stops) {
                active_stops.insert(stop->name);
            }
        }
    }
    return {active_buses, active_stops};
}
