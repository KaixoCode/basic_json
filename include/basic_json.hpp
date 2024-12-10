#pragma once

// ------------------------------------------------

#include <string_view>
#include <vector>
#include <charconv>
#include <map>
#include <variant>
#include <optional>
#include <string>
#include <array>
#include <stack>
#include <list>
#include <tuple>
#include <utility>
#include <type_traits>
#include <iostream>
#include <algorithm>
#include <expected>

// ------------------------------------------------

namespace kaixo {

    // ------------------------------------------------
    
    namespace detail {

        // ------------------------------------------------

        // Simple map implementation that keeps the insertion order. 
        // Uses an underlying list, meaning lookup is O(n).
        template<class Value>
        class ordered_flat_string_map : public std::list<std::pair<std::string, Value>> {
        public:

            // ------------------------------------------------
            
            using std::list<std::pair<std::string, Value>>::list;

            // ------------------------------------------------

            auto find(this auto& self, std::string_view value) {
                return std::ranges::find_if(self, [&](auto& it) { return it.first == value; });
            }

            bool contains(std::string_view value) const { return find(value) != this->end(); }

            // ------------------------------------------------

            Value& get_or_insert(std::string_view value) {
                auto it = find(value);
                if (it != this->end()) return it->second;
                return this->emplace_back(std::string{ value }, Value{}).second;
            }
            
            // ------------------------------------------------

            auto put(std::pair<std::string, Value> value, auto where) {
                remove(value.first); // remove any old value associated with key
                return ++this->insert(where, value);
            }

            // ------------------------------------------------
            
            auto remove(std::string_view value) {
                return std::remove_if(this->begin(), this->end(), [&](auto& val) { return val.first == value; });
            }

            // ------------------------------------------------

        };

        // ------------------------------------------------

    }

    // ------------------------------------------------
    
    class basic_json {
    public:

        // ------------------------------------------------

        enum type_index { number = 0, string, boolean, array, object, null, undefined };
        
        // ------------------------------------------------

        using number_t = std::variant<double, std::uint64_t, std::int64_t>;
        using string_t = std::string;
        using boolean_t = bool;
        using array_t = std::vector<basic_json>;
        using object_t = detail::ordered_flat_string_map<basic_json>;
        using null_t = std::nullptr_t;

        // ------------------------------------------------

    private:
        using value = std::variant<number_t, string_t, boolean_t, array_t, object_t, null_t>;

        // ------------------------------------------------

        value _value = null_t{};

        // ------------------------------------------------

        template<class Ty>
        struct type_alias : std::type_identity<null_t> {};
        
        template<> struct type_alias<object_t>  : std::type_identity<object_t> {};
        template<> struct type_alias<array_t>   : std::type_identity<array_t> {};
        template<> struct type_alias<boolean_t> : std::type_identity<boolean_t> {};

        template<class Ty> requires std::is_arithmetic_v<Ty>
        struct type_alias<Ty> : std::type_identity<number_t> {};
        
        template<class Ty> requires std::constructible_from<string_t, Ty>
        struct type_alias<Ty> : std::type_identity<string_t> {};

        template<class Ty>
        using type_alias_t = typename type_alias<Ty>::type;

        // ------------------------------------------------
        
        template<class Ty> requires std::is_arithmetic_v<Ty>
        struct number_type;
        
        template<std::floating_point Ty>    struct number_type<Ty> : std::type_identity<double> {};
        template<std::unsigned_integral Ty> struct number_type<Ty> : std::type_identity<std::uint64_t> {};
        template<std::signed_integral Ty>   struct number_type<Ty> : std::type_identity<std::int64_t> {};

        template<class Ty>
        using number_type_t = typename number_type<Ty>::type;

        // ------------------------------------------------

        template<class Ty> 
        struct type_index_of : type_index_of<type_alias_t<Ty>> {};

        template<> struct type_index_of<boolean_t> : std::integral_constant<type_index, boolean> {};
        template<> struct type_index_of<string_t>  : std::integral_constant<type_index, string> {};
        template<> struct type_index_of<number_t>  : std::integral_constant<type_index, number> {};
        template<> struct type_index_of<array_t>   : std::integral_constant<type_index, array> {};
        template<> struct type_index_of<object_t>  : std::integral_constant<type_index, object> {};
        template<> struct type_index_of<null_t>    : std::integral_constant<type_index, null> {};

        template<class Ty>
        constexpr static type_index type_index_of_v = type_index_of<Ty>::value;

    public:
        // ------------------------------------------------
        //                 Constructors
        // ------------------------------------------------

        basic_json() = default;

        basic_json(null_t) : _value(null_t{}) {}
        basic_json(boolean_t value) : _value(value)  {}

        basic_json(number_t&& value) : _value(std::move(value)) {}
        basic_json(object_t&& value) : _value(std::move(value)) {}
        basic_json(array_t&& value)  : _value(std::move(value)) {}
        basic_json(const number_t& value) : _value(value) {}
        basic_json(const object_t& value) : _value(value) {}
        basic_json(const array_t& value)  : _value(value) {}
        
        template<class Ty> requires std::constructible_from<string_t, Ty&&>
        basic_json(Ty&& value)
            : _value(string_t{ std::forward<Ty>(value) }) 
        {}

        template<class Ty> requires std::is_arithmetic_v<Ty>
        basic_json(Ty value)
            : _value(number_t{ static_cast<number_type_t<Ty>>(value) })
        {}
        
        basic_json(std::initializer_list<object_t::value_type> values)
            : _value(object_t{ values })
        {}
        
        // ------------------------------------------------
        
        bool operator==(const basic_json& other) const { 
            if (type() != other.type()) return false;
            if (!is<number_t>()) return _value == other._value;
            return std::visit([&](auto a) {
                return std::visit([&](auto b) {
                    return a == b; 
                }, std::get<number_t>(other._value));     
            }, std::get<number_t>(_value));
        }

        // ------------------------------------------------

        type_index type() const { 
            return static_cast<type_index>(_value.index());
        }

        template<class Ty = void>
        bool is(type_index t = undefined) const {
            if constexpr (std::same_as<Ty, void>) return t == type();
            else return type_index_of_v<Ty> == type();
        }

        // ------------------------------------------------

        bool contains(std::string_view key) const {
            if (!is<object_t>()) return false;
            return as<object_t>().contains(key);
        }

        template<class Ty = void>
        bool contains(std::string_view key, type_index type = undefined) const {
            if (!is<object_t>()) return false;
            auto& obj = as<object_t>();
            auto value = obj.find(key);
            return value != obj.end() && value->second.is<Ty>(type);
        }

        // ------------------------------------------------

        template<class Ty> requires (std::is_arithmetic_v<Ty> && !std::same_as<Ty, bool>)
        Ty as() const {
            const number_t& val = std::get<number_t>(_value);
            switch (val.index()) {
            case 0: return static_cast<Ty>(std::get<0>(val));
            case 1: return static_cast<Ty>(std::get<1>(val));
            case 2: return static_cast<Ty>(std::get<2>(val));
            default: throw std::bad_cast();
            }
        }

        template<std::same_as<boolean_t> Ty>               boolean_t as() const { return std::get<boolean_t>(_value); }
        template<std::same_as<std::string_view> Ty> std::string_view as() const { return std::get<string_t>(_value); }

        template<std::same_as<string_t> Ty>       string_t& as()       { return std::get<string_t>(_value); }
        template<std::same_as<string_t> Ty> const string_t& as() const { return std::get<string_t>(_value); }
        template<std::same_as<object_t> Ty>       object_t& as()       { return std::get<object_t>(_value); }
        template<std::same_as<object_t> Ty> const object_t& as() const { return std::get<object_t>(_value); }
        template<std::same_as<array_t> Ty>        array_t&  as()       { return std::get<array_t>(_value); }
        template<std::same_as<array_t> Ty>  const array_t&  as() const { return std::get<array_t>(_value); }
        
        // ------------------------------------------------

    private:
        template<class Ty>
        Ty& _get_or_assign() {
            if (is<null_t>()) _value = Ty{};
            else if (!is<Ty>()) throw std::exception("Invalid type.");
            return as<Ty>();
        }

        template<class Ty>
        const Ty& _get_or_assign() const {
            if (!is<Ty>()) throw std::exception("Invalid type.");
            return as<Ty>();
        }
    public:

        // ------------------------------------------------

        template<class Self>
        auto& operator[](this Self& self, std::string_view index) {
            auto& obj = self.template _get_or_assign<object_t>();
            auto _it = obj.find(index);
            if (_it == obj.end()) {
                if constexpr (std::is_const_v<Self>) {
                    throw std::exception("Invalid key.");
                } else {
                    return obj.get_or_insert(index);
                }
            }
            return _it->second;
        }

        template<class Self>
        auto& operator[](this Self& self, std::size_t index) {
            auto& arr = self.template _get_or_assign<array_t>();
            if (arr.size() <= index) {
                if constexpr (std::is_const_v<Self>) {
                    throw std::exception("Out of bounds");
                } else {
                    arr.resize(index + 1);
                }
            }
            return arr[index];
        }

        // ------------------------------------------------

        template<class Self>
        auto& at(this Self& self, std::string_view index) {
            auto& obj = self.template as<object_t>();
            auto _it = obj.find(index);
            if (_it == obj.end()) throw std::exception("Invalid key.");
            else return _it->second;
        }

        template<class Self>
        basic_json& at(this Self& self, std::size_t index) {
            auto& arr = self.template as<array_t>();
            if (arr.size() <= index) throw std::exception("Out of bounds");
            return arr[index];
        }
        
        // ------------------------------------------------
        
        template<class Ty>
        std::optional<Ty> get() const {
            if (is<Ty>()) return as<Ty>();
            else return {};
        }
        
        std::optional<basic_json> get(std::string_view key) const {
            if (contains(key)) return at(key);
            else return {};
        }

        template<class Ty>
        std::optional<Ty> get(std::string_view key) const {
            if (contains<Ty>(key)) return get(key)->template as<Ty>();
            else return {};
        }

        // ------------------------------------------------

        template<class Ty>
        bool try_get(Ty& value) const {
            if (is<Ty>()) return value = as<Ty>(), true;
            else return false;
        }

        template<class Ty>
        bool try_get(std::string_view key, Ty& value) const {
            if (auto val = get(key)) return val.value().try_get(value);
            return false;
        }
        
        template<class Ty>
        bool try_get(std::vector<Ty>& value) const {
            return foreach([&](auto& v) {
                if (v.template is<Ty>()) {
                    value.emplace_back(v.template as<Ty>());
                }
            });
        }
        
        template<class Ty, std::size_t N>
        bool try_get(std::array<Ty, N>& value) const {
            if (size() < N) return false;
            std::array<Ty, N> result;
            std::size_t index = 0;
            foreach([&](auto& v) {
                if (v.template is<Ty>()) {
                    if (index == N) return false; // too many elements
                    result[index] = v.template as<Ty>();
                    ++index;
                }
            });
            value = result;
            return true;
        }
        
        template<std::size_t N, class Ty>
        bool try_get(std::vector<Ty>& value) const {
            if (size() < N) return false;
            std::vector<Ty> result;
            std::size_t index = 0;
            foreach([&](auto& v) {
                if (v.template is<Ty>()) {
                    if (index == N) return false; // too many elements
                    result.emplace_back(v.template as<Ty>());
                    ++index;
                }
            });
            value = std::move(result);
            return true;
        }

        template<class Ty, class B>
        bool try_get_or_default(Ty& val, B&& def) const {
            if (try_get(val)) return true;
            val = std::forward<B>(def);
            return false;
        }
        
        template<class Ty, class B>
        bool try_get_or_default(std::string_view key, Ty& val, B&& def) const {
            if (try_get(key, val)) return true;
            val = std::forward<B>(def);
            return false;
        }
        
        // ------------------------------------------------
        
        template<class Functor, class Self >
        bool foreach(this Self& self, Functor&& fun) {
            using json_type = std::conditional_t<std::is_const_v<Self>, const basic_json&, basic_json&>;
            if constexpr (std::invocable<Functor, const std::string&, json_type>) {
                if (!self.template is<object_t>()) return false;
                for (auto& [key, val] : self.template as<object_t>()) fun(key, val);
                return true;
            } else if constexpr (std::invocable<Functor, json_type>) {
                if (!self.template is<array_t>()) return false;
                for (auto& val : self.template as<array_t>()) fun(val);
                return true;
            } else {
                static_assert(std::same_as<Functor, int>, "Wrong functor");
            }
        }

        template<class Functor, class Self>
        bool foreach(this Self& self, std::string_view key, Functor&& fun) {
            if (self.contains(key)) return self.at(key).foreach(fun);
            return false;
        }
        
        // ------------------------------------------------

        template<class Fun, class Self>
        void forall(this Self& self, Fun&& fun) {
            switch (self.type()) {
            case object: for (auto& [key, val] : self.template as<object_t>()) val.forall(fun); break;
            case array: for (auto& val : self.template as<array_t>()) val.forall(fun); break;
            default: fun(self); break;
            }
        }
        
        // ------------------------------------------------
        
        void merge(const basic_json& other) {
            switch (type()) {
            case object: merge(other, as<object_t>().end()); break;
            case null: *this = other; break;
            default: return; // Can't merge other types
            }
        }

        // Merges with other, inserts values from other at given iterator
        object_t::iterator merge(const basic_json& other, object_t::iterator where) {
            if (!is<object_t>()) return where; // Don't know where you got that iterator from, but I ain't an object
            other.foreach([&](const string_t& key, const basic_json& val) {
                if (!contains(key)) {
                    where = as<object_t>().put({ key, val }, where);
                } else if (val.is<object_t>()) {
                    this->operator[](key).merge(val);
                }
            });
            return where;
        }

        // ------------------------------------------------

        template<class Ty> requires std::constructible_from<basic_json, Ty&&>
        constexpr basic_json& push_back(Ty&& val) {
            return _get_or_assign<array_t>().emplace_back(std::forward<Ty>(val));
        }
        
        template<class Ty> requires std::constructible_from<basic_json, Ty&&>
        constexpr basic_json& push_front(Ty&& val) {
            auto& arr = _get_or_assign<array_t>();
            return *arr.emplace(arr.begin(), std::forward<Ty>(val));
        }

        // ------------------------------------------------

        bool empty() const { return size() == 0; }

        std::size_t size() const {
            return is<array_t>() ? as<array_t>().size() 
                : is<object_t>() ? as<object_t>().size() 
                : is<string_t>() ? as<string_t>().size() : 0ull;
        }

        // ------------------------------------------------

        std::string to_string() const {
            using namespace std::ranges;
            switch (type()) {
            case number: return std::visit([](auto& val) { return std::to_string(val); }, std::get<number_t>(_value));
            case string: return '"' + escape(as<string_t>()) + '"';
            case boolean: return as<boolean_t>() ? "true" : "false";
            case null: return "null";
            case array: return "[" 
                + fold_left_first(views::transform(as<array_t>(), 
                                                   [](auto& v) { return v.to_string(); }),
                                  [](auto&& a, auto&& b) { return a + "," + b; }).value_or("") 
                + "]";
            case object: return "{" 
                + fold_left_first(views::transform(as<object_t>(), 
                                                   [&](auto& v) { return '"' + escape(v.first) + "\":" + v.second.to_string(); }), 
                                  [](auto&& a, auto&& b) { return a + "," + b; }).value_or("") 
                + "}";
            default: return "";
            }
        }

        std::string to_pretty_string(std::size_t indent = 0) {
            std::string result;
            bool first = true;

            auto add = [&](std::string line = "", int x = 0, bool newline = true) {
                for (std::size_t i = 0; i < x + indent; ++i) result += "    ";
                result += line;
                if (newline) result += "\n";
            };

            switch (type()) {
            case array: {
                bool hasNestedObject = false;
                for (auto& val : as<array_t>()) {
                    if (val.is(basic_json::object) || val.is(basic_json::array)) {
                        hasNestedObject = true;
                        break;
                    }
                }

                if (hasNestedObject) {
                    result += "[\n";
                    for (auto& val : as<array_t>()) {
                        if (!first) result += ",\n";
                        add(val.to_pretty_string(indent + 1), 1, false);
                        first = false;
                    }
                    result += '\n';
                    add("]", 0, false);
                } else {
                    result += "[";
                    for (auto& val : as<array_t>()) {
                        if (!first) result += ", ";
                        result += val.to_pretty_string(indent + 1);
                        first = false;
                    }
                    result += "]";
                }
                return result;
            }
            case object: {
                result += "{\n";
                for (auto& [key, val] : as<object_t>()) {
                    if (!first) result += ",\n";
                    add("\"" + escape(key) + "\": " + val.to_pretty_string(indent + 1), 1, false);
                    first = false;
                }
                result += '\n';
                add("}", 0, false);
                return result;
            }
            default: return to_string();
            }
        }

        // ------------------------------------------------

        struct parser {

            // ------------------------------------------------

            constexpr static std::string_view whitespace = " \t\n\r\f\v";
            constexpr static std::string_view whitespace_no_lf = " \t\r\f\v";

            // ------------------------------------------------

            template<class Ty = void>
            using result = std::expected<Ty, std::string>;

            // ------------------------------------------------

            std::string_view original;
            std::string_view value = original;

            // ------------------------------------------------

            std::unexpected<std::string> die(std::string_view message) const {
                std::size_t newlines = 0, charsInLine = 0;
                for (auto& c : original.substr(0, original.size() - value.size())) {
                    ++charsInLine;
                    if (c == '\n') ++newlines, charsInLine = 0;
                }
                return std::unexpected{ std::format("line {}, character {}: {}", newlines, charsInLine, message) };
            }

            // ------------------------------------------------

            auto push() { 
                struct undo {
                    parser* self;
                    std::string_view backup;
                    bool committed = false;
                    void commit() { committed = true; }
                    void revert() { committed = false; }
                    ~undo() { if (!committed) self->value = backup; }
                };
                return undo{ this, value };
            }

            // ------------------------------------------------

            std::optional<char> consume_one_of(std::string_view chars) {
                if (value.empty() || !one_of(value[0], chars)) return std::nullopt;
                char result = value[0];
                value = value.substr(1);
                return result;
            }
            
            bool consume(std::string_view word) {
                if (!value.starts_with(word)) return false;
                value = value.substr(word.size());
                return true;
            }

            std::string_view consume_first(std::size_t i) {
                std::size_t _end = std::min(i, value.size());
                auto _result = value.substr(0, _end);
                value = value.substr(_end);
                return _result;
            }
            
            std::string_view consume_while(std::string_view oneOfs) { return consume_first(value.find_first_not_of(oneOfs)); }
            std::string_view consume_while_not(std::string_view oneOfs) { return consume_first(value.find_first_of(oneOfs)); }

            // ------------------------------------------------
            
            std::size_t nof_characters_since_last(char find) const {
                std::size_t parsed = original.size() - value.size();
                std::size_t index = original.substr(0, parsed).find_last_of(find);
                if (index == std::string_view::npos) return std::string_view::npos;
                return parsed - index - 1;
            }

            // ------------------------------------------------

            void ignore(std::string_view anyOf = whitespace) { consume_while(anyOf); }

            void removeIgnored(bool newline = true) {
                parse_comment();
                ignore(newline ? whitespace : whitespace_no_lf);
            }

            // ------------------------------------------------

            template<class Lambda, class Assign>
            bool maybe(Lambda&& fun, Assign&& assign) {
                auto result = fun();
                if (result.has_value()) {
                    if constexpr (std::invocable<Assign, decltype(std::move(result.value()))>) {
                        assign(std::move(result.value()));
                    } else {
                        assign = std::move(result.value());
                    }
                }
                return result.has_value();
            }

            // ------------------------------------------------

            template<class Lambda, class Assign>
            void parse_list(Lambda&& fun, Assign&& assign) {
                if (!maybe(fun, assign)) return;
                while (true) {
                    { // First try comma
                        auto _ = push(); 
                        removeIgnored();
                        if (consume(",")) {
                            _.commit();
                            if (!maybe(fun, assign)) return;
                            continue;
                        }
                    }
                    { // Otherwise try LF
                        auto _ = push();
                        removeIgnored(false);
                        if (consume("\n")) {
                            _.commit();
                            if (!maybe(fun, assign)) return;
                            continue;
                        }
                    }
                    return;
                }
            }

            // ------------------------------------------------

            result<int> parse_comment() {
                int nofCommentsParsed = 0;
                while (true) {
                    auto _ = push();

                    ignore(whitespace);
                    if (consume("#")) consume_while_not("\n");
                    else if (consume("//")) consume_while_not("\n");
                    else if (consume("/*")) {
                        bool closed = false;
                        while (!value.empty()) {
                            consume_while_not("*");
                            if (consume("*") && consume("/")) {
                                closed = true;
                                break;
                            }
                        }

                        if (!closed) {
                            _.revert();
                            return die("Expected end of multi-line comment");
                        }
                    } else {
                        if (nofCommentsParsed == 0) return die("No comments parsed"); // No more comments
                        else return nofCommentsParsed;
                    }

                    ++nofCommentsParsed;

                    _.commit();
                }
            }

            // ------------------------------------------------

            result<number_t> parse_number() {
                auto _ = push();
                removeIgnored();

                bool negative = false;
                if (consume("-")) negative = true;

                std::string pre = "";
                if (consume("0")) {
                    pre = "0";
                } else {
                    if (auto c = consume_one_of("123456789")) pre += c.value();
                    else return die("Expected at least 1 digit in number");
                    while (auto c = consume_one_of("0123456789")) pre += c.value();
                }

                std::string post = "";
                bool fractional = false;
                if (consume(".")) {
                    fractional = true;
                    while (auto c = consume_one_of("0123456789")) post += c.value();
                    if (post.empty()) return die("Expected at least 1 decimal digit");
                }

                std::string exponent = "";
                bool hasExponent = false;
                bool negativeExponent = false;
                if (consume_one_of("eE")) {
                    hasExponent = true;

                    if (consume("+")) negativeExponent = false;
                    else if (consume("-")) negativeExponent = true;

                    while (auto c = consume_one_of("0123456789")) exponent += c.value();
                    if (exponent.empty()) return die("Expected at least 1 exponent digit");
                }

                _.commit();
                std::string fullStr = (fractional ? pre + "." + post : pre)
                                    + (hasExponent ? (negativeExponent ? "E-" : "E+") + exponent : "");
                if (fractional || hasExponent) {
                    double val = 0;
                    std::from_chars(fullStr.data(), fullStr.data() + fullStr.size(), val);
                    return { negative ? -val : val };
                } else {
                    std::uint64_t val = 0;
                    std::from_chars(fullStr.data(), fullStr.data() + fullStr.size(), val);
                    return negative ? number_t{ -static_cast<std::int64_t>(val) } : number_t{ val };
                }
            }

            // ------------------------------------------------

            result<string_t> parse_json_string() {
                auto _ = push();
                string_t _result{};

                removeIgnored();
                auto v = consume_one_of("\"'");
                if (!v) return die("Expected \" or ' to start json string");
                bool smallQuote = v == '\'';
                while (true) {
                    _result += consume_while_not(smallQuote ? "'\\" : "\"\\");
                    if (consume(smallQuote ? "\'" : "\"")) break; // String ended
                    if (consume("\\")) { // Escaped character
                        if (consume("\"")) _result += "\"";
                        else if (consume("\'")) _result += "\'";
                        else if (consume("\\")) _result += "\\";
                        else if (consume("/")) _result += "/";
                        else if (consume("b")) _result += "\b";
                        else if (consume("f")) _result += "\f";
                        else if (consume("n")) _result += "\n";
                        else if (consume("r")) _result += "\r";
                        else if (consume("t")) _result += "\t";
                        else if (consume("u")) return die("Unicode is currently not supported");
                        else return die("Wrong escape character");
                    }
                }

                _.commit();
                return _result;
            }

            result<string_t> parse_quoteless_string() {
                auto _ = push();
                removeIgnored();
                
                if (value.empty() || one_of(value[0], "[]{},:")) 
                    return die("Quoteless string cannot start with any of \"[]{},:\"");

                _.commit();
                auto _result = consume_while_not("\n");
                _result = _result.substr(0, _result.find_last_not_of(whitespace) + 1);
                return string_t{ _result }; // ^^^ Remove whitespace from end
            }
            
            result<string_t> parse_multiline_string() {
                auto _ = push();
                removeIgnored();

                std::int64_t _columnsBeforeStart = nof_characters_since_last('\n');
                if (!consume("'''")) return die("Expected ''' to start multi-line string");
                ignore(whitespace_no_lf);
                consume("\n");

                bool first = true;
                string_t _result = "";
                while (true) {
                    ignore(whitespace_no_lf);

                    std::size_t index = nof_characters_since_last('\n');
                    if (index == std::string_view::npos) return die("Unexpected error");
                    std::int64_t _spaces = std::max(static_cast<std::int64_t>(index) - _columnsBeforeStart, 0ll);

                    if (consume("'''")) break; // End of string
                    
                    if (first) {
                        first = false;
                    } else {
                        _result += '\n';
                    }

                    for (std::size_t i = 0; i < _spaces; ++i) _result += ' ';
                    _result += consume_while_not("\n"); // Consume til end of line
                    consume("\n");
                }

                _.commit();
                return _result;
            }

            result<string_t> parse_string() {
                if (auto str = parse_multiline_string()) return str.value();
                if (auto str = parse_json_string()) return str.value();
                if (auto str = parse_quoteless_string()) return str.value();
                return die("Expected string");
            }

            // ------------------------------------------------

            result<std::pair<string_t, basic_json>> parse_member() {
                auto _ = push();
                string_t _key{};
                basic_json _value{};

                removeIgnored();
                if (!maybe([&] { return parse_json_string(); }, _key)) _key = consume_while_not(",:[]{} \t\n\r\f\v");
                if (_key.empty()) return die("Cannot have empty key");
                removeIgnored();
                if (!consume(":")) return die("Expected ':' after key name");
                if (!maybe([&] { return parse_value(); }, _value)) return die("Expected value");

                _.commit();
                return { { _key, _value } };
            }

            // ------------------------------------------------

            result<object_t> parse_object() {
                auto _ = push();
                object_t _result{};

                removeIgnored();
                if (!consume("{")) return die("Expected '{' to begin Object");
                parse_list([&] { return parse_member(); }, [&](auto&& val) { _result.put(val, _result.end()); });
                removeIgnored();
                if (!consume("}")) return die("Expected '}' to close Object");

                _.commit();
                return _result;
            }

            // ------------------------------------------------

            result<array_t> parse_array() {
                auto _ = push();
                array_t _result{};
                
                removeIgnored();
                if (!consume("[")) return die("Expected '[' to begin Array");
                parse_list([&] { return parse_value(); }, [&](auto&& val) { _result.push_back(val); });
                removeIgnored();
                if (!consume("]")) return die("Expected ']' to close Array");

                _.commit();
                return _result;
            }

            // ------------------------------------------------

            result<basic_json> parse_value_ambiguous() {
                auto _ = push();
                basic_json _value{};

                removeIgnored();
                if (consume("true")) _value = true;
                else if (consume("false")) _value = false;
                else if (consume("null")) _value = nullptr;
                else if (maybe([&] { return parse_number(); }, _value));
                else return die(""); // Not a potentially ambiguous value

                // Because after a true/false/null/number there could
                // be other characters that could turn it into a string
                // check whether this really is the parsed type.
                auto backup = push(); // backup, because we don't want to actually consume 
                ignore(whitespace_no_lf);
                if (parse_comment() || consume_one_of("\n,][}{:")) {
                    _.commit();
                    return _value;
                }

                return die("");
            }

            result<basic_json> parse_value() {
                if (auto _value = parse_object()) return _value;
                if (auto _value = parse_array()) return _value;
                if (auto _value = parse_value_ambiguous()) return _value;
                if (auto _value = parse_string()) return _value;
                return die("Expected value");
            }

            // ------------------------------------------------

        };

        // ------------------------------------------------
        
        static parser::result<basic_json> parse(std::string_view json) { return parser{ json }.parse_object(); }

        // ------------------------------------------------
        
    private:
        constexpr static std::string escape(std::string_view str) {
            std::string _str{ str };
            string_replace(_str, "\\", "\\\\");
            string_replace(_str, "\b", "\\b");
            string_replace(_str, "\f", "\\f");
            string_replace(_str, "\n", "\\n");
            string_replace(_str, "\r", "\\r");
            string_replace(_str, "\t", "\\t");
            string_replace(_str, "\"", "\\\"");
            string_replace(_str, "\'", "\\'");
            string_replace(_str, "/", "\\/");
            return _str;
        }

        constexpr static bool one_of(char c, std::string_view cs) { return cs.find(c) != std::string_view::npos; }

        constexpr static void string_replace(std::string& str, std::string_view from, std::string_view to) {
            for (std::size_t start_pos = 0; (start_pos = str.find(from, start_pos)) != std::string::npos; start_pos += to.length())
                str.replace(start_pos, from.length(), to);
        }

        // ------------------------------------------------

    };

    // ------------------------------------------------
    
    std::ostream& operator<<(std::ostream& stream, const basic_json& object) { return stream << object.to_string(); }

    // ------------------------------------------------
    
}

// ------------------------------------------------
