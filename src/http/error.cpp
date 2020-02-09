#include "http/error.h"
#include "net/util/tm.h"
#include <sstream>
void http_error(shared_ptr<Connection> conn, int error_code) {
    ostringstream ss;
    string status;
    if (error_code == 404) {
        status = "404 Not Found";
    } else if (error_code == 400) {
        status = "400 Bad Request";
    } else if (error_code == 413) {
        status = "413 Payload Too Large";
    } else if (error_code == 505) {
        status = "505 Version Not Supported";
    } else {
        status = "500 Internal Server Error";
    }
    ss << "HTTP/1.1 " << status << "\r\n";
    ss << "Server: Httpd (Ubuntu)\r\n";
    ss << "Date: " << get_time_fmt_gmt() << "\r\n";
    ss << "Content-Type: text/html\r\n";
    ss << "Content-Length: " << status.size() << "\r\n";
    ss << "Connection: keep-alive\r\n";
    ss << "\r\n";
    ss << status;
    conn->send(ss.str());
}
