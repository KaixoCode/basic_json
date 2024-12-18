#pragma once

// ------------------------------------------------

#include <algorithm>
#include <array>
#include <charconv>
#include <expected>
#include <list>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// ------------------------------------------------

namespace kaixo {

    // ------------------------------------------------
    
    class basic_json {

        // ------------------------------------------------

        // Simple map implementation that keeps the insertion order. 
        // Uses an underlying list, meaning lookup is O(n).
        struct map : std::list<std::pair<std::string, basic_json>> {

            // ------------------------------------------------

            auto find(this auto& self, std::string_view value) {
                return std::ranges::find_if(self, [&](auto& it) { return it.first == value; });
            }

            bool contains(std::string_view value) const { return find(value) != this->end(); }

            // ------------------------------------------------

            basic_json& get_or_insert(std::string_view value) {
                auto it = find(value);
                if (it != this->end()) return it->second;
                return this->emplace_back(std::string{ value }, basic_json{}).second;
            }

            // ------------------------------------------------

            auto put(std::pair<std::string, basic_json> value, auto where) {
                remove(value.first); // remove any old value associated with key
                return ++this->insert(where, value);
            }

            auto remove(std::string_view value) {
                auto res = std::find_if(this->begin(), this->end(), [&](auto& val) { return val.first == value; });
                if (res == this->end()) return res;
                return this->erase(res);
            }

            // ------------------------------------------------

        };

        // ------------------------------------------------

    public:
        enum type_index { number = 0, string, boolean, array, object, null, undefined };
        
        // ------------------------------------------------

        using number_t = std::variant<double, std::uint64_t, std::int64_t>;
        using string_t = std::string;
        using boolean_t = bool;
        using array_t = std::vector<basic_json>;
        using object_t = map;
        using null_t = std::nullptr_t;

        // ------------------------------------------------

    private:
        using value = std::variant<number_t, string_t, boolean_t, array_t, object_t, null_t>;

        // ------------------------------------------------

        value _value = null_t{};

        // ------------------------------------------------

        template<class Ty> struct type_alias : std::type_identity<null_t> {};
        template<> struct type_alias<object_t>  : std::type_identity<object_t> {};
        template<> struct type_alias<array_t>   : std::type_identity<array_t> {};
        template<> struct type_alias<boolean_t> : std::type_identity<boolean_t> {};

        template<class Ty> requires std::is_arithmetic_v<Ty>
        struct type_alias<Ty> : std::type_identity<number_t> {};
        
        template<class Ty> requires std::constructible_from<string_t, Ty>
        struct type_alias<Ty> : std::type_identity<string_t> {};

        // ------------------------------------------------
        
        template<class Ty> requires std::is_arithmetic_v<Ty> struct number_type;
        template<std::floating_point Ty>    struct number_type<Ty> : std::type_identity<double> {};
        template<std::unsigned_integral Ty> struct number_type<Ty> : std::type_identity<std::uint64_t> {};
        template<std::signed_integral Ty>   struct number_type<Ty> : std::type_identity<std::int64_t> {};

        // ------------------------------------------------

        template<class Ty> struct type_index_of : type_index_of<typename type_alias<Ty>::type> {};
        template<> struct type_index_of<boolean_t> : std::integral_constant<type_index, boolean> {};
        template<> struct type_index_of<string_t>  : std::integral_constant<type_index, string> {};
        template<> struct type_index_of<number_t>  : std::integral_constant<type_index, number> {};
        template<> struct type_index_of<array_t>   : std::integral_constant<type_index, array> {};
        template<> struct type_index_of<object_t>  : std::integral_constant<type_index, object> {};
        template<> struct type_index_of<null_t>    : std::integral_constant<type_index, null> {};

        // ------------------------------------------------

    public:
        basic_json() = default;
        basic_json(null_t) : _value(null_t{}) {}
        basic_json(boolean_t value) : _value(value)  {}
        basic_json(number_t&& value) : _value(std::move(value)) {}
        basic_json(object_t&& value) : _value(std::move(value)) {}
        basic_json(array_t&& value)  : _value(std::move(value)) {}
        basic_json(const number_t& value) : _value(value) {}
        basic_json(const object_t& value) : _value(value) {}
        basic_json(const array_t& value)  : _value(value) {}
        basic_json(std::initializer_list<object_t::value_type> values) : _value(object_t{ values }) {}

        template<class Ty> requires std::constructible_from<string_t, Ty&&>
        basic_json(Ty&& value)
            : _value(string_t{ std::forward<Ty>(value) }) 
        {}

        template<class Ty> requires std::is_arithmetic_v<Ty>
        basic_json(Ty value)
            : _value(number_t{ static_cast<typename number_type<Ty>::type>(value) })
        {}

        template<class Ty> requires std::constructible_from<basic_json, const Ty&>
        basic_json(const std::vector<Ty>& values)
            : _value(array_t{ values.begin(), values.end() })
        {}
        
        // ------------------------------------------------
        
        bool operator==(const basic_json& other) const { 
            if (type() != other.type()) return false;
            if (!is<number_t>()) return _value == other._value;
            return std::visit([&](auto a, auto b) { return a == b; }, 
                std::get<number_t>(_value), std::get<number_t>(other._value));
        }

        // ------------------------------------------------

        type_index type() const { return static_cast<type_index>(_value.index()); }

        template<class Ty = void>
        bool is(type_index t = undefined) const {
            if constexpr (std::same_as<Ty, void>) return t == type();
            else return type_index_of<Ty>::value == type();
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
        Ty as() const { return std::visit([](auto val) { return static_cast<Ty>(val); }, std::get<number_t>(_value)); }

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
        auto& at(this Self& self, std::size_t index) {
            auto& arr = self.template as<array_t>();
            if (arr.size() <= index) throw std::exception("Out of bounds");
            return arr[index];
        }
        
        // ------------------------------------------------
        
        template<class Ty>
        std::optional<Ty> get() const {
            return is<Ty>() ? std::optional{ as<Ty>() } : std::nullopt;
        }
        
        std::optional<basic_json> get(std::string_view key) const {
            return contains(key) ? std::optional{ at(key) } : std::nullopt;
        }

        template<class Ty>
        std::optional<Ty> get(std::string_view key) const {
            return contains(key) ? get(key)->get<Ty>() : std::nullopt;
        }

        // ------------------------------------------------

        template<class Ty>
        bool try_get(Ty& value) const {
            return is<Ty>() ? value = as<Ty>(), true : false;
        }

        template<class Ty>
        bool try_get(std::string_view key, Ty& value) const {
            if (auto val = get(key)) return val.value().try_get(value);
            return false;
        }
        
        template<class Ty>
        bool try_get(std::vector<Ty>& value) const {
            return foreach([&](auto& v) {
                if (v.template is<Ty>()) value.emplace_back(v.template as<Ty>());
            });
        }
        
        template<class Ty, std::size_t N>
        bool try_get(std::array<Ty, N>& value) const {
            if (size() < N) return false;
            std::array<Ty, N> result{};
            std::size_t index = 0;
            foreach([&](auto& v) {
                if (v.template is<Ty>()) {
                    if (index == N) return false; // too many elements
                    result[index++] = v.template as<Ty>();
                }
            });
            value = std::move(result);
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
        
        template<class Ty>
        bool try_get_or_default(Ty& val, Ty&& def) const {
            if (try_get(val)) return true;
            val = std::forward<Ty>(def);
            return false;
        }
        
        template<class Ty, class B>
        bool try_get_or_default(std::string_view key, Ty& val, B&& def) const {
            if (try_get(key, val)) return true;
            val = std::forward<B>(def);
            return false;
        }
        
        template<class Ty>
        bool try_get_or_default(std::string_view key, Ty& val, Ty&& def) const {
            if (try_get(key, val)) return true;
            val = std::forward<Ty>(def);
            return false;
        }
        
        // ------------------------------------------------
        
        template<class Functor, class Self>
            requires (std::invocable<Functor&, const std::string&, Self&>
                    || std::invocable<Functor&, Self&>)
        bool foreach(this Self& self, Functor&& fun) {
            if constexpr (std::invocable<Functor&, const std::string&, Self&>) {
                if (!self.template is<object_t>()) return false;
                for (auto& [key, val] : self.template as<object_t>()) fun(key, val);
                return true;
            } else if constexpr (std::invocable<Functor&, Self&>) {
                if (!self.template is<array_t>()) return false;
                for (auto& val : self.template as<array_t>()) fun(val);
                return true;
            }
        }

        template<class Functor, class Self>
            requires (std::invocable<Functor&, const std::string&, Self&>
                   || std::invocable<Functor&, Self&>)
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
                if (!contains(key)) where = as<object_t>().put({ key, val }, where);
                else if (val.is<object_t>()) this->operator[](key).merge(val);
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

        template<class ...Args> requires std::constructible_from<basic_json, Args&&...>
        basic_json& put(std::string_view value, Args&& ...args) {
            return operator[](value) = basic_json{ std::forward<Args>(args)... };
        }

        // ------------------------------------------------

        void remove(std::string_view index) { _get_or_assign<object_t>().remove(index); }

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
            case array: return '[' + fold_left_first(views::transform(as<array_t>(),
                    [](auto& v) { return v.to_string(); }),
                    [](auto&& a, auto&& b) { return a + ',' + b; }).value_or("") + ']';
            case object: return '{' + fold_left_first(views::transform(as<object_t>(),
                    [](auto& v) { return '"' + escape(v.first) + "\":" + v.second.to_string(); }), 
                    [](auto&& a, auto&& b) { return a + ',' + b; }).value_or("") + '}';
            default: return "";
            }
        }

        std::string to_pretty_string(std::size_t indent = 0) {
            using namespace std::ranges;
            const std::string spaces(indent * 4, ' ');
            switch (type()) {
            case array: {
                bool hasNestedObject = as<array_t>().end() != std::ranges::find_if(as<array_t>(), [](auto& val) {
                    return (val.is(basic_json::object) || val.is(basic_json::array)) && !val.empty();
                });

                if (hasNestedObject) {
                    return "[\n" + fold_left_first(views::transform(as<array_t>(),
                        [&](auto& v) { return spaces + "    " + v.to_pretty_string(indent + 1); }),
                        [](auto&& a, auto&& b) { return a + ",\n" + b; }).value_or("") + '\n' + spaces + ']';
                } 

                return to_string();
            }
            case object:
                if (empty()) return "{}";
                return "{\n" + fold_left_first(views::transform(as<object_t>(),
                    [&](auto& v) { return spaces + "    " + '"' + escape(v.first) + "\": " + v.second.to_pretty_string(indent + 1); }),
                    [](auto&& a, auto&& b) { return a + ",\n" + b; }).value_or("") + '\n' + spaces + '}';
            default: return to_string();
            }
        }

        // ------------------------------------------------

        // HJSON parser: https://hjson.github.io/syntax.html
        struct parser {

            // ------------------------------------------------

            constexpr static std::string_view whitespace = " \t\n\r\f\v";
            constexpr static std::string_view whitespace_no_lf = " \t\r\f\v";

            // ------------------------------------------------

            template<class Ty = void> using result = std::expected<Ty, std::string>;

            // ------------------------------------------------

            std::string_view original;
            std::string_view value = original;

            // ------------------------------------------------

            struct backup_struct {

                // ------------------------------------------------

                parser* self;
                std::string_view backup;

                // ------------------------------------------------

                void revert() { self->value = backup; }

                // ------------------------------------------------

                std::unexpected<std::string> revert(std::string_view message) {
                    std::string_view parsed = self->original.substr(0, self->original.size() - self->value.size());
                    std::size_t newlines = 0;
                    std::size_t charsInLine = 0;
                    for (auto& c : parsed) {
                        ++charsInLine;
                        if (c == '\n') ++newlines, charsInLine = 0;
                    }
                    
                    revert();
                    return std::unexpected{ std::format("line {}, character {}: {}", newlines, charsInLine, message) };
                }

                // ------------------------------------------------

            };

            // ------------------------------------------------

            backup_struct backup() { return backup_struct{ this, value }; }
            std::unexpected<std::string> fail(std::string_view message) { return backup().revert(message); }

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
                parse_comment(newline);
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
                        auto _ = backup(); 
                        removeIgnored();
                        if (consume(",")) {
                            if (!maybe(fun, assign)) return;
                            continue;
                        }
                        _.revert();
                    }
                    { // Otherwise try LF
                        auto _ = backup();
                        removeIgnored(false);
                        if (consume("\n")) {
                            if (!maybe(fun, assign)) return;
                            continue;
                        }
                        _.revert();
                    }
                    return;
                }
            }

            // ------------------------------------------------

            result<int> parse_comment(bool newline = true) {
                for (int nofCommentsParsed = 0;; ++nofCommentsParsed) {
                    auto _ = backup();

                    ignore(newline ? whitespace : whitespace_no_lf);
                    if (consume("#")) consume_while_not("\n");
                    else if (consume("//")) consume_while_not("\n");
                    else if (consume("/*")) {
                        bool closed = false;
                        while (!value.empty()) {
                            consume_while_not("*");
                            if (consume("*") && consume("/")) { closed = true; break; }
                        }
                        if (!closed) return _.revert("Expected end of multi-line comment");
                    } else {
                        if (nofCommentsParsed == 0) return _.revert("No comments parsed"); // No more comments
                        else return nofCommentsParsed;
                    }
                }
            }

            // ------------------------------------------------

            result<number_t> parse_number() {
                auto _ = backup();
                removeIgnored();
                std::string exponent = "", pre = "", post = "";
                bool negative = consume("-"), hasExponent = false, negativeExponent = false, fractional = false;

                if (consume("0")) pre = "0";
                else while (auto c = consume_one_of("0123456789")) pre += c.value();
                if (pre.empty()) return _.revert("Expected at least 1 digit in number");

                if (fractional = consume(".")) {
                    while (auto c = consume_one_of("0123456789")) post += c.value();
                    if (post.empty()) return _.revert("Expected at least 1 decimal digit");
                }

                if (hasExponent = static_cast<bool>(consume_one_of("eE"))) {
                    if (consume("+")) negativeExponent = false;
                    else if (consume("-")) negativeExponent = true;
                    while (auto c = consume_one_of("0123456789")) exponent += c.value();
                    if (exponent.empty()) return _.revert("Expected at least 1 exponent digit");
                }

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
                auto _ = backup();
                string_t _result{};

                removeIgnored();
                auto v = consume_one_of("\"'");
                if (!v) return _.revert("Expected \" or ' to start json string");
                bool smallQuote = v == '\'';
                while (!value.empty()) {
                    _result += consume_while_not(smallQuote ? "'\\" : "\"\\");
                    if (consume(smallQuote ? "\'" : "\"")) return _result; // String ended
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
                        else if (consume("u")) return _.revert("Unicode is currently not supported");
                        else return _.revert("Wrong escape character");
                    }
                }

                return _.revert("Expected \" or ' to end json string");
            }

            result<string_t> parse_quoteless_string() {
                auto _ = backup();
                removeIgnored();
                
                if (consume_one_of("[]{},:")) return _.revert("Quoteless string cannot start with any of \"[]{},:\"");
                auto _result = consume_while_not("\n");
                _result = _result.substr(0, _result.find_last_not_of(whitespace) + 1);
                return string_t{ _result }; // ^^^ Remove whitespace from end
            }
            
            result<string_t> parse_multiline_string() {
                auto _ = backup();
                removeIgnored();
                string_t _result = "";

                std::int64_t _columnsBeforeStart = nof_characters_since_last('\n');
                if (!consume("'''")) return _.revert("Expected ''' to start multi-line string");
                ignore(whitespace_no_lf);
                bool startsOnNewLine = consume("\n");

                bool firstLine = true;
                while (!value.empty()) {
                    ignore(whitespace_no_lf);
                    if (consume("'''")) return _result; // End of string

                    std::size_t index = nof_characters_since_last('\n');
                    if (index == std::string_view::npos) return _.revert("Unexpected error");
                    std::int64_t spaces = std::max(static_cast<std::int64_t>(index) - _columnsBeforeStart, 0ll);
                    if (firstLine && !startsOnNewLine) spaces -= 3; // remove 3 spaces to account for ''' on first line
                    
                    if (!firstLine) _result += '\n';
                    else firstLine = false;

                     _result += std::string(spaces, ' ');
                    while (!value.empty()) {
                        _result += consume_while_not("\n'");
                        if (consume("'''")) return _result; // End of string
                        else if (consume("\n")) break;      // End of line
                    }
                }

                return _.revert("Expected ''' to end multi-line string");
            }

            result<string_t> parse_string() {
                if (auto str = parse_multiline_string()) return str;
                if (auto str = parse_json_string()) return str;
                if (auto str = parse_quoteless_string()) return str;
                return fail("Expected string");
            }

            // ------------------------------------------------

            result<std::pair<string_t, basic_json>> parse_member() {
                auto _ = backup();
                string_t _key{};
                basic_json _value{};

                removeIgnored();
                if (!maybe([&] { return parse_json_string(); }, _key)) _key = consume_while_not(",:[]{} \t\n\r\f\v");
                if (_key.empty()) return _.revert("Cannot have empty key");
                removeIgnored();
                if (!consume(":")) return _.revert("Expected ':' after key name");
                if (!maybe([&] { return parse_value(); }, _value)) return _.revert("Expected value");

                return { { _key, _value } };
            }

            // ------------------------------------------------

            result<object_t> parse_object() {
                auto _ = backup();
                object_t _result{};

                removeIgnored();
                if (!consume("{")) return _.revert("Expected '{' to begin Object");
                parse_list([&] { return parse_member(); }, [&](auto&& val) { _result.put(std::move(val), _result.end()); });
                removeIgnored();
                if (!consume("}")) return _.revert("Expected '}' to close Object");

                return _result;
            }

            // ------------------------------------------------

            result<array_t> parse_array() {
                auto _ = backup();
                array_t _result{};
                
                removeIgnored();
                if (!consume("[")) return _.revert("Expected '[' to begin Array");
                parse_list([&] { return parse_value(); }, [&](auto&& val) { _result.push_back(std::move(val)); });
                removeIgnored();
                if (!consume("]")) return _.revert("Expected ']' to close Array");

                return _result;
            }

            // ------------------------------------------------

            result<basic_json> parse_value_ambiguous() {
                auto _ = backup();
                basic_json _value{};

                removeIgnored();
                if (consume("true")) _value = true;
                else if (consume("false")) _value = false;
                else if (consume("null")) _value = nullptr;
                else if (maybe([&] { return parse_number(); }, _value));
                else return _.revert("Not a potentially ambiguous value");

                // Because after a true/false/null/number there could be other characters that
                // could turn it into a string check whether this really is the parsed type.
                auto temp = backup(); // backup, because we don't want to actually consume 
                ignore(whitespace_no_lf);
                if (parse_comment() || consume_one_of("\n,][}{:")) {
                    temp.revert();
                    return _value;
                }

                return _.revert("Value turned out to be a string");
            }

            result<basic_json> parse_value() {
                if (auto _value = parse_object()) return _value;
                if (auto _value = parse_array()) return _value;
                if (auto _value = parse_value_ambiguous()) return _value;
                if (auto _value = parse_string()) return _value;
                return fail("Expected value");
            }

            // ------------------------------------------------

        };

        // ------------------------------------------------
        
        static parser::result<basic_json> parse(std::string_view json) { return parser{ json }.parse_value(); }

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
