
// ------------------------------------------------

#include "gtest/gtest.h"

// ------------------------------------------------

#include "basic_json.hpp"

// ------------------------------------------------

namespace kaixo::test {

    // ------------------------------------------------
    
    static_assert(std::same_as<basic_json::type_alias<bool>::type, basic_json::boolean_t>);
    static_assert(std::same_as<basic_json::type_alias<std::nullptr_t>::type, basic_json::null_t>);
    static_assert(std::same_as<basic_json::type_alias<std::int8_t>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<std::int16_t>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<std::int32_t>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<std::int64_t>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<std::uint8_t>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<std::uint16_t>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<std::uint32_t>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<std::uint64_t>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<float>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<double>::type, basic_json::number_t>);
    static_assert(std::same_as<basic_json::type_alias<std::string>::type, basic_json::string_t>);
    static_assert(std::same_as<basic_json::type_alias<std::string_view>::type, basic_json::string_t>);
    static_assert(std::same_as<basic_json::type_alias<char[2]>::type, basic_json::string_t>);
    static_assert(std::same_as<basic_json::type_alias<char(&)[2]>::type, basic_json::string_t>);
    static_assert(std::same_as<basic_json::type_alias<const char[2]>::type, basic_json::string_t>);
    static_assert(std::same_as<basic_json::type_alias<const char(&)[2]>::type, basic_json::string_t>);

    // ------------------------------------------------

    TEST(BasicJsonTests, Construction) {

        std::nullptr_t _nullv = nullptr;
        basic_json _null1{ nullptr };
        basic_json _null2{ _nullv };
        basic_json _null3{ std::move(_nullv) };

        bool _boolv = true;
        basic_json _bool1{ true };
        basic_json _bool2{ false };
        basic_json _bool3{ _boolv };
        basic_json _bool4{ std::move(_boolv) };

        ASSERT_TRUE(_bool1.is<bool>());
        ASSERT_TRUE(_bool2.is<bool>());
        ASSERT_TRUE(_bool3.is<bool>());
        ASSERT_TRUE(_bool4.is<bool>());
        ASSERT_EQ(_bool1.as<bool>(), true);
        ASSERT_EQ(_bool2.as<bool>(), false);
        ASSERT_EQ(_bool3.as<bool>(), true);
        ASSERT_EQ(_bool4.as<bool>(), true);

        int _intv = -1;
        basic_json _int1{ -1 };
        basic_json _int2{ _intv };
        basic_json _int3{ std::move(_intv) };

        ASSERT_TRUE(_int1.is<int>());
        ASSERT_TRUE(_int2.is<int>());
        ASSERT_TRUE(_int3.is<int>());
        ASSERT_EQ(_int1.as<int>(), -1);
        ASSERT_EQ(_int2.as<int>(), -1);
        ASSERT_EQ(_int3.as<int>(), -1);

        unsigned int _uintv = 1;
        basic_json _uint1{ 1u };
        basic_json _uint2{ _uintv };
        basic_json _uint3{ std::move(_uintv) };

        ASSERT_TRUE(_uint1.is<unsigned int>());
        ASSERT_TRUE(_uint2.is<unsigned int>());
        ASSERT_TRUE(_uint3.is<unsigned int>());
        ASSERT_EQ(_uint1.as<unsigned int>(), 1);
        ASSERT_EQ(_uint2.as<unsigned int>(), 1);
        ASSERT_EQ(_uint3.as<unsigned int>(), 1);

        float _floatv = 1.f;
        basic_json _float1{ 1.f };
        basic_json _float2{ _floatv };
        basic_json _float3{ std::move(_floatv) };

        ASSERT_TRUE(_float1.is<float>());
        ASSERT_TRUE(_float2.is<float>());
        ASSERT_TRUE(_float3.is<float>());
        ASSERT_EQ(_float1.as<float>(), 1.f);
        ASSERT_EQ(_float2.as<float>(), 1.f);
        ASSERT_EQ(_float3.as<float>(), 1.f);

        float _doublev = 1.f;
        basic_json _double1{ 1. };
        basic_json _double2{ _doublev };
        basic_json _double3{ std::move(_doublev) };

        ASSERT_TRUE(_double1.is<double>());
        ASSERT_TRUE(_double2.is<double>());
        ASSERT_TRUE(_double3.is<double>());
        ASSERT_EQ(_double1.as<double>(), 1.);
        ASSERT_EQ(_double2.as<double>(), 1.);
        ASSERT_EQ(_double3.as<double>(), 1.);

        const char _stringv1[] = "hello";
        const char* _stringv2 = "hello";
        std::string _stringv3 = "hello";
        std::string_view _stringv4 = "hello";
        basic_json _string1{ "hello"};
        basic_json _string2{ std::string("hello") };
        basic_json _string3{ std::string_view("hello") };
        basic_json _string4{ _stringv1 };
        basic_json _string5{ _stringv2 };
        basic_json _string6{ _stringv3 };
        basic_json _string7{ _stringv4 };
        basic_json _string8{ std::move(_stringv1) };
        basic_json _string9{ std::move(_stringv2) };
        basic_json _stringA{ std::move(_stringv3) };
        basic_json _stringB{ std::move(_stringv4) };

        ASSERT_TRUE(_string1.is<std::string>());
        ASSERT_TRUE(_string2.is<std::string>());
        ASSERT_TRUE(_string3.is<std::string>());
        ASSERT_TRUE(_string4.is<std::string>());
        ASSERT_TRUE(_string5.is<std::string>());
        ASSERT_TRUE(_string6.is<std::string>());
        ASSERT_TRUE(_string7.is<std::string>());
        ASSERT_TRUE(_string8.is<std::string>());
        ASSERT_TRUE(_string9.is<std::string>());
        ASSERT_TRUE(_stringA.is<std::string>());
        ASSERT_TRUE(_stringB.is<std::string>());

        ASSERT_EQ(_string1.as<std::string_view>(), "hello");
        ASSERT_EQ(_string2.as<std::string_view>(), "hello");
        ASSERT_EQ(_string3.as<std::string_view>(), "hello");
        ASSERT_EQ(_string4.as<std::string_view>(), "hello");
        ASSERT_EQ(_string5.as<std::string_view>(), "hello");
        ASSERT_EQ(_string6.as<std::string_view>(), "hello");
        ASSERT_EQ(_string7.as<std::string_view>(), "hello");
        ASSERT_EQ(_string8.as<std::string_view>(), "hello");
        ASSERT_EQ(_string9.as<std::string_view>(), "hello");
        ASSERT_EQ(_stringA.as<std::string_view>(), "hello");
        ASSERT_EQ(_stringB.as<std::string_view>(), "hello");

    }

    // ------------------------------------------------

    TEST(BasicJsonTests, PushBack) {
        basic_json arr = basic_json::array_t();

        arr.push_back(int{});
        arr.push_back(unsigned int{});
        arr.push_back(float{});
        arr.push_back(double{});
        arr.push_back(std::string{});
        arr.push_back(std::string_view{});

    }

    // ------------------------------------------------
    
    class ParseNumberTests : public ::testing::TestWithParam<std::tuple<std::string, double>> {};

    TEST_P(ParseNumberTests, ParseNumber) {
        auto& [string, expected] = GetParam();
        basic_json::parser parser{ string };
        auto number = parser.parse_number();
        ASSERT_TRUE(number.has_value()) << number.error();
        double result = basic_json{ number.value() }.as<double>();
        ASSERT_EQ(result, expected);
    }

    INSTANTIATE_TEST_CASE_P(JsonNumbers, ParseNumberTests, ::testing::Values(
        std::make_tuple("1", 1),
        std::make_tuple("0", 0),
        std::make_tuple("12345", 12345),
        std::make_tuple("1.1", 1.1),
        std::make_tuple("0.1", 0.1),
        std::make_tuple("1.12345", 1.12345),
        std::make_tuple("12345.12345", 12345.12345),
        std::make_tuple("1e1", 1e1),
        std::make_tuple("0e1", 0e1),
        std::make_tuple("12345e1", 12345e1),
        std::make_tuple("1e2", 1e2),
        std::make_tuple("12345e2", 12345e2),
        std::make_tuple("1E2", 1E2),
        std::make_tuple("12345E2", 12345E2),
        std::make_tuple("1E+2", 1E+2),
        std::make_tuple("12345E+2", 12345E+2),
        std::make_tuple("1E-2", 1E-2),
        std::make_tuple("12345E-2", 12345E-2),
        std::make_tuple("1.0E-2", 1E-2),
        std::make_tuple("1.1E-2", 1.1E-2),
        std::make_tuple("1.12345E-2", 1.12345E-2),
        std::make_tuple("12345.12345E-2", 12345.12345E-2),        
        std::make_tuple("-1", -1),
        std::make_tuple("-0", -0),
        std::make_tuple("-12345", -12345),
        std::make_tuple("-1.1", -1.1),
        std::make_tuple("-0.1", -0.1),
        std::make_tuple("-1.12345", -1.12345),
        std::make_tuple("-12345.12345", -12345.12345),
        std::make_tuple("-1e1", -1e1),
        std::make_tuple("-0e1", -0e1),
        std::make_tuple("-12345e1", -12345e1),
        std::make_tuple("-1e2", -1e2),
        std::make_tuple("-12345e2", -12345e2),
        std::make_tuple("-1E2", -1E2),
        std::make_tuple("-12345E2", -12345E2),
        std::make_tuple("-1E+2", -1E+2),
        std::make_tuple("-12345E+2", -12345E+2),
        std::make_tuple("-1E-2", -1E-2),
        std::make_tuple("-12345E-2", -12345E-2),
        std::make_tuple("-1.0E-2", -1E-2),
        std::make_tuple("-1.1E-2", -1.1E-2),
        std::make_tuple("-1.12345E-2", -1.12345E-2),
        std::make_tuple("-12345.12345E-2", -12345.12345E-2)
    ));

    // ------------------------------------------------
    
    class ParseStringMemberTests : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

    TEST_P(ParseStringMemberTests, ParseString) {
        auto& [string, expected] = GetParam();
        basic_json::parser parser{ string };
        auto member = parser.parse_member();
        ASSERT_TRUE(member.has_value()) << member.error();
        auto& result = member.value();
        auto& key = result.first;
        auto& value = result.second;
        ASSERT_TRUE(value.is<std::string>());
        auto valueString = value.as<std::string_view>();
        ASSERT_EQ(key, "member");
        ASSERT_EQ(valueString, expected);
    }

    INSTANTIATE_TEST_CASE_P(JsonStrings, ParseStringMemberTests, ::testing::Values(
        std::make_tuple(R"~~("member":"value")~~", "value"),
        std::make_tuple(R"~~("member": "value" )~~", "value"),
        std::make_tuple(R"~~(member:"value")~~", "value"),
        std::make_tuple(R"~~(member: "value" )~~", "value"),
        std::make_tuple(R"~~( member :"value")~~", "value"),
        std::make_tuple(R"~~( member : "value" )~~", "value"),
        std::make_tuple(R"~~("member":value)~~", "value"),
        std::make_tuple(R"~~("member": value )~~", "value"),
        std::make_tuple(R"~~(member:value)~~", "value"),
        std::make_tuple(R"~~(member: value )~~", "value"),
        std::make_tuple(R"~~( member :value)~~", "value"),
        std::make_tuple(R"~~( member : value )~~", "value")
    ));

    // ------------------------------------------------
    
    // ------------------------------------------------
    
    class ParseJsonObjectTests : public ::testing::TestWithParam<std::tuple<std::string, basic_json>> {};

    TEST_P(ParseJsonObjectTests, ParseObject) {
        auto& [string, expected] = GetParam();
        auto object = basic_json::parse(string);
        ASSERT_TRUE(object.has_value()) << object.error();
        ASSERT_EQ(object.value(), expected);
    }

    INSTANTIATE_TEST_CASE_P(JsonObjects, ParseJsonObjectTests, ::testing::Values(
        std::make_tuple(R"~~({
            a: // comment
               v
            b: v v
            c: /* comment */ 1, d: v
            e: 1 #comment
            // Comment
            f: true, g /* comment */ : "v",
            h # comment
              : null,
            " ": true 1
            i: false 1
            j: null 1
            k: 1 1
            l: '''
               a
                b
                 c
               '''
            m: a: []
            n: " {} "
            o: '"\'',
            p: "\"'",
            q: "\/\\\b\f\n\r\t",
        })~~", basic_json{
            { "a", "v" },
            { "b", "v v" },
            { "c", 1 },
            { "d", "v" },
            { "e", 1 },
            { "f", true },
            { "g", "v"},
            { "h", nullptr },
            { " ", "true 1" },
            { "i", "false 1" },
            { "j", "null 1" },
            { "k", "1 1" },
            { "l", "a\n b\n  c" },
            { "m", "a: []" },
            { "n", " {} " },
            { "o", "\"'" },
            { "p", "\"'" },
            { "q", "\/\\\b\f\n\r\t" },
        })
    ));

    // ------------------------------------------------

}

// ------------------------------------------------
