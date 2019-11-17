#include <iostream>

#define ASIO_STANDALONE
#include "asio.hpp"

std::string fizzbuzz(const int n) {
  auto d3 = n % 3;
  auto d5 = n % 5;

  if (d3 == 0 && d5 == 0) {
    return "fizzbuzz";
  } else if (d3 == 0) {
    return "fizz";
  } else if (d5 == 0) {
    return "buzz";
  } else {
    return std::to_string(n);
  }
}

class session : public std::enable_shared_from_this<session> {
 public:
  explicit session(asio::ip::tcp::socket socket)
      : tcp_socket(std::move(socket)) {}

  void start() { read(); }

 private:
  void read() {
    auto self(shared_from_this());
    tcp_socket.async_read_some(
        asio::buffer(data, data.size()),
        [this, self](std::error_code const ec, std::size_t const length) {
          if (!ec) {
            auto number = std::string(data.data(), length);
            auto result = fizzbuzz(std::atoi(number.c_str()));
            std::cout << number << " -> " << result << std::endl;
            write(result);
          }
        });
  }

  void write(std::string_view response) {
    auto self(shared_from_this());
    tcp_socket.async_write_some(
        asio::buffer(response.data(), response.length()),
        [this, self](std::error_code const ec, std::size_t const) {
          if (!ec) {
            read();
          }
        });
  }

  std::array<char, 1024> data;
  asio::ip::tcp::socket tcp_socket;
};

class server {
 public:
  server(asio::io_context& context, short const port)
      : tcp_acceptor(context,
                     asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
        tcp_socket(context) {
    std::cout << "server running on port " << port << std::endl;
    accept();
  }

 private:
  void accept() {
    tcp_acceptor.async_accept(tcp_socket, [this](std::error_code ec) {
      if (!ec) {
        std::make_shared<session>(std::move(tcp_socket))->start();
      }
      accept();
    });
  }

  asio::ip::tcp::acceptor tcp_acceptor;
  asio::ip::tcp::socket tcp_socket;
};

void run_server(short const port) {
  try {
    asio::io_context context;
    server srv(context, port);
    context.run();
  } catch (std::exception const& e) {
    std::cerr << "exception: " << e.what() << std::endl;
  }
}

void run_client(std::string const& host, short const port) {
  try {
    asio::io_context context;
    asio::ip::tcp::socket tcp_socket(context);
    asio::ip::tcp::resolver resolver(context);
    asio::connect(tcp_socket,
                  resolver.resolve({host.c_str(), std::to_string(port)}));

    while (true) {
      std::cout << "number [1-99]: ";

      int number;
      std::cin >> number;
      if (std::cin.fail() || number < 1 || number > 99) {
        break;
      }

      auto request = std::to_string(number);
      tcp_socket.write_some(asio::buffer(request, request.length()));

      std::array<char, 1024> reply;
      auto reply_length =
          tcp_socket.read_some(asio::buffer(reply, reply.size()));

      std::cout << "reply is ";
      std::cout.write(reply.data(), reply_length);
      std::cout << std::endl;
    }
  } catch (std::exception const& e) {
    std::cerr << "exception: " << e.what() << std::endl;
  }
}

int main() {
  run_client("localhost", 11234);

  return 0;
}
