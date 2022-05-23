#pragma once

#include <istream>

#include "transport_catalogue.h"

namespace transport_catalogue::io {

void InputBatch(std::istream& input, TransportCatalogue& tc);

} // namespace transport_catalogue::io