#pragma once
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include<unordered_set>
#include <optional>
#include <iostream>
#include <set>
#include <utility>
#include <cassert>

#include "geo.h"

struct TransportStop
{
    std::string name = "";
    geo::Coordinates coordinates;
};

struct TransportBus{
    std::string name = "";
    std::vector<TransportStop*> stops;
    bool is_roundtrip = false;
};

struct BusInfo
{
    std::string name = "";
    int stops_on_route = 0;
    int unique_stops = 0;
    double route_length = 0;
    double curvature_denominator = 0;

    friend std::ostream& operator << (std::ostream& out, const BusInfo& bus_info){
        assert(bus_info.curvature_denominator > 0);
        out << "Bus " << bus_info.name;
        out << ": " << bus_info.stops_on_route << " stops on route, ";
        out << bus_info.unique_stops << " unique stops, ";
        out << bus_info.route_length << " route length, ";
        out << bus_info.route_length / bus_info.curvature_denominator << " curvature";
        return out;
    }

};

struct PairHash {
    size_t operator()(const std::pair<std::string_view, std::string_view>& p) const {
        size_t h1 = std::hash<std::string_view>{}(p.first);
        size_t h2 = std::hash<std::string_view>{}(p.second);
        return h1 + (h2 * 37);
    }
};

class TransportCatalogue{
public:
    TransportCatalogue() = default;
    void AddStop(const TransportStop & stop);
    void AddBus (const TransportBus& bus);
    TransportBus* GetBus(std::string_view name);
    const TransportBus* GetBus(std::string_view name) const;
    TransportStop* GetStop (std::string_view name);
    const TransportStop* GetStop (std::string_view name) const;
    std::optional<BusInfo> GetBusInfo(std::string_view name) const;
    std::optional<const std::set<std::string_view>*> GetBusesForStop(std::string_view stop_name) const;
    void AddStopToStopLength(std::string_view stop1, std::string_view stop2, double length);
    double GetStopToStopLength(std::string_view stop1, std::string_view stop2) const;
    const std::pair<std::unordered_set<std::string_view>, std::unordered_set<std::string_view>> GetActiveBusesAndStops() const;
private:
    std::deque<TransportStop> stops_;
    std::deque<TransportBus> buses_;
    std::unordered_map<std::string_view, TransportStop*> stopname_to_stop_;
    std::unordered_map<std::string_view, TransportBus*> busname_to_bus_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stops_on_bus_;
    std::unordered_map<std::pair<std::string_view,std::string_view>, double, PairHash> stop_to_stop_length_;
};
