#pragma once

#include <iostream>
#include <memory>

#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.h"

namespace tcat::io::serialization {

struct Settings {
    std::string file;
};

struct Base {
    db::TransportCatalogue& transport_catalogue;
    io::MapRendererSettings& render_settings;
    db::RoutingSettings& routing_settings;
    std::unique_ptr<db::TransportRouter>& transport_router;
};

bool Serialize(const Base& base, std::ostream& output);

bool Deserialize(std::istream& input, Base& base);

} // tcat::io::serialization
