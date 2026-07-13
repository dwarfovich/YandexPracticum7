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
    auto l = [&expected, &counter](const auto & header, const auto & value) { 
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

//TEST(iterHeaders, SkipRequestLine) {
//    std::string str = "header: 1";
//    std::vector<std::pair<std::string, std::string>> expected{{"header", "1"}};
//    int counter = 0;
//    auto l = [&expected, &counter](const auto &header, const auto &value) {
//        ASSERT_EQ(counter, 0);
//        ++counter;
//        ASSERT_EQ(expected.front().first, header);
//        ASSERT_EQ(expected.front().second, value);
//    };
//    iterHeaders(str, l);
//    ASSERT_EQ(counter, 1);
//}

TEST(iterHeaders, SingleHeader) {
    // code here
}

TEST(iterHeaders, MultipleHeaders) {
    // code here
}

TEST(iterHeaders, MultipleSameHeaders) {
    // code here
}

TEST(findHostPort, Simple) {
    // code here
}

TEST(findHostPort, NoHost) {
    // code here
}

TEST(findContentLength, Simple) {
    // code here
}

TEST(findContentLength, NoContentLength) {
    // code here
}
