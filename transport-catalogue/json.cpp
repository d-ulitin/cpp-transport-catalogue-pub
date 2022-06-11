#include "json.h"

#include <cassert>

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;

    char c = 0;
    while (input >> c) {
        if (c == ']') {
            break;
        } else if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (!input)
        throw ParsingError("invalid array");

    return Node(move(result));
}

string Escape(char c) {
    switch (c) {
    case '\n':
        return "\\n"s;
    case '\r':
        return "\\r"s;
    case '"':
        return "\\\""s;
    // case '\t':
    //     return "\\t"s;
    case '\\':
        return "\\\\"s;
    default:
        return string(1, c);
    }
}

char Unescape(char escaped) {
    switch (escaped) {
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case '"':
        return '"';
    case 't':
        return '\t';
    case '\\':
        return '\\';
    default:
        throw ParsingError("invalid escape character");
    }
}

Node LoadString(istream& input) {
    string str;
    char c = 0;
    while(input.get(c)) {
        if (c == '"') {
            break;
        } else if (c == '\\') {
            char escaped;
            if (input.get(escaped)) {
                str.push_back(Unescape(escaped));
            } else {
                throw ParsingError("invalid escape character");
            }
        } else if (c == '\n' || c == '\r') {
            throw ParsingError("unexpected end of line");
        } else {
            str.push_back(c);
        }
    }
    if (!input)
        throw ParsingError("invalid string");
    return Node(move(str));
}

Node LoadDict(istream& input) {
    Dict result;

    char c = 0;
    while (input >> c) {
        if (c == '}') {
            break;
        } else if (c == ',') {
            input >> c;
            if (c != '"')
                throw ParsingError("dict key must be string");
        }

        string key = LoadString(input).AsString();
        input >> c;
        if (c != ':')
            throw ParsingError("dict key and value must be separated by ':'");
        result.insert({move(key), LoadNode(input)});
    }
    if (!input)
        throw ParsingError("invalid map");

    return Node(move(result));
}

using Number = std::variant<int, double>;

Number LoadNumber(istream& input) {
    string num;

    auto read_char = [&input, &num](char char_prm) -> int {
        char c;
        if (input.get(c)) {
            if (c == char_prm) {
                num.push_back(c);
                return 1;
            } else {
                input.unget();
                return 0;
            }
        }
        return 0;
    };

    auto read_1_9 = [&input, &num]() -> int {
        char c;
        if (input.get(c)) {
            if (c >= '1' && c <= '9') {
                num.push_back(c);
                return 1;
            } else {
                input.unget();
                return 0;
            }
        }
        return 0;
    };

    auto read_digits = [&input, &num]() -> int {
        char c;
        int n = 0;
        while (input.get(c)) {
            if (isdigit(c)) {
                num.push_back(c);
                ++n;
            } else {
                input.unget();
                break;
            }
        }
        return n;
    };

    bool is_float = false;

    read_char('-');

    if (read_char('0') == 0) {
        if (read_1_9() > 0)
            read_digits();
        else
            throw ParsingError("invalid number");
    }

    if (read_char('.')) {
        is_float = true;
        read_digits();
    }
    
    if (read_char('e') || read_char('E')) {
        is_float = true;
        read_char('+') || read_char('-');
        if (read_digits() == 0)
            throw ParsingError("invalid number");
    }

    while (true)
    {
        if (is_float) {
            double parsed_double;
            try {
                parsed_double = stod(num);
            } catch (...) {
                throw ParsingError("invalid float");
            }
            return parsed_double;
        } else {
            int parsed_int;
            try {
                parsed_int = stoi(num);
            } catch (...) {
                is_float = true;
                continue;
            }
            return parsed_int;
        }
    }
}

Node LoadNode(istream& input) {
    auto alpha = [&input]() {
        string s;
        char c;
        while (input.get(c)) {
            if (isalpha(c))
                s.push_back(c);
            else {
                input.unget();
                break;
            }
        }
        return s;
    };

    char c;
    input >> c;
    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == '-' || isdigit(c)) {
        input.putback(c);
        Number num = LoadNumber(input);
        if (holds_alternative<int>(num))
            return Node(get<int>(num));
        else if (holds_alternative<double>(num))
            return Node(get<double>(num));
        else
            assert(false);
    } else {
        input.unget();
    }

    string s = alpha();
    if (s == true_value)
        return Node(true);
    else if (s == false_value)
        return Node(false);
    else if (s == null_value)
        return Node(nullptr);

    throw ParsingError("json parsing error: "s + s);
}

}  // namespace


Node::Node()
    : value_(nullptr) {
}

Node::Node(nullptr_t)
    : value_(nullptr) {
}

Node::Node(int value)
    : value_(value) {
}

Node::Node(double value)
    : value_(value) {
}

Node::Node(const string& value)
    : value_(value) {
}

Node::Node(string&& value)
    : value_(move(value)) {
}

Node::Node(bool value)
    : value_(value) {
}

Node::Node(const Array& value)
    : value_(value) {
}

Node::Node(Array&& value)
    : value_(move(value)) {
}

Node::Node(const Dict& value)
    : value_(value) {
}

Node::Node(Dict&& value)
    : value_(move(value)) {
}

Node::ValueType& Node::Value() {
    return value_;
}

const Node::ValueType& Node::Value() const {
    return value_;
}

int Node::AsInt() const {
    if (holds_alternative<int>(value_))
        return get<int>(value_);
    else
        throw logic_error("node value must be int");
}

bool Node::AsBool() const {
    if (holds_alternative<bool>(value_))
        return get<bool>(value_);
    else
        throw logic_error("node value must be bool");
}

double Node::AsDouble() const {
    // Возвращает значение типа double, если внутри хранится double либо int. В последнем случае возвращается приведённое в double значение.
    if (holds_alternative<double>(value_))
        return get<double>(value_);
    else if(holds_alternative<int>(value_))
        return get<int>(value_);
    else
        throw logic_error("node value must be double or int");
}

const std::string& Node::AsString() const {
    if (holds_alternative<string>(value_))
        return get<string>(value_);
    else
        throw logic_error("node value must be string");
}

const Array& Node::AsArray() const {
    if (holds_alternative<Array>(value_))
        return get<Array>(value_);
    else
        throw logic_error("node value must be array");
}

const Dict& Node::AsMap() const {
    if (holds_alternative<Dict>(value_))
        return get<Dict>(value_);
    else
        throw logic_error("node value must be map");
}

bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}

bool Node::IsDouble() const {
    return IsInt() || IsPureDouble();
}

bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}

bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}

bool Node::IsString() const {
    return holds_alternative<string>(value_);
}

bool Node::IsNull() const {
    return holds_alternative<nullptr_t>(value_);
}

bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}

bool Node::IsDict() const {
    return holds_alternative<Dict>(value_);
}

bool operator==(const Node& v, const Node& w) {
    return v.Value() == w.Value();
}

bool operator!=(const Node& v, const Node& w) {
    return !(v == w);
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool operator==(const Document& v, const Document& w) {
    return v.GetRoot() == w.GetRoot();
}

bool operator!=(const Document& v, const Document& w) {
    return !(v == w);
}


Document Load(istream& input) {
    return Document{LoadNode(input)};
}

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

void PrintNode(const Node& node, PrintContext& context);

template <typename T>
void PrintValue(const T& value, PrintContext& context) {
    context.out << value;
}

template <>
void PrintValue(const nullptr_t&, PrintContext& context) {
    context.out << null_value;
}

template <>
void PrintValue(const bool& value, PrintContext& context) {
    if (value)
        context.out << true_value;
    else
        context.out << false_value;
}

template <>
void PrintValue(const string& value, PrintContext& context) {
    context.out << '"';
    for (char c : value)
        context.out << Escape(c);
    context.out << '"';
}

template <>
void PrintValue(const Array& value, PrintContext& context) {
    if (value.empty()) {
        context.out << "[]";
        return;
    }
    context.out << "[";
    auto indented = context.Indented();
    bool first = true;
    for (const auto& node : value) {
        if (!first) {
            indented.out << ","s;
        }
        first = false;
        indented.out << '\n';
        indented.PrintIndent();
        PrintNode(node, indented);
    }
    context.out << '\n';
    context.PrintIndent();
    context.out << ']';
}

template <>
void PrintValue(const Dict& value, PrintContext& context) {
    if (value.empty()) {
        context.out << "{}";
        return;
    }
    context.out << "{";
    auto indented = context.Indented();
    bool first = true;
    for (const auto& node : value) {
        if (!first) {
            indented.out << ",";
        }
        first = false;
        indented.out << '\n';
        indented.PrintIndent();
        indented.out << "\"" << node.first << "\": ";
        PrintNode(node.second, indented);
    }
    context.out << '\n';
    context.PrintIndent();
    context.out << '}';
}

void PrintNode(const Node& node, PrintContext& context) {
    visit(
        [&context](const auto& value) { PrintValue(value, context); },
        node.Value()
    );
}

void Print(const Document& doc, std::ostream& output) {
    PrintContext context{output};
    PrintNode(doc.GetRoot(), context);
}

}  // namespace json