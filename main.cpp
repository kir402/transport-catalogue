#include <iostream>

#include "json_reader.h"
#include "request_handler.h"

void work(std::istream& in, std::ostream& out, TransportCatalogue& catalogue){

    json_reader::JSONReader reader(catalogue);
    auto doc = reader.ReadJsonRequests(in);
    request_handler::RequestHandler request(catalogue, reader);
    request.ProcessRequests(doc.GetRoot(), out);
}

int main()
{
    TransportCatalogue catalogue;
    work(std::cin, std::cout, catalogue);
}
