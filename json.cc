#include "json.hpp"
#include <cassert>
#include <cstdlib>
#include <limits>
#include <cmath>
#include <iostream>
namespace tiny_json {
static constexpr size_t MAX_DEPTH = 200;
struct NullStruct {
    bool operator==(NullStruct) const {return true;}
    bool operator< (NullStruct) const {return true;}
};

static void dump(NullStruct,std::string &out) {
    out += "null";
}
static void dump(double value,std::string &out) {
    if(std::isfinite(value)) {
        char buf[32] = {0};
        snprintf(buf,sizeof buf,"%.17g",value);
        out += buf;
    }
    else {
        out += "null";
    }
}
static void dump(int value,std::string &out) {
    char buf[32] = {0};
    snprintf(buf,sizeof buf,"%d",value);
    out += buf;
}
static void dump(bool value,std::string &out) {
    out += value ? "true" : "false";
}
static void dump(const std::string &value,std::string &out) {
    out += '"';
    for(size_t i = 0;i < value.size();i ++) {
        const char ch = value[i];
        if(ch == '\\') {
            out += "\\\\";
        }
        else if(ch == '"') {
            out += "\\\"";
        }
        else if(ch == '\b') {
            out += "\\b";
        }
        else if(ch == '\f') {
            out += "\\f";
        }
        else if(ch == '\n') {
            out += "\\n";
        }
        else if(ch == '\r') {
            out += "\\r";
        }
        else if(ch == '\t') {
            out += "\\t";
        }
        else if(static_cast<uint8_t>(ch) <= 0x1f) {
            char buf[8];
            snprintf(buf,sizeof buf,"\\u%04x",ch);
            out += buf;
        }
        else if(static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i + 1]) == 0x80
            && static_cast<uint8_t>(value[i + 2]) == 0xa8) {
                out += "\\u2028";
                i += 2;
        }
        else if(static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i + 1]) == 0x80
            && static_cast<uint8_t>(value[i + 2]) == 0xa9) {
                out += "\\u2029";
                i += 2;
        }
        else {
            out += ch;
        }
    }
    out += '"';
}
static void dump(const Json::array &value,std::string &out) {
    bool first_flag = true;
    out += "[";
    for(const auto &v : value) {
        if(!first_flag) out += ",";
        v.dump(out);
        first_flag = false;
    }
    out += "]";
}
static void dump(const Json::object &value,std::string &out) {
    out += "{";
    bool first_flag = true;
    for(const auto &v : value) {
        if(!first_flag) out += ",";
        dump(v.first,out);
        out += ":";
        v.second.dump(out);
        first_flag = false;
    }
    out += "}";
}

void Json::dump(std::string &out) const {
    value_ptr_->dump(out);
}

// ***********************************
//  * Value Wrappers
//  * 
template<Json::Type tag,typename T>
class Value : public JsonValue {
protected:
    // ctors
    explicit Value(const T &value):value_(value) {} // Use () instead of {}
    explicit Value(T &&value):value_(std::move(value)) {} // Use () instead of {}
    // get type
    Json::Type type() const override{
        return tag;
    }
    // comparisons
    bool equals(const JsonValue *other) const override {
        return value_ == static_cast<const Value<tag,T>*>(other)->value_;
    }
    bool less(const JsonValue *other) const override {
        return value_ < static_cast<const Value<tag,T>*>(other)->value_;
    }

    const T value_;
    void dump(std::string &out) const override {::tiny_json::dump(value_,out);}
};

class JsonDouble final : public Value<Json::Type::NUMBER,double> {
    double number_value() const override {return value_;}
    int int_value() const override {return static_cast<int>(value_);}
    bool equals(const JsonValue *other) {return value_ == other->number_value();}
    bool less(const JsonValue *other) {return value_ < other->number_value();}
public:
    explicit JsonDouble(double value):Value(value) {}
};

class JsonInt final : public Value<Json::Type::NUMBER,int> {
    double number_value() const override {return value_;}
    int int_value() const override {return value_;}
    bool equals(const JsonValue *other) {return value_ == other->number_value();}
    bool less(const JsonValue *other) {return value_ < other->number_value();}
public:
    explicit JsonInt(int value):Value(value) {}
};

class JsonBoolean final : public Value<Json::Type::BOOL,bool> {
    bool bool_value() const override {return value_;}
public:
    explicit JsonBoolean(bool value):Value(value) {}
};

class JsonString final : public Value<Json::Type::STRING,std::string> {
    const std::string& string_value() const override {return value_;}
public:
    explicit JsonString(const std::string &value):Value(value) {}
    explicit JsonString(std::string &&value):Value(std::move(value)) {}
};

class JsonArray final : public Value<Json::Type::ARRAY,Json::array> {
    const Json::array& array_items() const override {return value_;}
    const Json& operator[](size_t) const override;
public:
    explicit JsonArray(const Json::array &value):Value(value) {}
    explicit JsonArray(Json::array &&value):Value(std::move(value)) {}
};

class JsonObject final : public Value<Json::Type::OBJECT,Json::object> {
    const Json::object& object_items() const override {return value_;}
    const Json& operator[](const std::string&) const override;
public:
    explicit JsonObject(const Json::object &value):Value(value) {}
    explicit JsonObject(Json::object &&value):Value(std::move(value)) {}
};

class JsonNUll final : public Value<Json::Type::NUL,NullStruct> {
public:
    JsonNUll():Value({}) {}
};

// ***********************************
//  * Statics
//  * 
struct Statics {
    const std::shared_ptr<JsonValue> null = std::make_shared<JsonNUll>();
    const std::shared_ptr<JsonValue> t = std::make_shared<JsonBoolean>(true);
    const std::shared_ptr<JsonValue> f = std::make_shared<JsonBoolean>(false);
    const std::string empty_string;
    const Json::array empty_array;
    const Json::object empty_object;
};

const static Statics& statics() {
    static const Statics s{};
    return s;
}
const static Json& static_null() {
    static const Json json_null;
    return json_null;
}

// ***********************************
//  * Ctors
//  * 

Json::Json() noexcept                   :value_ptr_{statics().null} {}
Json::Json(std::nullptr_t) noexcept     :value_ptr_{statics().null} {}
Json::Json(double value)                :value_ptr_{std::make_shared<JsonDouble>(value)} {}
Json::Json(int value)                   :value_ptr_{std::make_shared<JsonInt>(value)} {}
Json::Json(bool value)                  :value_ptr_{value ? statics().t : statics().f} {}
Json::Json(const std::string &value)    :value_ptr_{std::make_shared<JsonString>(value)} {}
Json::Json(std::string &&value)         :value_ptr_{std::make_shared<JsonString>(std::move(value))} {}
Json::Json(const char *value)           :value_ptr_{std::make_shared<JsonString>(value)} {}
Json::Json(const Json::array &value)    :value_ptr_{std::make_shared<JsonArray>(value)} {}
Json::Json(Json::array &&value)         :value_ptr_{std::make_shared<JsonArray>(std::move(value))} {}
Json::Json(const object &value)         :value_ptr_{std::make_shared<JsonObject>(value)} {}
Json::Json(object &&value)              :value_ptr_{std::make_shared<JsonObject>(std::move(value))} {}

// ***********************************
//  * Accesstors
//  *
Json::Type              Json::type()                                const {return value_ptr_->type();}
double                  Json::number_value()                        const {return value_ptr_->number_value();}
int                     Json::int_value()                           const {return value_ptr_->int_value();}
bool                    Json::bool_value()                          const {return value_ptr_->bool_value();}
const std::string&      Json::string_value()                        const {return value_ptr_->string_value();}
const Json::array&      Json::array_items()                         const {return value_ptr_->array_items();}
const Json::object&     Json::object_items()                        const {return value_ptr_->object_items();}
const Json&             Json::operator[](size_t index)              const {return (*value_ptr_)[index];}
const Json&             Json::operator[](const std::string &key)    const {return (*value_ptr_)[key];}

double                  JsonValue::number_value()                   const {return 0;}
int                     JsonValue::int_value()                      const {return 0;}
bool                    JsonValue::bool_value()                     const {return false;}
const std::string&      JsonValue::string_value()                   const {return statics().empty_string;}
const Json::array&      JsonValue::array_items()                    const {return statics().empty_array;}
const Json::object&     JsonValue::object_items()                   const {return statics().empty_object;}
const Json&             JsonValue::operator[](size_t)               const {return static_null();}
const Json&             JsonValue::operator[](const std::string&)   const {return static_null();}

const Json& JsonArray::operator[](size_t index) const {
    if(index >= value_.size()) throw std::runtime_error("out index");
    return value_[index];
}
const Json& JsonObject::operator[](const std::string &key) const {
    auto iter = value_.find(key);
    return iter != value_.end() ? iter->second : static_null();
}

// ***********************************
//  * Comparetors
//  *
bool Json::operator==(const Json &rhs) const {
    if(value_ptr_ == rhs.value_ptr_) return true;
    if(type() != rhs.type()) return false;
    return value_ptr_->equals(rhs.value_ptr_.get());
}

bool Json::operator<(const Json &rhs) const {
    if(value_ptr_ == rhs.value_ptr_) return false;
    if(type() < rhs.type()) return true;
    return value_ptr_->less(rhs.value_ptr_.get());
}

// ***********************************
//  * Parse
//  *

static inline std::string esc(char c) {
    char buf[12];
    if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f) {
        snprintf(buf, sizeof buf, "'%c' (%d)", c, c);
    } else {
        snprintf(buf, sizeof buf, "(%d)", c);
    }
    return std::string(buf);
}

constexpr static inline bool in_range(long x,long lower,long upper) {
    return (x >= lower && x <= upper);
}
namespace {
// parser
struct JsonParser final {
    const std::string &str;
    std::string &err;
    size_t cur;
    bool failed;
    const JsonParse strategy;

    Json fail(const std::string &msg) {
        return fail(msg,Json());
    }
    template<class T>
    T fail(const std::string &msg,const T ret) {
        failed = true;
        if(!failed) err = std::move(msg);
        return ret;
    }

    void consume_whitespace() {
        while(str[cur] == ' ' 
            || str[cur] == '\r' 
            || str[cur] == '\n' || str[cur] == '\t') cur ++;
    }
    // consume commet like:
    // // ...
    // /* ..
    //    ..
    // */
    bool consume_comment() {
        consume_whitespace();
        bool comment_found = false;
        if(str[cur] == '/') {
            cur ++;
            if(cur >= str.size()) return fail("out of str the range",false);
            if(str[cur] == '/') { // inline comment
                cur ++;
                comment_found = true;
                while(cur < str.size() && str[cur] != '\n') cur ++;
            }
            else if(str[cur] == '*') { // multiline comment
                cur ++;
                if(cur > str.size() - 2) return fail("out of the str range",false); 
                while(!(str[cur] == '*' && str[cur + 1] == '/')) { 
                    cur ++;
                    if(cur > str.size() - 2) return fail("out of the str range",false); 
                }
                cur += 2;
                comment_found = true;
            }
            else
                return fail("malformed comment", false);
        }
        return comment_found;
    }

    void consume_garbage() {
        consume_whitespace();
        if(strategy == JsonParse::COMMENTS) {
            bool comment_founed = false;
            do {
                comment_founed = consume_comment();
                if(failed) return;
                consume_whitespace();
            } while(comment_founed);
        }
    }
    // return the next no whitespace character
    char get_next_token() {
        consume_garbage();
        if(failed) return {};
        if(cur == str.size()) return fail("out of the str range",char{});
        return str[cur ++];
    }

    void encode_utf8(long pt, std::string &out) {
        if (pt < 0)
            return;

        if (pt < 0x80) {
            out += static_cast<char>(pt);
        } else if (pt < 0x800) {
            out += static_cast<char>((pt >> 6) | 0xC0);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        } else if (pt < 0x10000) {
            out += static_cast<char>((pt >> 12) | 0xE0);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        } else {
            out += static_cast<char>((pt >> 18) | 0xF0);
            out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
            out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
            out += static_cast<char>((pt & 0x3F) | 0x80);
        }
    }

    // parse string
    std::string parse_string() {
        std::string out;
        long last_escaped_codepoint = -1;
        while(true) {
            if(cur >= str.size()) return fail("out of the str range",std::string{});
            auto ch = get_next_token();
            if(ch == '"') {
                encode_utf8(last_escaped_codepoint,out);
                return out;
            }
            if(in_range(ch,0x0,0x1f)) {
                return fail("the character is not unescaped",std::string{});
            }
            if(ch != '\\') {
                encode_utf8(last_escaped_codepoint,out);
                last_escaped_codepoint = -1;
                out += ch;
                continue;
            }

            if(cur >= str.size()) return fail("out of the str range",std::string{});
            
            ch = get_next_token();
            if(ch == 'u') {
                auto esp = str.substr(cur,4);
                if(esp.size() < 4) return fail("bad escape" + esp,std::string{});
                for(auto &c : esp) {
                    if(!in_range(c,'a','f') && !in_range(c,'A','F') 
                        && !in_range(c,'0','9')) 
                        return fail("bad escape" + esp,std::string{});
                }
                long codepoint = strtol(esp.data(),nullptr,16);

                // example.
                // "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234"
                if(in_range(last_escaped_codepoint,0xD800,0xDBFF)
                    && in_range(codepoint,0xDC00,0xDFFF)) {
                    encode_utf8(((last_escaped_codepoint - 0xD800) << 10)
                        | (codepoint - 0xDC00) + 0x10000,out);
                    last_escaped_codepoint = -1;    
                }
                else {
                    encode_utf8(last_escaped_codepoint,out);
                    last_escaped_codepoint = codepoint;
                }
                cur += 4;
                continue;
            }
            encode_utf8(last_escaped_codepoint, out);
            last_escaped_codepoint = -1;

            if(ch == 'b') {
                out += '\b';
            } 
            else if(ch == 't') {
                out += '\t';
            }
            else if(ch == 'f') {
                out += '\f';
            }
            else if(ch == 'n') {
                out += '\n';
            }
            else if(ch == 'r') {
                out += '\r';
            }
            else if(ch == '"' || ch == '\\' || ch == '/') {
                out += ch;
            }
            else {
                return fail("invalid escape character " + esc(ch), "");
            }
        }
    }
    Json parse_number() {
        size_t start_pos = cur;

        if (str[cur] == '-')
            cur++;

        // Integer part
        if (str[cur] == '0') {
            cur++;
            if (in_range(str[cur], '0', '9'))
                return fail("leading 0s not permitted in numbers");
        } else if (in_range(str[cur], '1', '9')) {
            cur++;
            while (in_range(str[cur], '0', '9'))
                cur++;
        } else {
            return fail("invalid " + esc(str[cur]) + " in number");
        }

        if (str[cur] != '.' && str[cur] != 'e' && str[cur] != 'E'
                && (cur - start_pos) <= static_cast<size_t>(std::numeric_limits<int>::digits10)) {
            return std::atoi(str.c_str() + start_pos);
        }

        // Decimal part
        if (str[cur] == '.') {
            cur++;
            if (!in_range(str[cur], '0', '9'))
                return fail("at least one digit required in fractional part");

            while (in_range(str[cur], '0', '9'))
                cur++;
        }

        // Exponent part
        if (str[cur] == 'e' || str[cur] == 'E') {
            cur++;

            if (str[cur] == '+' || str[cur] == '-')
                cur++;

            if (!in_range(str[cur], '0', '9'))
                return fail("at least one digit required in exponent");

            while (in_range(str[cur], '0', '9'))
                cur++;
        }

        return std::strtod(str.c_str() + start_pos, nullptr);
    }

    // expect
    Json expect(const std::string &expected,const Json &res) {
        cur --;
        if(str.compare(cur,expected.length(),expected) == 0) {
            cur += expected.length();
            return expected;
        }
        return fail("parse error: expected " + expected + ", got " + str.substr(cur, expected.length())); 
    }

    // parse json

    Json parse_json(int depth) {
        if (depth > MAX_DEPTH) {
            return fail("exceeded maximum nesting depth");
        }

        char ch = get_next_token();
        if (failed)
            return Json();

        if (ch == '-' || in_range(ch,'0','9')) {
            cur --;
            return parse_number();
        }

        if (ch == 't')
            return expect("true", true);

        if (ch == 'f')
            return expect("false", false);

        if (ch == 'n')
            return expect("null", Json());

        if (ch == '"')
            return parse_string();

        if (ch == '{') {
           Json::object data;
            ch = get_next_token();
            if (ch == '}')
                return data;

            while (1) {
                if (ch != '"')
                    return fail("expected '\"' in object, got " + esc(ch));

                std::string key = parse_string();
                if (failed)
                    return Json();

                ch = get_next_token();
                if (ch != ':')
                    return fail("expected ':' in object, got " + esc(ch));

                data[std::move(key)] = parse_json(depth + 1);
                if (failed)
                    return Json();

                ch = get_next_token();
                if (ch == '}')
                    break;
                if (ch != ',')
                    return fail("expected ',' in object, got " + esc(ch));

                ch = get_next_token();
            }
            return data;
        }

        if (ch == '[') {
            Json::array data;
            ch = get_next_token();
            if (ch == ']')
                return data;

            while (1) {
                cur --;
                data.push_back(parse_json(depth + 1));
                if (failed) {
                    std::cout << err;
                    return Json();
                }

                ch = get_next_token();
                if (ch == ']') {
                    break;
                }
                    
                if (ch != ',')
                    return fail("expected ',' in list, got " + esc(ch));

                ch = get_next_token();
                (void)ch;
            }
            return data;
        }

        return fail("expected value, got " + esc(ch));
    }
};
} // namespace

Json Json::parse(const std::string &in,std::string &err,JsonParse strategy) {
    JsonParser parser {in,err,0,false,strategy};
    try {
        auto result = parser.parse_json(0);
        parser.consume_garbage();
        if(parser.failed) {
            std::cout << result.value_ptr_->string_value();
            return Json{};
        }
        if(parser.cur != in.size()) {
            return parser.fail("unexpected trailing " + esc(in[parser.cur]));
        }
        return result;
    }
    catch(std::exception e) {
        std::cout << e.what();
    }
    return {};
}
}