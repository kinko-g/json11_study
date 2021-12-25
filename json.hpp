#pragma once
#include <vector>   // for vector
#include <map>      // for map
#include <string>   // for string
#include <memory>   // for shared_ptr
#include <type_traits>  // for is_constructible
#include <initializer_list>
#include <iostream>
namespace tiny_json {

enum JsonParse {
    STANDARD, COMMENTS
};

class JsonValue;

class Json final {
public:
    enum Type {
        NUL, NUMBER, BOOL, STRING, ARRAY, OBJECT
    };
    using array = std::vector<Json>;
    using object = std::map<std::string,Json>;
    Json() noexcept;
    Json(std::nullptr_t) noexcept;
    Json(double);
    Json(int);
    Json(bool);
    Json(const std::string&);
    Json(std::string&&);
    Json(const char*);
    Json(const array&);
    Json(array&&);
    Json(const object&);
    Json(object&&);
    // for vector-like
    template<class T,typename 
        std::enable_if<
            std::is_constructible<Json,decltype(*std::declval<T>().begin())>::value,
        int>::type = 0>
    Json(T &t):Json(array{t.begin(),t.end()}) {}
    // for map-like
    template<class T,typename 
        std::enable_if<
            std::is_constructible<decltype(std::declval<T>().begin()->first)>::value &&
            std::is_constructible<decltype(std::declval<T>().begin()->second)>::value
        ,int>::type = 0>
    Json(T &t):Json(object{t.begin(),t.end()}) {}
    Json(void*) = delete;
    
    Type type() const;
    bool is_null() {return type() == NUL;}
    bool is_number() {return type() == NUMBER;}
    bool is_bool() {return type() == BOOL;}
    bool is_string() {return type() == STRING;}
    bool is_array() {return type() == ARRAY;}
    bool is_object() {return type() == OBJECT;}

    double number_value() const;
    int int_value() const;
    bool bool_value() const;
    const std::string& string_value() const;
    const array& array_items() const;
    const object& object_items() const;
    const Json& operator[](size_t) const;
    const Json& operator[](const std::string&) const;
    
    // serialize
    void dump(std::string&) const;
    std::string dump() const {
        std::string out;
        dump(out);
        return out;
    }

    // parse. if parse fails , return Json() and assgin an error message to err.
    static Json parse(
        const std::string &in,
        std::string &err,
        JsonParse strategy = JsonParse::STANDARD);
    static Json parse(
        const char *in,
        std::string &err,
        JsonParse strategy = JsonParse::STANDARD) {
            if(in) {
                return parse(std::string(in),err,strategy);
            }
            err = "null input";
            return nullptr;
        }
    
    // // parse multiple objects,concatenated or sparated by whitespace
    // static std::vector<Json> parse_multi(
    //     const std::string &in,
    //     std::string::size_type &parse_stop_pos,
    //     std::string &err,
    //     JsonParse strategy = JsonParse::STANDARD);

    // static inline std::vector<Json> parse_multi(
    //     const std::string &in,
    //     std::string &err,
    //     JsonParse strategy = JsonParse::STANDARD) {
    //         std::string::size_type parse_stop_pos;
    //         return parse_multi(in,parse_stop_pos,err,strategy);
    // }
    
    bool operator==(const Json &rhs) const;
    bool operator< (const Json &rhs) const;
    bool operator!=(const Json &rhs) const {return !(*this == rhs);}
    bool operator<=(const Json &rhs) const {return !(rhs < *this);}
    bool operator>=(const Json &rhs) const {return !(*this < rhs);}
    bool operator> (const Json &rhs) const {return rhs < *this;}

    // using shape = std::initializer_list<std::pair<std::string,Type>>;
    // bool has_shape(const shape &types,std::string &err) const;
private:
    std::shared_ptr<JsonValue> value_ptr_;
};

class JsonValue {
protected:
    friend class Json;
    friend class JsonInt;
    friend class JsonDouble;

    virtual Json::Type type() const = 0;
    virtual bool equals(const JsonValue*) const = 0;
    virtual bool less(const JsonValue*) const = 0;
    virtual void dump(std::string&) const = 0;
    virtual double number_value() const;
    virtual int int_value() const;
    virtual bool bool_value() const;
    virtual const std::string& string_value() const;
    virtual const Json::array& array_items() const;
    virtual const Json::object& object_items() const;
    // operator[] for array
    virtual const Json& operator[](size_t) const;
    // operator[] for object 
    virtual const Json& operator[](const std::string&) const;
    virtual ~JsonValue() {}
};
Json parse(const std::string &in,const std::string &err);
} // namespace tiny_json