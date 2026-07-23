#include "headers.h"

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <iostream>
#include <string_view>

using boost::asio::async_read_until;
using boost::asio::awaitable;
using boost::asio::buffer;
using boost::asio::co_spawn;
using boost::asio::dynamic_buffer;
using boost::asio::io_context;
using boost::asio::transfer_at_least;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;
using boost::system::error_code;

constexpr std::string_view httpHeadersDelimiter = "\r\n\r\n";

awaitable<std::pair<std::string, std::string>> readAnswer(tcp::socket &socket) {
    boost::asio::streambuf buffer;
    co_await async_read_until(socket, buffer, httpHeadersDelimiter, use_awaitable);
    std::istream responseStream(&buffer);

    std::string headers;
    std::string line;

    while (std::getline(responseStream, line)) {
        headers += line;
        headers += "\r\n";

        if (line == "\r") {
            break;
        }
    }

    const auto contentLength = findContentLength(headers).value_or(0);
    if (buffer.size() < contentLength) {
        co_await async_read(socket, buffer, boost::asio::transfer_exactly(contentLength - buffer.size()),
                            use_awaitable);
    } else if (contentLength == 0) {
        boost::system::error_code errorCode;
        while (true) {
            static constexpr std::size_t bufferSize = 4096;
            std::size_t n = co_await socket.async_read_some(
                buffer.prepare(bufferSize), boost::asio::redirect_error(boost::asio::use_awaitable, errorCode));

            if (errorCode == boost::asio::error::eof) {
                break;
            }

            if (errorCode) {
                throw boost::system::system_error(errorCode);
            }

            buffer.commit(n);
        }
    }

    std::string body{boost::asio::buffers_begin(buffer.data()), boost::asio::buffers_end(buffer.data())};

    co_return std::make_pair(std::move(headers), std::move(body));
}

awaitable<void> session(tcp::socket client_socket, io_context &io_context) {
    try {
        boost::asio::streambuf buffer;
        co_await boost::asio::async_read_until(client_socket, buffer, httpHeadersDelimiter, use_awaitable);
        std::string request{boost::asio::buffers_begin(buffer.data()), boost::asio::buffers_end(buffer.data())};
        std::println(std::cout, "Got request: {}", request);

        auto [host, port] = findHostPort(request);
        if (port.empty()) {
            static const std::string defaultPort = "80";
            port = defaultPort;
        }

        tcp::resolver resolver{io_context.get_executor()};
        tcp::socket targetSocket{io_context.get_executor()};
        const auto endpoints = co_await resolver.async_resolve(host, port, use_awaitable);

        const auto connectedEndpoint = co_await boost::asio::async_connect(targetSocket, endpoints, use_awaitable);
        co_await boost::asio::async_write(targetSocket, boost::asio::buffer(request), boost::asio::use_awaitable);
        buffer.consume(buffer.size());

        const auto [headers, body] = co_await readAnswer(targetSocket);
        co_await async_write(client_socket, boost::asio::buffer(body), use_awaitable);
    } catch (const std::exception &e) {
        std::println(std::cout, "Exception: {}", e.what());
    }

    co_return;
}

class Server {
public:
    Server(io_context &io_context, short port)
        : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        co_spawn(io_context_, do_accept(), boost::asio::detached);
    }

    void stop() {
        boost::system::error_code ec;
        acceptor_.close(ec);
    }

private:
    awaitable<void> do_accept() {
        try {
            while (true) {
                tcp::socket socket = co_await acceptor_.async_accept(use_awaitable);
                co_spawn(io_context_, session(std::move(socket), io_context_), boost::asio::detached);
            }
        } catch (const boost::system::system_error &e) {
            if (e.code() != boost::asio::error::operation_aborted) {
                throw;
            }
        }
    }

private:
    io_context &io_context_;
    tcp::acceptor acceptor_;
};

int main(int argc, char *argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: proxy_server";
            std::cerr << " <listen_port>\n";
            return 1;
        }
        io_context io_service(1);
        const size_t firstArgLength = std::strlen(argv[1]);
        std::size_t port;
        const auto [ptr, ec] = std::from_chars(argv[1], argv[1] + firstArgLength, port);
        if (ec != std::errc{} || ptr != argv[1] + firstArgLength) {
            std::cerr << "Error parsing port";
            return -1;
        }

        std::print(std::cout, "Start listening at port {}...\n", port);
        Server server(io_service, std::atoi(argv[1]));
        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

        signals.async_wait([&](const boost::system::error_code &, int) { server.stop(); });
        io_service.run();
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
