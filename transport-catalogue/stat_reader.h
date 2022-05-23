#pragma once

#include "transport_catalogue.h"

#include <iostream>

namespace transport_catalogue::io {

void StatBatch(std::istream& input, TransportCatalogue& tc, std::ostream& out);

} // namespace transport_catalogue::io