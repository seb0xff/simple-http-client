#include <arpa/inet.h>
#include <cstdlib>  // exit()
#include <cstring>  // std::memset()
#include <iostream> // std::cout, std::cerr, std::endl
#include <map>      // std::map
#include <netdb.h> // getaddinfo(), freeaddrinfo(), gai_strerror(), struct addrinfo, gethostbyname()
#include <regex>   // std::regex, std::smatch, std::regex_search()
#include <string> // std::string, std::to_string(), std::string::find(), std::string::substr()
// #include <sys/socket.h> // socket(), connetct()
#include "http_client.hpp"
#include <linux/if_ether.h>
#include <tuple>
#include <unistd.h> // write(), read(), close()

HttpConnection::HttpConnection(const std::string hostAddress,
                               const uint16_t hostPort) {

  addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;

  if (int infoStatus =
          getaddrinfo(hostAddress.data(), std::to_string(hostPort).data(),
                      &hints, &hostInfo) != 0) {
    std::cerr << gai_strerror(infoStatus) << std::endl;
  }

  if (hostInfo->ai_family == AF_INET) {
    socketAddress = (sockaddr_in *)(hostInfo->ai_addr);
  } else if (hostInfo->ai_family == AF_INET6) {
    socketAddress = (sockaddr_in6 *)(hostInfo->ai_addr);
  }
  // std::string buf;
  // buf.resize(hostInfo->ai_addrlen);
  // if (inet_ntop(hostInfo->ai_family, &(((sockaddr_in
  // *)socketAddress)->sin_addr), &buf[0], hostInfo->ai_addrlen) <= 0)
  // {
  //   std::cerr << "ERROR could not convert\n";
  // }
  // else
  // {
  //   std::cout << buf << std::endl;
  // }

  socketFileDescriptor =
      socket(hostInfo->ai_family, hostInfo->ai_socktype, hostInfo->ai_protocol);
  if (socketFileDescriptor < 0) {
    std::cerr << "ERROR could not create socket\n";
    std::exit(EXIT_FAILURE);
  }
}

void HttpConnection::openConnection() {
  if (connected) {
    std::cerr << "WARNING already connected\n";
  } else {
    if (connect(socketFileDescriptor, (sockaddr *)socketAddress,
                hostInfo->ai_addrlen) < 0) {
      std::cerr << "ERROR could not connect\n";
      std::exit(EXIT_FAILURE);
    } else {
      connected = true;
    }
  }
}

void HttpConnection::closeConnection() {
  if (!connected) {
    std::cerr << "WARNING already closed\n";
  } else {
    close(socketFileDescriptor);
    connected = false;
  }
}

HttpResponse HttpConnection::sendRequest(HttpRequest request) {

  // Send request

  std::string requestString = request.string();
  uint64_t sentBytesTotal = 0;
  int64_t sentBytes;

  if (!connected) {
    openConnection();
  }
  do {
    sentBytes =
        send(socketFileDescriptor, requestString.data() + sentBytesTotal,
             requestString.length() - sentBytesTotal, 0);
    std::cout << "sent: " << sentBytes << std::endl;
    if (sentBytes > 0) {
      sentBytesTotal += sentBytes;
    } else if (sentBytes < 0) {
      std::cerr << "ERROR writing to socket\n";
      std::exit(EXIT_FAILURE);
    } else {
      break;
    }
  } while (sentBytesTotal <= requestString.length());

  // Receive response

  uint64_t receivedBytesTotal = 0;
  int64_t receivedBytes;
  std::string buffer;
  buffer.resize(1024);

  receivedBytes = recv(socketFileDescriptor, &buffer[0] + receivedBytesTotal,
                       buffer.length() - receivedBytesTotal, 0);
  std::cout << "received: " << receivedBytes << std::endl << std::endl;
  if (receivedBytes > 0) {
    receivedBytesTotal += receivedBytes;
  } else if (receivedBytes < 0) {
    std::cerr << "ERROR reading from socket\n";
    std::exit(EXIT_FAILURE);
  }

  std::tuple<StatusLine, std::size_t> statusLineAndHeadersPosition =
      getStatusLine(buffer);
  StatusLine statusLine = std::get<0>(statusLineAndHeadersPosition);
  std::size_t headersBeginPosition = std::get<1>(statusLineAndHeadersPosition);
  std::tuple<Headers, std::size_t> headersAndBodyPosition =
      getHeaders(buffer, headersBeginPosition);
  Headers headers = std::get<0>(headersAndBodyPosition);
  std::size_t bodyBeginPosition = std::get<1>(headersAndBodyPosition);
  std::string body =
      buffer.substr(bodyBeginPosition, receivedBytesTotal - bodyBeginPosition);

  // trim right
  std::size_t newLinePosition;
  if ((newLinePosition = body.rfind("\r\n", receivedBytesTotal)) !=
      std::string::npos) {
    body = body.erase(newLinePosition);
  }

  // std::cout << "version: " << statusLine.version << std::endl;
  // std::cout << "code: " << statusLine.code << std::endl;
  // std::cout << "message: " << statusLine.message << std::endl;
  // std::cout << std::endl;
  // for (Headers::iterator it = headers.begin(); it != headers.end(); ++it)
  // {
  //   std::cout << it->first << " : " << it->second << std::endl;
  // }
  // std::cout << buffer.substr(0) << std::endl;
  // std::cout << std::endl;
  // std::cout << buffer << std::endl;
  return HttpResponse(statusLine, headers, body);
}
HttpConnection::~HttpConnection() { freeaddrinfo(hostInfo); }

std::tuple<StatusLine, std::size_t>
HttpConnection::getStatusLine(std::string &buffer) {
  std::string firstLine;
  std::size_t position = buffer.find("\r\n");
  if (position == std::string::npos) {
    std::cerr << "ERROR could not find \\r\\n\n";
  }
  firstLine = buffer.substr(0, position);
  std::regex pattern(R"(([ -~]+)\s([0-9]+)\s(.*))");
  std::smatch match;
  if (std::regex_search(firstLine, match, pattern)) {
    return std::tuple<StatusLine, std::size_t>(
        StatusLine(match.str(1), std::stoul(match.str(2), nullptr, 10),
                   match.str(3)),
        position + 2);
  } else {
    std::cerr << "ERROR invalid status line\n";
    return std::tuple<StatusLine, std::size_t>(StatusLine("", 0, ""),
                                               position + 2);
  }
}
std::tuple<Headers, std::size_t>
HttpConnection::getHeaders(std::string &buffer, std::size_t startPosition) {
  if (!startPosition) {
    std::size_t position = buffer.find("\r\n");
    if (position == std::string::npos) {
      std::cerr << "ERROR could not find \\r\\n\n";
      return std::tuple<Headers, std::size_t>(Headers(), position);
    }
    startPosition = position + 2;
  }
  Headers headers;
  std::size_t delimeterPosition = startPosition;
  std::size_t lastDelimeterPosition = startPosition;
  std::string substring;
  do {
    lastDelimeterPosition = delimeterPosition;
    delimeterPosition = buffer.find("\r\n", lastDelimeterPosition + 2);
    // std::cout << "lastDelimeterPosition: " << lastDelimeterPosition <<
    // std::endl; std::cout << "delimeterPosition: " << delimeterPosition <<
    // std::endl;
    if (delimeterPosition == std::string::npos) {
      // std::cout << "NOT FOUND\n";
      break;
    }
    substring = buffer.substr(lastDelimeterPosition,
                              delimeterPosition - lastDelimeterPosition);
    if (substring.length() == 2) {
      // std::cout << "END OF HEADERS\n";
      break;
    }
    std::regex pattern(R"(([ -~]+):\s+(.*))");
    std::smatch match;
    if (std::regex_search(substring, match, pattern)) {
      headers.insert(Header(match.str(1), match.str(2)));
      // headers[match.str(1)] = match.str(2);
    }
  } while (1);
  return std::tuple<Headers, std::size_t>(headers, delimeterPosition + 2);
}
