#include "http_client.hpp"

int main() {
  HttpConnection httpConnection("google.com", 80);
  httpConnection.openConnection();
  HttpRequest request(
      "GET", "/",
      Headers({{"host", "google.com"},
               {"user-agent", "Mozilla/5.0 (Windows NT 10.0; rv:91.0) "
                              "Gecko/20100101 Firefox/91.0"}}));
  auto response = httpConnection.sendRequest(request);

  std::cout << "STATUS LINE: \n";
  std::cout << "version: " << response.statusLine.version << std::endl;
  std::cout << "code: " << response.statusLine.code << std::endl;
  std::cout << "message: " << response.statusLine.message << std::endl;
  std::cout << std::endl;
  std::cout << "HEADERS: \n";
  for (Headers::const_iterator it = response.headers.begin();
       it != response.headers.end(); ++it) {
    std::cout << it->first << " : " << it->second << std::endl;
  }
  std::cout << std::endl;
  std::cout << "BODY: \n" << response.body << std::endl;

  // std::size_t pos = response.body.find("\r\n");
  // if (pos != std::string::npos)
  //   std::cout << "found at: " << pos << std::endl;
  // std::cout << response.body.substr(pos) << std::endl;
}