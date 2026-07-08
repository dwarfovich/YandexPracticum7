#include "headers.h"

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/signal_set.hpp>

#include <csignal>

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

awaitable<void> session(tcp::socket client_socket, io_context &) {
    try {
        boost::asio::streambuf buffer;

        for (;;) {
            co_await boost::asio::async_read_until(client_socket, buffer, '\n', use_awaitable);

            std::istream is(&buffer);

            std::string line;
            std::getline(is, line);

            std::println(std::cout, "Received: {}", line);

            line += delimiter;

            co_await boost::asio::async_write(client_socket, boost::asio::buffer(line), use_awaitable);
        }
    } catch (...) {
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
            for (;;) {
                tcp::socket socket = co_await acceptor_.async_accept(use_awaitable);

                co_spawn(io_context_, session(std::move(socket), io_context_), boost::asio::detached);
            }
        } catch (const boost::system::system_error &e) {
            if (e.code() != boost::asio::error::operation_aborted)
                throw;
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
        io_context io_service (1);
        Server server (io_service, std::atoi(argv[1]));
        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);

        signals.async_wait([&](const boost::system::error_code &, int) {
            server.stop();
        });

        io_service.run();
        std::cout << "Run\n";

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
