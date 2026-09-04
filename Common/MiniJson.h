#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <utility>

namespace MiniJson
{
    enum class Type
    {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    class JsonValue
    {
    public:
        Type type = Type::Null;
        bool boolVal = false;
        double numVal = 0.0;
        int64_t intVal = 0;
        bool isInteger = false;
        std::string strVal;
        std::vector<JsonValue> arrVal;
        std::map<std::string, JsonValue> objVal;

        JsonValue() : type(Type::Null) {}
        JsonValue(bool b) : type(Type::Boolean), boolVal(b) {}
        JsonValue(int n) : type(Type::Number), numVal(static_cast<double>(n)), intVal(n), isInteger(true) {}
        JsonValue(int64_t n) : type(Type::Number), numVal(static_cast<double>(n)), intVal(n), isInteger(true) {}
        JsonValue(double d) : type(Type::Number), numVal(d), intVal(static_cast<int64_t>(d)), isInteger(false) {}
        JsonValue(float f) : type(Type::Number), numVal(static_cast<double>(f)), intVal(static_cast<int64_t>(f)), isInteger(false) {}
        JsonValue(const char* s) : type(Type::String), strVal(s ? s : "") {}
        JsonValue(const std::string& s) : type(Type::String), strVal(s) {}
        JsonValue(Type t) : type(t) {}

        bool is_null() const { return type == Type::Null; }
        bool is_bool() const { return type == Type::Boolean; }
        bool is_number() const { return type == Type::Number; }
        bool is_string() const { return type == Type::String; }
        bool is_array() const { return type == Type::Array; }
        bool is_object() const { return type == Type::Object; }

        bool to_bool(bool def = false) const
        {
            if (type == Type::Boolean) return boolVal;
            if (type == Type::Number) return numVal != 0.0;
            if (type == Type::String) return strVal == "true" || strVal == "1";
            return def;
        }

        int to_int(int def = 0) const
        {
            if (type == Type::Number) return static_cast<int>(isInteger ? intVal : numVal);
            if (type == Type::String) {
                char* end = nullptr;
                long v = std::strtol(strVal.c_str(), &end, 10);
                if (end != strVal.c_str()) return static_cast<int>(v);
            }
            if (type == Type::Boolean) return boolVal ? 1 : 0;
            return def;
        }

        int64_t to_int64(int64_t def = 0) const
        {
            if (type == Type::Number) return isInteger ? intVal : static_cast<int64_t>(numVal);
            if (type == Type::String) {
                char* end = nullptr;
                long long v = std::strtoll(strVal.c_str(), &end, 10);
                if (end != strVal.c_str()) return static_cast<int64_t>(v);
            }
            if (type == Type::Boolean) return boolVal ? 1 : 0;
            return def;
        }

        float to_float(float def = 0.0f) const
        {
            if (type == Type::Number) return static_cast<float>(numVal);
            if (type == Type::String) {
                char* end = nullptr;
                float v = std::strtof(strVal.c_str(), &end);
                if (end != strVal.c_str()) return v;
            }
            return def;
        }

        double to_double(double def = 0.0) const
        {
            if (type == Type::Number) return numVal;
            if (type == Type::String) {
                char* end = nullptr;
                double v = std::strtod(strVal.c_str(), &end);
                if (end != strVal.c_str()) return v;
            }
            return def;
        }

        std::string to_string(const std::string& def = "") const
        {
            if (type == Type::String) return strVal;
            if (type == Type::Boolean) return boolVal ? "true" : "false";
            if (type == Type::Number) {
                if (isInteger) return std::to_string(intVal);
                std::ostringstream ss;
                ss << numVal;
                return ss.str();
            }
            if (type == Type::Null) return "";
            return def;
        }

        bool as_bool() const { return to_bool(); }
        int as_int() const { return to_int(); }
        std::string as_string() const { return to_string(); }
        const std::vector<JsonValue>& as_array() const { return arrVal; }
        const std::map<std::string, JsonValue>& as_object() const { return objVal; }

        bool has(const std::string& key) const
        {
            if (type != Type::Object) return false;
            return objVal.find(key) != objVal.end();
        }

        const JsonValue& operator[](const std::string& key) const
        {
            static const JsonValue s_Null;
            if (type != Type::Object) return s_Null;
            auto it = objVal.find(key);
            if (it != objVal.end()) return it->second;
            return s_Null;
        }

        JsonValue& operator[](const std::string& key)
        {
            if (type != Type::Object) {
                type = Type::Object;
                objVal.clear();
            }
            return objVal[key];
        }

        const JsonValue& operator[](size_t index) const
        {
            static const JsonValue s_Null;
            if (type != Type::Array || index >= arrVal.size()) return s_Null;
            return arrVal[index];
        }

        JsonValue& operator[](size_t index)
        {
            if (type != Type::Array) {
                type = Type::Array;
                arrVal.clear();
            }
            if (index >= arrVal.size()) arrVal.resize(index + 1);
            return arrVal[index];
        }

        std::string get_string(const std::string& key, const std::string& def = "") const
        {
            if (!has(key)) return def;
            return operator[](key).to_string(def);
        }

        bool get_bool(const std::string& key, bool def = false) const
        {
            if (!has(key)) return def;
            return operator[](key).to_bool(def);
        }

        int get_int(const std::string& key, int def = 0) const
        {
            if (!has(key)) return def;
            return operator[](key).to_int(def);
        }

        int64_t get_int64(const std::string& key, int64_t def = 0) const
        {
            if (!has(key)) return def;
            return operator[](key).to_int64(def);
        }

        float get_float(const std::string& key, float def = 0.0f) const
        {
            if (!has(key)) return def;
            return operator[](key).to_float(def);
        }

        double get_double(const std::string& key, double def = 0.0) const
        {
            if (!has(key)) return def;
            return operator[](key).to_double(def);
        }

        static std::string EscapeString(const std::string& s)
        {
            std::string out;
            out.reserve(s.size() + 16);
            for (unsigned char c : s)
            {
                switch (c)
                {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20)
                    {
                        char hexBuf[8];
                        snprintf(hexBuf, sizeof(hexBuf), "\\u%04x", c);
                        out += hexBuf;
                    }
                    else
                    {
                        out += static_cast<char>(c);
                    }
                    break;
                }
            }
            return out;
        }

        std::string serialize(bool pretty = false, int indent = 0) const
        {
            std::string ind(pretty ? (indent * 2) : 0, ' ');
            std::string indNext(pretty ? ((indent + 1) * 2) : 0, ' ');
            std::string nl = pretty ? "\n" : "";
            std::string sp = pretty ? " " : "";

            switch (type)
            {
            case Type::Null:
                return "null";
            case Type::Boolean:
                return boolVal ? "true" : "false";
            case Type::Number:
                if (isInteger) return std::to_string(intVal);
                {
                    std::ostringstream ss;
                    ss << numVal;
                    return ss.str();
                }
            case Type::String:
                return "\"" + EscapeString(strVal) + "\"";
            case Type::Array:
            {
                if (arrVal.empty()) return "[]";
                std::string s = "[" + nl;
                for (size_t i = 0; i < arrVal.size(); ++i)
                {
                    s += indNext + arrVal[i].serialize(pretty, indent + 1);
                    if (i + 1 < arrVal.size()) s += ",";
                    s += nl;
                }
                s += ind + "]";
                return s;
            }
            case Type::Object:
            {
                if (objVal.empty()) return "{}";
                std::string s = "{" + nl;
                size_t i = 0;
                for (auto it = objVal.begin(); it != objVal.end(); ++it, ++i)
                {
                    s += indNext + "\"" + EscapeString(it->first) + "\":" + sp + it->second.serialize(pretty, indent + 1);
                    if (i + 1 < objVal.size()) s += ",";
                    s += nl;
                }
                s += ind + "}";
                return s;
            }
            }
            return "null";
        }
    };

    class Parser
    {
    public:
        static constexpr size_t MAX_INPUT_SIZE = 10 * 1024 * 1024; // 10MB
        static constexpr int MAX_DEPTH = 64;

        static bool Parse(const std::string& input, JsonValue& outValue, std::string& outError)
        {
            if (input.size() > MAX_INPUT_SIZE)
            {
                outError = "Entrada JSON excede o tamanho maximo suportado (10MB).";
                return false;
            }

            size_t idx = 0;
            SkipWhitespace(input, idx);
            if (idx >= input.size())
            {
                outError = "Documento JSON vazio.";
                return false;
            }

            if (!ParseValue(input, idx, outValue, outError, 0))
            {
                return false;
            }

            SkipWhitespace(input, idx);
            if (idx < input.size())
            {
                outError = "Caracteres inesperados apos final do JSON na posicao " + std::to_string(idx);
                return false;
            }
            return true;
        }

    private:
        static void SkipWhitespace(const std::string& s, size_t& i)
        {
            while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n'))
            {
                ++i;
            }
        }

        static bool ParseValue(const std::string& s, size_t& i, JsonValue& val, std::string& err, int depth = 0)
        {
            if (depth > MAX_DEPTH)
            {
                err = "Profundidade maxima de aninhamento JSON excedida (limite: 64).";
                return false;
            }

            SkipWhitespace(s, i);
            if (i >= s.size())
            {
                err = "Fim inesperado da entrada JSON.";
                return false;
            }

            char c = s[i];
            if (c == '{') return ParseObject(s, i, val, err, depth + 1);
            if (c == '[') return ParseArray(s, i, val, err, depth + 1);
            if (c == '"') return ParseString(s, i, val, err);
            if (c == 't' || c == 'f') return ParseBool(s, i, val, err);
            if (c == 'n') return ParseNull(s, i, val, err);
            if (c == '-' || (c >= '0' && c <= '9')) return ParseNumber(s, i, val, err);

            err = std::string("Token inesperado '") + c + "' na posicao " + std::to_string(i);
            return false;
        }

        static bool ParseObject(const std::string& s, size_t& i, JsonValue& val, std::string& err, int depth)
        {
            val.type = Type::Object;
            val.objVal.clear();
            ++i; // skip '{'

            SkipWhitespace(s, i);
            if (i < s.size() && s[i] == '}')
            {
                ++i;
                return true;
            }

            while (i < s.size())
            {
                SkipWhitespace(s, i);
                if (i >= s.size() || s[i] != '"')
                {
                    err = "Esperada chave de string no objeto na posicao " + std::to_string(i);
                    return false;
                }

                JsonValue keyVal;
                if (!ParseString(s, i, keyVal, err)) return false;

                SkipWhitespace(s, i);
                if (i >= s.size() || s[i] != ':')
                {
                    err = "Esperado ':' apos chave no objeto na posicao " + std::to_string(i);
                    return false;
                }
                ++i; // skip ':'

                JsonValue memberVal;
                if (!ParseValue(s, i, memberVal, err, depth + 1)) return false;

                val.objVal[keyVal.strVal] = std::move(memberVal);

                SkipWhitespace(s, i);
                if (i >= s.size())
                {
                    err = "Objeto JSON nao finalizado (falta '}').";
                    return false;
                }

                if (s[i] == ',')
                {
                    ++i;
                    continue;
                }
                else if (s[i] == '}')
                {
                    ++i;
                    return true;
                }
                else
                {
                    err = std::string("Esperado ',' ou '}' no objeto na posicao ") + std::to_string(i);
                    return false;
                }
            }

            err = "Fim prematuro do objeto JSON.";
            return false;
        }

        static bool ParseArray(const std::string& s, size_t& i, JsonValue& val, std::string& err, int depth)
        {
            val.type = Type::Array;
            val.arrVal.clear();
            ++i; // skip '['

            SkipWhitespace(s, i);
            if (i < s.size() && s[i] == ']')
            {
                ++i;
                return true;
            }

            while (i < s.size())
            {
                JsonValue elem;
                if (!ParseValue(s, i, elem, err, depth + 1)) return false;

                val.arrVal.push_back(std::move(elem));

                SkipWhitespace(s, i);
                if (i >= s.size())
                {
                    err = "Array JSON nao finalizado (falta ']').";
                    return false;
                }

                if (s[i] == ',')
                {
                    ++i;
                    continue;
                }
                else if (s[i] == ']')
                {
                    ++i;
                    return true;
                }
                else
                {
                    err = std::string("Esperado ',' ou ']' no array na posicao ") + std::to_string(i);
                    return false;
                }
            }

            err = "Fim prematuro do array JSON.";
            return false;
        }

        static bool ParseString(const std::string& s, size_t& i, JsonValue& val, std::string& err)
        {
            val.type = Type::String;
            val.strVal.clear();
            ++i; // skip initial '"'

            while (i < s.size())
            {
                char c = s[i++];
                if (c == '"')
                {
                    return true;
                }
                if (c == '\\')
                {
                    if (i >= s.size())
                    {
                        err = "Escape inacabado no final da string.";
                        return false;
                    }
                    char esc = s[i++];
                    switch (esc)
                    {
                    case '"':  val.strVal += '"'; break;
                    case '\\': val.strVal += '\\'; break;
                    case '/':  val.strVal += '/'; break;
                    case 'b':  val.strVal += '\b'; break;
                    case 'f':  val.strVal += '\f'; break;
                    case 'n':  val.strVal += '\n'; break;
                    case 'r':  val.strVal += '\r'; break;
                    case 't':  val.strVal += '\t'; break;
                    case 'u':
                    {
                        if (i + 4 > s.size())
                        {
                            err = "Sequencia unicode \\u incompleta.";
                            return false;
                        }
                        std::string hexStr = s.substr(i, 4);
                        i += 4;
                        char* end = nullptr;
                        unsigned long code = std::strtoul(hexStr.c_str(), &end, 16);
                        if (code < 0x80)
                        {
                            val.strVal += static_cast<char>(code);
                        }
                        else if (code < 0x800)
                        {
                            val.strVal += static_cast<char>(0xC0 | ((code >> 6) & 0x1F));
                            val.strVal += static_cast<char>(0x80 | (code & 0x3F));
                        }
                        else
                        {
                            val.strVal += static_cast<char>(0xE0 | ((code >> 12) & 0x0F));
                            val.strVal += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                            val.strVal += static_cast<char>(0x80 | (code & 0x3F));
                        }
                        break;
                    }
                    default:
                        val.strVal += esc;
                        break;
                    }
                }
                else
                {
                    val.strVal += c;
                }
            }

            err = "String JSON nao finalizada com aspas.";
            return false;
        }

        static bool ParseNumber(const std::string& s, size_t& i, JsonValue& val, std::string& err)
        {
            size_t start = i;
            if (s[i] == '-') ++i;

            if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i])))
            {
                err = "Digito esperado apos sinal negativo na posicao " + std::to_string(i);
                return false;
            }

            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;

            bool isFloat = false;
            if (i < s.size() && s[i] == '.')
            {
                isFloat = true;
                ++i;
                if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i])))
                {
                    err = "Digito esperado apos ponto decimal na posicao " + std::to_string(i);
                    return false;
                }
                while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
            }

            if (i < s.size() && (s[i] == 'e' || s[i] == 'E'))
            {
                isFloat = true;
                ++i;
                if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
                if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i])))
                {
                    err = "Digito esperado no expoente na posicao " + std::to_string(i);
                    return false;
                }
                while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
            }

            std::string numStr = s.substr(start, i - start);
            char* end = nullptr;
            val.type = Type::Number;
            val.isInteger = !isFloat;
            if (isFloat)
            {
                val.numVal = std::strtod(numStr.c_str(), &end);
                val.intVal = static_cast<int64_t>(val.numVal);
            }
            else
            {
                val.intVal = std::strtoll(numStr.c_str(), &end, 10);
                val.numVal = static_cast<double>(val.intVal);
            }
            return true;
        }

        static bool ParseBool(const std::string& s, size_t& i, JsonValue& val, std::string& err)
        {
            if (s.compare(i, 4, "true") == 0)
            {
                val = JsonValue(true);
                i += 4;
                return true;
            }
            if (s.compare(i, 5, "false") == 0)
            {
                val = JsonValue(false);
                i += 5;
                return true;
            }
            err = "Esperado 'true' ou 'false' na posicao " + std::to_string(i);
            return false;
        }

        static bool ParseNull(const std::string& s, size_t& i, JsonValue& val, std::string& err)
        {
            if (s.compare(i, 4, "null") == 0)
            {
                val = JsonValue(Type::Null);
                i += 4;
                return true;
            }
            err = "Esperado 'null' na posicao " + std::to_string(i);
            return false;
        }
    };

    using Value = JsonValue;

    inline bool Parse(const std::string& input, JsonValue& outValue, std::string& outError)
    {
        return Parser::Parse(input, outValue, outError);
    }

    inline std::string Serialize(const JsonValue& val, bool pretty = false)
    {
        return val.serialize(pretty);
    }
}
