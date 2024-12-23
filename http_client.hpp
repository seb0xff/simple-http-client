#ifndef HTTP_CLIENT_TEST_V2
#define HTTP_CLIENT_TEST_V2

#include <iostream>
#include <map>
#include <netdb.h>
#include <string>

typedef std::pair<std::string, std::string> Header;
typedef std::map<std::string, std::string> Headers;

class StatusLine {
public:
  StatusLine(std::string version, uint32_t code, std::string message)
      : version(version), code(code), message(message) {}
  const std::string version;
  const uint32_t code;
  const std::string message;
};

class HttpResponse {
public:
  HttpResponse(StatusLine statusLine, Headers headers, std::string body)
      : statusLine(statusLine), headers(headers), body(body) {}
  const StatusLine statusLine;
  const Headers headers;
  const std::string body;
};

class HttpRequest {
public:
  HttpRequest(std::string method, std::string path, Headers headers,
              std::string body = "", std::string httpVersion = "HTTP/1.1")
      : method(method), path(path), httpVersion(httpVersion), headers(headers),
        body(body) {
    requestString = method + ' ' + path + ' ' + httpVersion + "\r\n";
    for (Headers::const_iterator it = headers.cbegin(); it != headers.cend();
         ++it) {
      requestString += it->first + ": " + it->second + "\r\n";
    }
    requestString += "\r\n";
    requestString += body;
  }

  const std::string method;
  const std::string path;
  const std::string httpVersion;
  const Headers headers;
  const std::string body;
  std::string string() { return requestString; }

private:
  /// String representation of the Request, ready to send
  std::string requestString;
};

class HttpConnection {
public:
  HttpConnection(const std::string hostAddress, const uint16_t hostPort);
  void openConnection();
  void closeConnection();
  HttpResponse sendRequest(const HttpRequest request);
  ~HttpConnection();

private:
  bool connected = false;
  addrinfo *hostInfo;
  void *socketAddress;
  int32_t socketFileDescriptor;

  std::tuple<StatusLine, std::size_t> getStatusLine(std::string &buffer);
  std::tuple<Headers, std::size_t> getHeaders(std::string &buffer,
                                              std::size_t startPosition = 0);
};

#endif