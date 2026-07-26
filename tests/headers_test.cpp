#include "headers.h"
#include <gtest/gtest.h>

TEST(iterHeaders, Empty) {
    std::string str;
    auto l = [](const auto &, const auto &) { ASSERT_FALSE(true); };
    iterHeaders(str, l);
}

TEST(iterHeaders, Spaces) {
    std::string str = "   ";
    auto l = [](const auto &, const auto &) { ASSERT_FALSE(true); };
    iterHeaders(str, l);
}

TEST(iterHeaders, Spaces2) {
    std::string str = "   \t  \t";
    auto l = [](const auto &, const auto &) { ASSERT_FALSE(true); };
    iterHeaders(str, l);
}

TEST(iterHeaders, Spaces3) {
    std::string str = "\r\n";
    auto l = [](const auto &, const auto &) { ASSERT_FALSE(true); };
    iterHeaders(str, l);
}

TEST(iterHeaders, WithoutRequestLine) {
    std::string str = "header: 1";
    std::vector<std::pair<std::string, std::string>> expected{{"header", "1"}};
    int counter = 0;
    auto l = [&expected, &counter](const auto &header, const auto &value) {
        ASSERT_EQ(counter, 0);
        ++counter;
        ASSERT_EQ(expected.front().first, header);
        ASSERT_EQ(expected.front().second, value);
    };
    iterHeaders(str, l);
    ASSERT_EQ(counter, 1);
}

TEST(iterHeaders, WithoutRequestLineWithLineEnding) {
    std::string str = "header: 1\r\n";
    std::vector<std::pair<std::string, std::string>> expected{{"header", "1"}};
    int counter = 0;
    auto l = [&expected, &counter](const auto &header, const auto &value) {
        ASSERT_EQ(counter, 0);
        ++counter;
        ASSERT_EQ(expected.front().first, header);
        ASSERT_EQ(expected.front().second, value);
    };
    iterHeaders(str, l);
    ASSERT_EQ(counter, 1);
}

TEST(iterHeaders, WithoutRequestLine2) {
    std::string str = "header: 1\r\nheader2: 2";
    std::vector<std::pair<std::string, std::string>> expected{{"header", "1"}, {"header2", "2"}};
    int counter = 0;

    auto l = [&expected, &counter](const auto &header, const auto &value) {
        ASSERT_TRUE(counter < expected.size());
        ASSERT_EQ(expected[counter].first, header);
        ASSERT_EQ(expected[counter].second, value);
        ++counter;
    };
    iterHeaders(str, l);
    ASSERT_EQ(counter, 2);
}

TEST(iterHeaders, SingleHeader) {
    std::string str = "GET /index.html HTTP/1.1\r\nheader: 1";
    std::vector<std::pair<std::string, std::string>> expected{{"header", "1"}};
    int counter = 0;
    auto l = [&expected, &counter](const auto &header, const auto &value) {
        ASSERT_EQ(counter, 0);
        ++counter;
        ASSERT_EQ(expected.front().first, header);
        ASSERT_EQ(expected.front().second, value);
    };
    iterHeaders(str, l);
    ASSERT_EQ(counter, 1);
}

TEST(iterHeaders, MultipleHeaders) {
    std::string str = "GET /index.html HTTP/1.1\r\nheader: 1\r\nheader2: 2\r\nheader3: 3";
    std::vector<std::pair<std::string, std::string>> expected{{"header", "1"}, {"header2", "2"}, {"header3", "3"}};
    int counter = 0;
    auto l = [&expected, &counter](const auto &header, const auto &value) {
        ASSERT_EQ(expected[counter].first, header);
        ASSERT_EQ(expected[counter].second, value);
        ++counter;
    };
    iterHeaders(str, l);
    ASSERT_EQ(counter, 3);
}

TEST(iterHeaders, MultipleSameHeaders) {
    std::string str = "GET /index.html HTTP/1.1\r\nheader: 1\r\nheader: 1\r\nheader: 1";
    std::vector<std::pair<std::string, std::string>> expected{{"header", "1"}, {"header", "1"}, {"header", "1"}};
    int counter = 0;
    auto l = [&expected, &counter](const auto &header, const auto &value) {
        ASSERT_EQ(expected[counter].first, header);
        ASSERT_EQ(expected[counter].second, value);
        ++counter;
    };
    iterHeaders(str, l);
    ASSERT_EQ(counter, 3);
}

TEST(findHostPort, Simple) {
    std::string str = "Hello\r\nHost: ya.ru:1234";
    const auto [host, port] = findHostPort(str);
    ASSERT_EQ(host, "ya.ru");
    ASSERT_EQ(port, "1234");
}

TEST(findHostPort, NoHost) {
    std::string str = "Hello\r\nHeader: ya.ru:1234";
    const auto [host, port] = findHostPort(str);
    ASSERT_EQ(host, "");
    ASSERT_EQ(port, "");
}

TEST(findContentLength, Simple) {
    std::string str = "Hello\r\nContent-Length: 123";
    const auto length = findContentLength(str);
    ASSERT_TRUE(length.has_value());
    ASSERT_EQ(length.value(), 123);
}

TEST(findContentLength, NoContentLength) {
    std::string str = "Hello\r\nHeader: 123";
    const auto length = findContentLength(str);
    ASSERT_FALSE(length.has_value());
}
