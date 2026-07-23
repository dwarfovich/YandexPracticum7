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

constexpr std::string_view delimiter = "\r\n\r\n";

awaitable<void> session(tcp::socket client_socket, io_context &io_context) {
    try {
        boost::asio::streambuf buffer;
        static const std::string hostHeader = "Host";
        std::string hostHeaderValue;
        std::string request;
        while (true) {
            co_await boost::asio::async_read_until(client_socket, buffer, '\n', use_awaitable);

            std::istream is(&buffer);
            std::string line;
            std::getline(is, line);

             if (line == "\r" || line.empty()){
                break;
             }

            request += line + "\n";

            //std::println(std::cout, "Received: {}, length = {}", line, line.size());
            iterHeaders(line, [&](const auto &header, const auto &value) {
                if (header == hostHeader) {
                    hostHeaderValue = value;
                }
            });
        }

        request += "\r\n";

        std::println(std::cout, "Got request: {}", request);
        for (char c : request) {
            if (c == '\r')
                std::print(std::cout, "\\r");
            else if (c == '\n')
                std::print(std::cout, "\\n");
            else
                std::print(std::cout, "{}", c);
        }

        std::println(std::cout);
        hostHeaderValue.pop_back();
        std::println(std::cout, "Found host header: {}", hostHeaderValue);
        std::println(std::cout, "Header size: {}", hostHeaderValue.size());
        tcp::resolver resolver(io_context.get_executor());
        tcp::socket socket(io_context.get_executor());
        static const std::string defaultPort = "80";

        try {
            const auto endpoints = co_await resolver.async_resolve(hostHeaderValue, "80", use_awaitable);

            std::println(std::cout, "Resolve OK");
        } catch (const std::exception &e) {
            std::println(std::cout, "Resolve failed: {}", e.what());
        }

        const auto endpoints = co_await resolver.async_resolve(hostHeaderValue, defaultPort, use_awaitable);

        const auto connectedEndpoint = co_await boost::asio::async_connect(socket, endpoints, use_awaitable);
        std::println(std::cout, "Connected: {}", connectedEndpoint.address().to_string());
        std::println(std::cout, "Request: {}", request);
        co_await boost::asio::async_write(socket, boost::asio::buffer(request), boost::asio::use_awaitable);

        boost::asio::streambuf responseBuffer;

        boost::system::error_code ec;

        std::println(std::cout, "Reading response");

        co_await async_read_until(socket, responseBuffer, "\r\n\r\n", use_awaitable);

        std::istream is(&responseBuffer);

        std::string line;
        std::size_t contentLength = 0;

        while (std::getline(is, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (line.empty())
                break;

            iterHeaders(line, [&](auto const &header, auto const &value) {
                if (header == "Content-Length") {
                    std::from_chars(value.data(), value.data() + value.size(), contentLength);
                }
            });
        }

        // здесь responseBuffer уже содержит только тело ответа

        if (responseBuffer.size() < contentLength) {
            co_await async_read(socket, responseBuffer,
                                boost::asio::transfer_exactly(contentLength - responseBuffer.size()), use_awaitable);
        } else if (contentLength == 0){
            for (;;) {
                std::size_t n = co_await socket.async_read_some(
                    responseBuffer.prepare(4096), boost::asio::redirect_error(boost::asio::use_awaitable, ec));

                if (ec == boost::asio::error::eof)
                    break;

                if (ec)
                    throw boost::system::system_error(ec);

                responseBuffer.commit(n);
            }
        }

        std::string response(boost::asio::buffers_begin(responseBuffer.data()),
                             boost::asio::buffers_end(responseBuffer.data()));
        co_await async_write(client_socket, boost::asio::buffer(response), use_awaitable);
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
        Server server(io_service, std::atoi(argv[1]));
        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

        signals.async_wait([&](const boost::system::error_code &, int) { server.stop(); });

        io_service.run();
        std::cout << "Run\n";

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
