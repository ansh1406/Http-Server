#include <iostream>
#include "includes/http.hpp"

int main()
{
    http::HttpServer server(51234);
    server.start();
}