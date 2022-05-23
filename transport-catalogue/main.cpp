#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

#include "testing.h"

#include <iostream>

using namespace std;

int main() {
    using namespace transport_catalogue;

    testing::TestAll();

    TransportCatalogue tc;
    io::InputBatch(cin, tc);
    io::StatBatch(cin, tc, cout);
    return 0;
}