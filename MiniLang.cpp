#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <variant>
#include <optional>
#include <utility>
#include <functional>
#include <fstream>
#include <chrono>
#include <exception>
#include <set>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// ===================================================================
// 1. 异常类型
// ===================================================================
class RuntimeError : public std::runtime_error {
public:
    const int line;
    RuntimeError(int ln, const std::string& message)
        : std::runtime_error("Line " + std::to_string(ln) + ": " + message), line(ln) {}
};

class BreakSignal : public std::exception {};
class ContinueSignal : public std::exception {};

// ===================================================================
// 2. 前置声明与辅助结构
// ===================================================================
enum class TokenType;
class Value;
class Environment;
class Callable;
class ClassValue;
class MutableObject;
struct BlockStmt;
struct Stmt;
struct Expr;
struct FuncStmt;
struct MemberAccessExpr;
struct DictLiteralExpr;
struct VarExpr;

// Wrappers to break recursive dependency in std::variant
struct ArrayValue;
struct DictValue;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using StmtList = std::vector<StmtPtr>;

struct ParamInfo {
    std::string name;
    std::optional<TokenType> type;
};

TokenType get_value_type_token(const Value& val);
bool check_type(TokenType expected, const Value& val);


// ===================================================================
// 3. 词法分析器 (Lexer)
// ===================================================================
enum class TokenType {
    ID, INT_LITERAL, FLOAT_LITERAL, STR,
    TRUE, FALSE,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQ, NE, LT, LE, GT, GE,
    AND, OR, NOT,
    ASSIGN,
    IF, ELSE, WHILE, FOR,
    FUNC, RETURN, VAR, BREAK, CONTINUE,
    CLASS, THIS, SUPER, EXTENDS,
    LBRACE, RBRACE, LPAREN, RPAREN, COMMA, LBRACKET, RBRACKET, COLON, DOT,
    SEMICOLON,
    INT, FLOAT, BOOL, STRING, ARRAY, DICT, OBJECT,
    END
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    Token(TokenType t, std::string l, int ln) : type(t), lexeme(std::move(l)), line(ln) {}
};

class Lexer {
    std::string source;
    size_t start = 0;
    size_t current = 0;
    int line = 1;
public:
    explicit Lexer(std::string_view src) : line(1) {
        size_t first_char_pos = 0;
        while (first_char_pos < src.length() && std::isspace(static_cast<unsigned char>(src[first_char_pos]))) {
            if (src[first_char_pos] == '\n') line++;
            first_char_pos++;
        }
        this->source = src.substr(first_char_pos);
    }
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (!isAtEnd()) {
            start = current;
            scanToken(tokens);
        }
        tokens.emplace_back(TokenType::END, "", line);
        return tokens;
    }
private:
    bool isAtEnd() const { return current >= source.size(); }
    char advance() { return source[current++]; }
    char peek() const { return current < source.size() ? source[current] : '\0'; }
    char peekNext() const {
        if (current + 1 >= source.size()) return '\0';
        return source[current + 1];
    }
    bool match(char expected) {
        if (isAtEnd() || source[current] != expected) return false;
        current++;
        return true;
    }
    void addToken(TokenType type, std::vector<Token>& tokens) {
        tokens.emplace_back(type, source.substr(start, current - start), line);
    }
    void scanToken(std::vector<Token>& tokens) {
        char c = advance();
        switch (c) {
            case ' ': case '\r': case '\t': break;
            case '\n': line++; break;
            case '#': while (peek() != '\n' && !isAtEnd()) advance(); break;
            case '(': addToken(TokenType::LPAREN, tokens); break;
            case ')': addToken(TokenType::RPAREN, tokens); break;
            case '{': addToken(TokenType::LBRACE, tokens); break;
            case '}': addToken(TokenType::RBRACE, tokens); break;
            case '[': addToken(TokenType::LBRACKET, tokens); break;
            case ']': addToken(TokenType::RBRACKET, tokens); break;
            case ',': addToken(TokenType::COMMA, tokens); break;
            case ':': addToken(TokenType::COLON, tokens); break;
            case '.': addToken(TokenType::DOT, tokens); break;
            case ';': addToken(TokenType::SEMICOLON, tokens); break;
            case '+': addToken(TokenType::PLUS, tokens); break;
            case '-': addToken(TokenType::MINUS, tokens); break;
            case '*': addToken(TokenType::STAR, tokens); break;
            case '/':
                 if (match('/')) {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else if (match('*')) {
                    while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) {
                        if (peek() == '\n') line++;
                        advance();
                    }
                    if (isAtEnd()) {
                        throw std::runtime_error("Unterminated block comment starting at line " + std::to_string(line));
                    }
                    advance();
                    advance();
                } else {
                    addToken(TokenType::SLASH, tokens);
                }
                break;
            case '%': addToken(TokenType::PERCENT, tokens); break;
            case '=': addToken(match('=') ? TokenType::EQ : TokenType::ASSIGN, tokens); break;
            case '!': addToken(match('=') ? TokenType::NE : TokenType::NOT, tokens); break;
            case '<': addToken(match('=') ? TokenType::LE : TokenType::LT, tokens); break;
            case '>': addToken(match('=') ? TokenType::GE : TokenType::GT, tokens); break;
            case '&': if (match('&')) addToken(TokenType::AND, tokens); break;
            case '|': if (match('|')) addToken(TokenType::OR, tokens); break;
            case '"':
            case '\'':
                stringLiteral(tokens, c);
                break;
            default:
                if (std::isdigit(c)) number(tokens);
                else if (std::isalpha(c) || c == '_') identifier(tokens);
                else throw std::runtime_error("Unexpected character '" + std::string(1, c) + "' at line " + std::to_string(line));
        }
    }
    void stringLiteral(std::vector<Token>& tokens, char quote_type) {
        std::string value;
        while (peek() != quote_type && !isAtEnd()) {
            char c = peek();
            if (c == '\\') {
                advance();
                if(isAtEnd()) break;
                switch(peek()){
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case '\\': value += '\\'; break;
                    case '\'': value += '\''; break;
                    case '"': value += '"'; break;
                    default: value += '\\'; value += peek(); break;
                }
                advance();
            } else {
                if (c == '\n') line++;
                value += c;
                advance();
            }
        }
        if (isAtEnd()) throw std::runtime_error("Unterminated string at line " + std::to_string(line));
        advance();
        tokens.emplace_back(TokenType::STR, value, line);
    }
    void number(std::vector<Token>& tokens) {
        while (std::isdigit(peek())) advance();
        if (peek() == '.' && std::isdigit(peekNext())) {
            advance();
            while (std::isdigit(peek())) advance();
            addToken(TokenType::FLOAT_LITERAL, tokens);
        } else {
            addToken(TokenType::INT_LITERAL, tokens);
        }
    }
    void identifier(std::vector<Token>& tokens) {
        while (std::isalnum(peek()) || peek() == '_') advance();
        std::string text = source.substr(start, current - start);
        static const std::unordered_map<std::string, TokenType> keywords = {
            {"if", TokenType::IF}, {"else", TokenType::ELSE},
            {"while", TokenType::WHILE}, {"for", TokenType::FOR},
            {"true", TokenType::TRUE}, {"false", TokenType::FALSE},
            {"func", TokenType::FUNC}, {"return", TokenType::RETURN}, {"var", TokenType::VAR},
            {"break", TokenType::BREAK}, {"continue", TokenType::CONTINUE},
            {"class", TokenType::CLASS}, {"this", TokenType::THIS}, {"super", TokenType::SUPER}, {"extends", TokenType::EXTENDS},
            {"int", TokenType::INT}, {"float", TokenType::FLOAT},
            {"bool", TokenType::BOOL}, {"string", TokenType::STRING}, {"array", TokenType::ARRAY},
            {"dict", TokenType::DICT}, {"object", TokenType::OBJECT}
        };
        if (auto it = keywords.find(text); it != keywords.end()) {
            addToken(it->second, tokens);
        } else {
            addToken(TokenType::ID, tokens);
        }
    }
};

// ===================================================================
// 4. 动态类型系统
// ===================================================================

class StringData {
    using SharedString = std::shared_ptr<std::string>;
    SharedString data;

    static std::unordered_map<std::string, SharedString> intern_pool;

    explicit StringData(SharedString s) : data(std::move(s)) {}

public:
    explicit StringData(const std::string& s) : data(std::make_shared<std::string>(s)) {}
    explicit StringData(const char* s) : data(std::make_shared<std::string>(s)) {}

    static StringData from_literal(const std::string& literal) {
        if (auto it = intern_pool.find(literal); it != intern_pool.end()) {
            return StringData(it->second);
        }
        auto new_shared_str = std::make_shared<std::string>(literal);
        intern_pool[literal] = new_shared_str;
        return StringData(new_shared_str);
    }
    
    const std::string& get() const { return *data; }

    std::string& writeable() {
        if (data.use_count() > 1) {
            data = std::make_shared<std::string>(*data);
        }
        return *data;
    }

    bool operator==(const StringData& other) const {
        return data == other.data || (data && other.data && *data == *other.data);
    }
    bool operator!=(const StringData& other) const { return !(*this == other); }
};
std::unordered_map<std::string, std::shared_ptr<std::string>> StringData::intern_pool;


class Callable {
public:
    virtual ~Callable() = default;
    virtual int arity() const = 0;
    virtual Value call(const std::vector<Value>& args) = 0;
    virtual std::string toString() const = 0;
};

class Value {
public:
    using FuncType = std::shared_ptr<Callable>;
    using MutableObjectType = std::shared_ptr<MutableObject>;
    using ArrayType = std::shared_ptr<ArrayValue>;
    using DictType = std::shared_ptr<DictValue>;
    using VariantType = std::variant<std::monostate, int, double, bool, StringData, FuncType, ArrayType, DictType, MutableObjectType>;
private:
    VariantType data;
public:
    Value() : data(std::monostate{}) {}
    Value(int v) : data(v) {}
    Value(double v) : data(v) {}
    Value(bool v) : data(v) {}
    Value(const std::string& v) : data(StringData(v)) {}
    Value(const char* v) : data(StringData(v)) {}
    Value(StringData v) : data(std::move(v)) {}
    Value(FuncType v) : data(std::move(v)) {}
    Value(ArrayType v) : data(std::move(v)) {}
    Value(DictType v) : data(std::move(v)) {}
    Value(MutableObjectType v) : data(std::move(v)) {}

    template <typename T> bool is() const { return std::holds_alternative<T>(data); }
    template <typename T> const T& as() const {
        if (auto* val = std::get_if<T>(&data)) return *val;
        throw std::runtime_error("Invalid type cast in Value::as()");
    }
    template <typename T> T& as() {
        if (auto* val = std::get_if<T>(&data)) return *val;
        throw std::runtime_error("Invalid type cast in Value::as()");
    }

    bool toBool() const;
    std::string toString() const;
    const VariantType& getVariant() const { return data; }

    bool operator==(const Value& other) const {
        return data == other.data;
    }
    bool operator!=(const Value& other) const {
        return data != other.data;
    }
};

struct ArrayValue {
    std::vector<Value> elements;
    bool operator==(const ArrayValue& other) const {
        return elements == other.elements;
    }
};

struct DictValue {
    std::unordered_map<std::string, Value> pairs;
    bool operator==(const DictValue& other) const {
        return pairs == other.pairs;
    }
};

class MutableObject : public std::enable_shared_from_this<MutableObject> {
public:
    std::unordered_map<std::string, Value> fields;
    std::shared_ptr<MutableObject> parent;
    std::shared_ptr<ClassValue> klass;

    explicit MutableObject(std::shared_ptr<MutableObject> p = nullptr) : parent(std::move(p)), klass(nullptr) {}

    Value get(const std::string& name) {
        if (auto it = fields.find(name); it != fields.end()) {
            return it->second;
        }
        if (parent) {
            return parent->get(name);
        }
        throw std::runtime_error("Undefined property '" + name + "'.");
    }

    void set(const std::string& name, const Value& value) {
        fields[name] = value;
    }

    bool has(const std::string& name) {
        if (fields.count(name)) {
            return true;
        }
        if (parent) {
            return parent->has(name);
        }
        return false;
    }
    std::string toString() const;
};

class FunctionValue : public Callable {
public:
    std::vector<ParamInfo> params;
    const BlockStmt* body;
    std::shared_ptr<Environment> closure;
    bool is_initializer = false;

    FunctionValue(std::vector<ParamInfo> p, const BlockStmt* b, std::shared_ptr<Environment> c, bool is_init = false)
        : params(std::move(p)), body(b), closure(std::move(c)), is_initializer(is_init) {}

    int arity() const override { return static_cast<int>(params.size()); }
    Value call(const std::vector<Value>& args) override;
    std::string toString() const override { return "<function>"; }

    std::shared_ptr<FunctionValue> bind(std::shared_ptr<MutableObject> instance);
};

class ClassValue : public Callable, public std::enable_shared_from_this<ClassValue> {
public:
    std::string name;
    std::shared_ptr<ClassValue> superclass;
    std::shared_ptr<MutableObject> prototype;
    std::optional<std::shared_ptr<FunctionValue>> initializer;

    ClassValue(std::string n, std::shared_ptr<ClassValue> sc)
        : name(std::move(n)), superclass(std::move(sc)) {
        prototype = std::make_shared<MutableObject>(superclass ? superclass->prototype : nullptr);
    }

    int arity() const override {
        return initializer.has_value() ? (*initializer)->arity() : 0;
    }
    Value call(const std::vector<Value>& args) override;
    std::string toString() const override { return "<class " + name + ">"; }

    std::shared_ptr<FunctionValue> findMethod(const std::string& methodName) {
        Value methodVal;
        try {
            methodVal = this->prototype->get(methodName);
        } catch (const std::runtime_error&) {
            return nullptr;
        }

        if (methodVal.is<Value::FuncType>()) {
             auto func = methodVal.as<Value::FuncType>();
             return std::dynamic_pointer_cast<FunctionValue>(func);
        }
        return nullptr;
    }
};


class NativeFunction : public Callable {
public:
    using NativeFn = std::function<Value(const std::vector<Value>&)>;
private:
    NativeFn function;
    int _arity;
    std::string name;
public:
    NativeFunction(NativeFn fn, int arity, std::string name)
        : function(std::move(fn)), _arity(arity), name(std::move(name)) {}
    int arity() const override { return _arity; }
    Value call(const std::vector<Value>& args) override { return function(args); }
    std::string toString() const override { return "<native function: " + name + ">"; }
};

struct VariableInfo {
    Value value;
    std::optional<TokenType> static_type;
};

class Environment : public std::enable_shared_from_this<Environment> {
    std::unordered_map<std::string, VariableInfo> variables;
    std::shared_ptr<Environment> parent;
public:
    Environment() : parent(nullptr) {}
    explicit Environment(std::shared_ptr<Environment> p) : parent(std::move(p)) {}

    void define(const std::string& name, Value value, std::optional<TokenType> type) {
        if (type.has_value()) {
            if (!check_type(*type, value)) {
                throw std::runtime_error("Initializer type mismatch for variable '" + name + "'.");
            }
        }
        variables[name] = { std::move(value), type };
    }

    Value& get(const std::string& name) {
        if (auto it = variables.find(name); it != variables.end()) return it->second.value;
        if (parent) return parent->get(name);
        throw std::runtime_error("Undefined variable: " + name);
    }

    Value& get_at(int distance, const std::string& name) {
        std::shared_ptr<Environment> ancestor = shared_from_this();
        for (int i = 0; i < distance; i++) {
            ancestor = ancestor->parent;
        }
        auto it = ancestor->variables.find(name);
        if (it != ancestor->variables.end()) {
            return it->second.value;
        }
        throw std::runtime_error("Internal error: cannot find variable for 'super'.");
    }


    Value getThis(const std::string& name, const Token& token_for_line) {
         try {
            return get(name);
        } catch (const std::runtime_error&) {
            throw RuntimeError(token_for_line.line, "Cannot use 'this' outside of a class method.");
        }
    }

    bool assign(const std::string& name, const Value& value) {
        auto it = variables.find(name);
        if (it != variables.end()) {
            if (it->second.static_type.has_value()) {
                if (!check_type(*(it->second.static_type), value)) {
                    throw std::runtime_error("Type mismatch on assignment to static variable '" + name + "'.");
                }
            }
            it->second.value = value;
            return true;
        }
        if (parent) return parent->assign(name, value);
        return false;
    }
};


// ===================================================================
// 5. 抽象语法树 (AST)
// ===================================================================
struct Expr {
    const int line;
    explicit Expr(int ln) : line(ln) {}
    virtual ~Expr() = default;
    [[nodiscard]] virtual Value eval(Environment& env) const = 0;
};
struct Stmt {
    const int line;
    explicit Stmt(int ln) : line(ln) {}
    virtual ~Stmt() = default;
    [[nodiscard]] virtual std::optional<Value> exec(Environment& env) const = 0;
};
struct AssignExpr final : Expr {
    ExprPtr target;
    ExprPtr value;
    AssignExpr(ExprPtr t, ExprPtr v, int ln) : Expr(ln), target(std::move(t)), value(std::move(v)) {}
    Value eval(Environment& env) const override;
};
struct LiteralExpr final : Expr {
    Value value;
    explicit LiteralExpr(Value v, int ln) : Expr(ln), value(std::move(v)) {}
    Value eval(Environment&) const override { return value; }
};
struct VarExpr final : Expr {
    std::string name;
    explicit VarExpr(std::string n, int ln) : Expr(ln), name(std::move(n)) {}
    Value eval(Environment& env) const override;
};
struct UnaryExpr final : Expr {
    Token op;
    ExprPtr expr;
    UnaryExpr(Token o, ExprPtr e, int ln) : Expr(ln), op(std::move(o)), expr(std::move(e)) {}
    Value eval(Environment& env) const override;
};
struct BinaryExpr final : Expr {
    Token op;
    ExprPtr left, right;
    BinaryExpr(Token o, ExprPtr l, ExprPtr r, int ln) : Expr(ln), op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
    Value eval(Environment& env) const override;
};
struct CallExpr final : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    CallExpr(ExprPtr c, std::vector<ExprPtr> a, int ln) : Expr(ln), callee(std::move(c)), args(std::move(a)) {}
    Value eval(Environment& env) const override;
};
struct ArrayLiteralExpr final : Expr {
    std::vector<ExprPtr> elements;
    explicit ArrayLiteralExpr(std::vector<ExprPtr> elems, int ln) : Expr(ln), elements(std::move(elems)) {}
    Value eval(Environment& env) const override;
};
struct DictLiteralExpr final : Expr {
    std::vector<std::pair<std::string, ExprPtr>> pairs;
    DictLiteralExpr(std::vector<std::pair<std::string, ExprPtr>> p, int ln)
        : Expr(ln), pairs(std::move(p)) {}
    Value eval(Environment& env) const override;
};
struct IndexExpr final : Expr {
    ExprPtr array;
    ExprPtr index;
    IndexExpr(ExprPtr a, ExprPtr i, int ln) : Expr(ln), array(std::move(a)), index(std::move(i)) {}
    Value eval(Environment& env) const override;
};
struct MemberAccessExpr final : Expr {
    ExprPtr object;
    Token member;
    MemberAccessExpr(ExprPtr obj, Token mem, int ln)
        : Expr(ln), object(std::move(obj)), member(std::move(mem)) {}
    Value eval(Environment& env) const override;
};
struct ThisExpr final : Expr {
    Token keyword;
    ThisExpr(Token kw, int ln) : Expr(ln), keyword(std::move(kw)) {}
    Value eval(Environment& env) const override;
};
struct SuperExpr final : Expr {
    Token keyword;
    Token method;
    SuperExpr(Token kw, Token m, int ln) : Expr(ln), keyword(std::move(kw)), method(std::move(m)) {}
    Value eval(Environment& env) const override;
};
struct BlockStmt final : Stmt {
    StmtList statements;
    explicit BlockStmt(StmtList stmts, int ln) : Stmt(ln), statements(std::move(stmts)) {}
    std::optional<Value> exec(Environment& env) const override;
};
struct ExprStmt final : Stmt {
    ExprPtr expr;
    explicit ExprStmt(ExprPtr e, int ln) : Stmt(ln), expr(std::move(e)) {}
    std::optional<Value> exec(Environment& env) const override { expr->eval(env); return std::nullopt; }
};
struct IfStmt final : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e, int ln) : Stmt(ln), condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
    std::optional<Value> exec(Environment& env) const override;
};
struct WhileStmt final : Stmt {
    ExprPtr condition;
    StmtPtr body;
    WhileStmt(ExprPtr c, StmtPtr b, int ln) : Stmt(ln), condition(std::move(c)), body(std::move(b)) {}
    std::optional<Value> exec(Environment& env) const override;
};
struct FuncStmt final : Stmt {
    std::string name;
    std::vector<ParamInfo> params;
    std::unique_ptr<BlockStmt> body;
    FuncStmt(std::string n, std::vector<ParamInfo> p, std::unique_ptr<BlockStmt> b, int ln)
        : Stmt(ln), name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
    std::optional<Value> exec(Environment& env) const override;
};
struct ClassStmt final : Stmt {
    std::string name;
    std::optional<std::unique_ptr<VarExpr>> superclass;
    std::vector<std::unique_ptr<FuncStmt>> methods;
    ClassStmt(std::string n, std::optional<std::unique_ptr<VarExpr>> sc, std::vector<std::unique_ptr<FuncStmt>> m, int ln)
        : Stmt(ln), name(std::move(n)), superclass(std::move(sc)), methods(std::move(m)) {}
    std::optional<Value> exec(Environment& env) const override;
};
struct ReturnStmt final : Stmt {
    ExprPtr expr;
    explicit ReturnStmt(ExprPtr e, int ln) : Stmt(ln), expr(std::move(e)) {}
    std::optional<Value> exec(Environment& env) const override {
        if (expr) return expr->eval(env);
        return Value();
    }
};
struct VarDeclStmt final : Stmt {
    std::string name;
    std::optional<TokenType> type_token;
    ExprPtr initializer;
    VarDeclStmt(std::string n, std::optional<TokenType> tt, ExprPtr init, int ln)
        : Stmt(ln), name(std::move(n)), type_token(tt), initializer(std::move(init)) {}
    std::optional<Value> exec(Environment& env) const override;
};
struct ForEachStmt final : Stmt {
    std::string variableName;
    ExprPtr iterable;
    StmtPtr body;
    ForEachStmt(std::string varName, ExprPtr iter, StmtPtr b, int ln)
        : Stmt(ln), variableName(std::move(varName)), iterable(std::move(iter)), body(std::move(b)) {}
    std::optional<Value> exec(Environment& env) const override;
};
struct ForStmt final : Stmt {
    StmtPtr initializer;
    ExprPtr condition;
    ExprPtr increment;
    StmtPtr body;
    ForStmt(StmtPtr init, ExprPtr cond, ExprPtr incr, StmtPtr b, int ln)
        : Stmt(ln), initializer(std::move(init)), condition(std::move(cond)),
          increment(std::move(incr)), body(std::move(b)) {}
    std::optional<Value> exec(Environment& env) const override;
};
struct BreakStmt final : Stmt {
    explicit BreakStmt(int ln) : Stmt(ln) {}
    std::optional<Value> exec([[maybe_unused]] Environment& env) const override {
        throw BreakSignal();
    }
};
struct ContinueStmt final : Stmt {
    explicit ContinueStmt(int ln) : Stmt(ln) {}
    std::optional<Value> exec([[maybe_unused]] Environment& env) const override {
        throw ContinueSignal();
    }
};

// ===================================================================
// 6. AST 节点与辅助函数实现
// ===================================================================
bool Value::toBool() const {
    return std::visit(overloaded{
        [](std::monostate) { return false; },
        [](int v) { return v != 0; },
        [](double v) { return v != 0.0; },
        [](bool v) { return v; },
        [](const StringData& v) { return !v.get().empty(); },
        [](const FuncType& v) { return v != nullptr; },
        [](const ArrayType& v) { return v && !v->elements.empty(); },
        [](const DictType& v) { return v && !v->pairs.empty(); },
        [](const MutableObjectType& v) { return v && (!v->fields.empty() || v->parent); }
    }, data);
}

std::string Value::toString() const {
    return std::visit(overloaded{
        [](std::monostate) -> std::string { return "nil"; },
        [](int v) -> std::string { return std::to_string(v); },
        [](double v) -> std::string {
            std::ostringstream oss;
            oss << v;
            return oss.str();
        },
        [](bool v) -> std::string { return v ? "true" : "false"; },
        [](const StringData& v) -> std::string { return v.get(); },
        [](const FuncType& v) -> std::string { return v ? v->toString() : "<null function>"; },
        [](const ArrayType& v) -> std::string {
            std::string result = "[";
            if (v) {
                for (size_t i = 0; i < v->elements.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += v->elements[i].toString();
                }
            }
            return result + "]";
        },
        [](const DictType& v) -> std::string {
            std::string result = "{";
            if (v) {
                bool first = true;
                for (const auto& pair : v->pairs) {
                    if (!first) result += ", ";
                    result += "\"" + pair.first + "\": ";
                    result += pair.second.toString();
                    first = false;
                }
            }
            return result + "}";
        },
        [](const MutableObjectType& v) -> std::string {
             return v ? v->toString() : "<null object>";
        }
    }, data);
}

std::string MutableObject::toString() const {
    if (klass) {
        auto class_val = klass;
        if (auto toStringMethod = class_val->findMethod("toString")) {
            if (toStringMethod->arity() == 0) {
                auto bound_method = toStringMethod->bind(std::const_pointer_cast<MutableObject>(this->shared_from_this()));
                Value result = bound_method->call({});
                if (result.is<StringData>()) {
                    return result.as<StringData>().get();
                }
            }
        }
        return "<" + klass->name + " instance>";
    }

    std::string result = "<object>{";
    bool first = true;
    for (const auto& pair : fields) {
        if (!first) result += ", ";
        result += "\"" + pair.first + "\": ";
        result += pair.second.toString();
        first = false;
    }
    return result + "}";
}


TokenType get_value_type_token(const Value& val) {
    return std::visit(overloaded{
        [](std::monostate) { return TokenType::OBJECT; }, // nil is like a null object
        [](int) { return TokenType::INT; },
        [](double) { return TokenType::FLOAT; },
        [](bool) { return TokenType::BOOL; },
        [](const StringData&) { return TokenType::STRING; },
        [](const Value::ArrayType&) { return TokenType::ARRAY; },
        [](const Value::DictType&) { return TokenType::DICT; },
        [](const Value::FuncType&) { return TokenType::FUNC; },
        [](const Value::MutableObjectType& obj) {
            if (obj && obj->klass) return TokenType::ID;
            return TokenType::OBJECT;
        }
    }, val.getVariant());
}

bool check_type(TokenType expected, const Value& val) {
    if (get_value_type_token(val) == expected) {
        return true;
    }
    if (expected == TokenType::FLOAT && val.is<int>()) {
        return true;
    }
    if (expected == TokenType::OBJECT && (val.is<Value::MutableObjectType>() || val.is<std::monostate>())) {
        return true;
    }
    return false;
}

// --- 完全重构的 AssignExpr::eval ---
Value AssignExpr::eval(Environment& env) const {
    Value valToAssign = value->eval(env);

    if (auto* varExpr = dynamic_cast<VarExpr*>(target.get())) {
        if (!env.assign(varExpr->name, valToAssign)) {
            throw RuntimeError(varExpr->line, "Undefined variable: " + varExpr->name);
        }
        return valToAssign;
    }

    if (auto* memberAccessExpr = dynamic_cast<MemberAccessExpr*>(target.get())) {
        Value objVal = memberAccessExpr->object->eval(env);
        if (objVal.is<Value::MutableObjectType>()) {
            auto obj = objVal.as<Value::MutableObjectType>();
            obj->set(memberAccessExpr->member.lexeme, valToAssign);
            return valToAssign;
        }
        if (objVal.is<Value::DictType>()) {
            auto& dict = objVal.as<Value::DictType>()->pairs;
            dict[memberAccessExpr->member.lexeme] = valToAssign;
            return valToAssign;
        }
        throw RuntimeError(memberAccessExpr->line, "Can only set properties on objects or dicts.");
    }

    if (auto* indexExpr = dynamic_cast<IndexExpr*>(target.get())) {
        Value indexVal = indexExpr->index->eval(env);

        auto perform_set_on_ref = [&](Value& containerRef) {
            if (containerRef.is<Value::ArrayType>()) {
                if (!indexVal.is<int>()) throw RuntimeError(indexExpr->index->line, "Array index must be an integer.");
                auto& arrVec = containerRef.as<Value::ArrayType>()->elements;
                int idx = indexVal.as<int>();
                if (idx < 0 || idx >= static_cast<int>(arrVec.size())) throw RuntimeError(indexExpr->line, "Array index out of bounds for assignment.");
                arrVec[idx] = valToAssign;
                return;
            }
            if (containerRef.is<StringData>()) {
                if (!indexVal.is<int>()) throw RuntimeError(indexExpr->index->line, "String index must be an integer.");
                if (!valToAssign.is<StringData>() || valToAssign.as<StringData>().get().length() != 1) {
                    throw RuntimeError(this->line, "Can only assign a single-character string to a string index.");
                }
                std::string& str = containerRef.as<StringData>().writeable();
                int idx = indexVal.as<int>();
                if (idx < 0 || idx >= static_cast<int>(str.length())) throw RuntimeError(indexExpr->line, "String index out of bounds for assignment.");
                str[idx] = valToAssign.as<StringData>().get()[0];
                return;
            }
            if (containerRef.is<Value::DictType>()) {
                if (!indexVal.is<StringData>()) throw RuntimeError(indexExpr->index->line, "Dict index must be a string.");
                auto& dict = containerRef.as<Value::DictType>()->pairs;
                const std::string& key = indexVal.as<StringData>().get();
                dict[key] = valToAssign;
                return;
            }
            if (containerRef.is<Value::MutableObjectType>()) {
                if (!indexVal.is<StringData>()) throw RuntimeError(indexExpr->index->line, "Object index must be a string.");
                auto& obj = containerRef.as<Value::MutableObjectType>();
                const std::string& key = indexVal.as<StringData>().get();
                obj->set(key, valToAssign);
                return;
            }
            throw RuntimeError(indexExpr->line, "This value type does not support indexed assignment.");
        };
        
        if (auto* containerVar = dynamic_cast<VarExpr*>(indexExpr->array.get())) {
            Value& containerRef = env.get(containerVar->name);
            perform_set_on_ref(containerRef);
        } else if (auto* containerMember = dynamic_cast<MemberAccessExpr*>(indexExpr->array.get())) {
             Value objVal = containerMember->object->eval(env);
            if (objVal.is<Value::MutableObjectType>()) {
                auto obj = objVal.as<Value::MutableObjectType>();
                if (!obj->has(containerMember->member.lexeme)) throw RuntimeError(containerMember->line, "Property '" + containerMember->member.lexeme + "' does not exist.");
                Value& containerRef = obj->fields.at(containerMember->member.lexeme);
                perform_set_on_ref(containerRef);
            } else if (objVal.is<Value::DictType>()) {
                auto& dict = objVal.as<Value::DictType>()->pairs;
                const std::string& key = containerMember->member.lexeme;
                if (dict.find(key) == dict.end()) throw RuntimeError(containerMember->line, "Key '" + key + "' does not exist.");
                Value& containerRef = dict.at(key);
                perform_set_on_ref(containerRef);
            } else {
                throw RuntimeError(containerMember->line, "Base of indexed assignment must be an object or a dictionary.");
            }
        } else {
            throw RuntimeError(indexExpr->line, "Left-hand side of indexed assignment must be a variable or a member access.");
        }
        return valToAssign;
    }
    
    throw RuntimeError(this->line, "Invalid assignment target.");
}


Value VarExpr::eval(Environment& env) const {
    try {
        return env.get(name);
    } catch (const std::runtime_error& e) {
        throw RuntimeError(this->line, e.what());
    }
}
Value ThisExpr::eval(Environment& env) const {
    return env.getThis("this", this->keyword);
}

Value SuperExpr::eval(Environment& env) const {
    Value this_val = env.get("this");
    auto instance = this_val.as<Value::MutableObjectType>();

    if (!instance->klass || !instance->klass->superclass) {
        throw RuntimeError(keyword.line, "Cannot use 'super' in a class with no superclass.");
    }

    auto super_class = instance->klass->superclass;

    Value method_val;
    try {
        method_val = super_class->prototype->get(method.lexeme);
    } catch (const std::runtime_error&) {
         throw RuntimeError(method.line, "Undefined property '" + method.lexeme + "' on superclass.");
    }


    if (!method_val.is<Value::FuncType>()) {
        throw RuntimeError(method.line, "Property '" + method.lexeme + "' on superclass is not a function.");
    }
    auto function = std::dynamic_pointer_cast<FunctionValue>(method_val.as<Value::FuncType>());
    if (!function) {
        throw RuntimeError(method.line, "Cannot call non-user-defined function with 'super'.");
    }

    return Value(function->bind(instance));
}

Value UnaryExpr::eval(Environment& env) const {
    Value val = expr->eval(env);
    if (op.type == TokenType::NOT) return !val.toBool();

    return std::visit(overloaded {
        [&](int v) -> Value {
            if (op.type == TokenType::MINUS) return -v;
            throw RuntimeError(this->line, "Invalid unary operator for integer.");
        },
        [&](double v) -> Value {
            if (op.type == TokenType::MINUS) return -v;
            throw RuntimeError(this->line, "Invalid unary operator for double.");
        },
        [&]([[maybe_unused]] auto v) -> Value {
            throw RuntimeError(this->line, "Invalid unary operator for this type.");
        }
    }, val.getVariant());
}
Value BinaryExpr::eval(Environment& env) const {
    if (op.type == TokenType::OR) {
        Value lval = left->eval(env);
        if (lval.toBool()) return Value(true);
        return Value(right->eval(env).toBool());
    }
    if (op.type == TokenType::AND) {
        Value lval = left->eval(env);
        if (!lval.toBool()) return Value(false);
        return Value(right->eval(env).toBool());
    }
    Value lval = left->eval(env);
    Value rval = right->eval(env);
    if (lval.is<int>() && rval.is<int>()) {
        const int l = lval.as<int>();
        const int r = rval.as<int>();
        switch (op.type) {
            case TokenType::PLUS:    return l + r;
            case TokenType::MINUS:   return l - r;
            case TokenType::STAR:    return l * r;
            case TokenType::SLASH:
                if (r == 0) throw RuntimeError(this->line, "Division by zero.");
                return static_cast<double>(l) / r;
            case TokenType::PERCENT:
                if (r == 0) throw RuntimeError(this->line, "Modulo by zero.");
                return l % r;
            case TokenType::EQ:      return l == r;
            case TokenType::NE:      return l != r;
            case TokenType::LT:      return l < r;
            case TokenType::LE:      return l <= r;
            case TokenType::GT:      return l > r;
            case TokenType::GE:      return l >= r;
            default: break;
        }
    }
    auto apply_double_op = [&](double l, double r) -> Value {
        switch (op.type) {
            case TokenType::PLUS:    return l + r;
            case TokenType::MINUS:   return l - r;
            case TokenType::STAR:    return l * r;
            case TokenType::SLASH:   if (r == 0.0) throw RuntimeError(this->line, "Division by zero."); return l / r;
            case TokenType::LT:      return l < r;
            case TokenType::LE:      return l <= r;
            case TokenType::GT:      return l > r;
            case TokenType::GE:      return l >= r;
            case TokenType::EQ:      return l == r;
            case TokenType::NE:      return l != r;
            default: throw RuntimeError(this->line, "Operator not applicable to float types.");
        }
    };
    auto visitor = overloaded {
        [&](int l, int r) -> Value {
            switch (op.type) {
                case TokenType::PLUS: return l + r;
                case TokenType::MINUS: return l - r;
                case TokenType::STAR: return l * r;
                case TokenType::SLASH: if (r == 0) throw RuntimeError(this->line, "Division by zero."); return static_cast<double>(l) / r;
                case TokenType::PERCENT: if (r == 0) throw RuntimeError(this->line, "Modulo by zero."); return l % r;
                case TokenType::LT: return l < r;
                case TokenType::LE: return l <= r;
                case TokenType::GT: return l > r;
                case TokenType::GE: return l >= r;
                case TokenType::EQ: return l == r;
                case TokenType::NE: return l != r;
                default: throw RuntimeError(this->line, "Operator not applicable to integers.");
            }
        },
        [&](double l, double r) -> Value { return apply_double_op(l, r); },
        [&](double l, int r) -> Value { return apply_double_op(l, static_cast<double>(r)); },
        [&](int l, double r) -> Value { return apply_double_op(static_cast<double>(l), r); },
        [&](StringData l, StringData r) -> Value {
            const std::string& l_str = l.get();
            const std::string& r_str = r.get();
            if (op.type == TokenType::PLUS) {
                StringData new_str_data = l;
                new_str_data.writeable() += r_str;
                return Value(new_str_data);
            }
             switch (op.type) {
                case TokenType::EQ: return l_str == r_str;
                case TokenType::NE: return l_str != r_str;
                case TokenType::LT: return l_str < r_str;
                case TokenType::LE: return l_str <= r_str;
                case TokenType::GT: return l_str > r_str;
                case TokenType::GE: return l_str >= r_str;
                default: throw RuntimeError(this->line, "Operator not applicable to strings.");
            }
        },
        [&](const Value::ArrayType& l, const Value::ArrayType& r) -> Value {
            if (op.type == TokenType::PLUS) {
                auto newArr = std::make_shared<ArrayValue>();
                newArr->elements = l->elements;
                newArr->elements.insert(newArr->elements.end(), r->elements.begin(), r->elements.end());
                return Value(newArr);
            }
            switch (op.type) {
                case TokenType::EQ: return lval == rval;
                case TokenType::NE: return lval != rval;
                default: throw RuntimeError(this->line, "Operator '" + op.lexeme + "' not applicable to arrays.");
            }
        },
        [&]([[maybe_unused]] const auto& l, [[maybe_unused]] const auto& r) -> Value {
             switch (op.type) {
                case TokenType::EQ: return lval == rval;
                case TokenType::NE: return lval != rval;
                default: throw RuntimeError(this->line, "Invalid operands for binary operator '" + op.lexeme + "'.");
            }
        }
    };
    return std::visit(visitor, lval.getVariant(), rval.getVariant());
}
Value CallExpr::eval(Environment& env) const {
    Value calleeVal = callee->eval(env);
    if (!calleeVal.is<Value::FuncType>()) {
        throw RuntimeError(this->line, "Can only call functions and other callables.");
    }
    auto func = calleeVal.as<Value::FuncType>();
    std::vector<Value> arguments;
    arguments.reserve(args.size());
    for (const auto& arg : args) {
        arguments.push_back(arg->eval(env));
    }
    if (func->arity() != -1 && arguments.size() != static_cast<size_t>(func->arity())) {
        throw RuntimeError(this->line, "Expected " + std::to_string(func->arity()) +
                               " arguments but got " + std::to_string(arguments.size()) + ".");
    }
    try {
        return func->call(arguments);
    } catch (const RuntimeError& e) {
        throw;
    } catch (const std::runtime_error& e) {
        throw RuntimeError(this->line, e.what());
    }
}
Value ArrayLiteralExpr::eval(Environment& env) const {
    auto newArr = std::make_shared<ArrayValue>();
    newArr->elements.reserve(elements.size());
    for (const auto& element : this->elements) {
        newArr->elements.push_back(element->eval(env));
    }
    return Value(newArr);
}
Value DictLiteralExpr::eval(Environment& env) const {
    auto newDict = std::make_shared<DictValue>();
    for (const auto& pair : pairs) {
        newDict->pairs[pair.first] = pair.second->eval(env);
    }
    return Value(newDict);
}
Value IndexExpr::eval(Environment& env) const {
    Value containerVal = array->eval(env);
    Value indexVal = index->eval(env);
    if (containerVal.is<Value::ArrayType>()) {
        if (!indexVal.is<int>()) {
            throw RuntimeError(this->index->line, "Array index must be an integer.");
        }
        int idx = indexVal.as<int>();
        const auto& arr = containerVal.as<Value::ArrayType>()->elements;
        if (idx < 0 || idx >= static_cast<int>(arr.size())) {
            throw RuntimeError(this->line, "Array index out of bounds");
        }
        return arr[idx];
    }
    if (containerVal.is<StringData>()) {
        if (!indexVal.is<int>()) {
            throw RuntimeError(this->index->line, "String index must be an integer.");
        }
        int idx = indexVal.as<int>();
        const auto& str = containerVal.as<StringData>().get();
        if (idx < 0 || idx >= static_cast<int>(str.length())) {
            throw RuntimeError(this->line, "String index out of bounds");
        }
        return Value(std::string(1, str[idx]));
    }
    if (containerVal.is<Value::DictType>()) {
        if (!indexVal.is<StringData>()) {
            throw RuntimeError(this->index->line, "Dict index must be a string.");
        }
        const std::string& key = indexVal.as<StringData>().get();
        const auto& dict = containerVal.as<Value::DictType>()->pairs;
        auto it = dict.find(key);
        if (it != dict.end()) {
            return it->second;
        }
        throw RuntimeError(this->line, "Undefined property '" + key + "'.");
    }
    if (containerVal.is<Value::MutableObjectType>()) {
        if (!indexVal.is<StringData>()) {
            throw RuntimeError(this->index->line, "Object index must be a string.");
        }
        const std::string& key = indexVal.as<StringData>().get();
        auto& obj = containerVal.as<Value::MutableObjectType>();
        try {
            return obj->get(key);
        } catch (const std::runtime_error& e) {
            throw RuntimeError(this->line, e.what());
        }
    }
    throw RuntimeError(this->line, "Index operation on a non-indexable value (must be array, string, dict, or object).");
}
Value MemberAccessExpr::eval(Environment& env) const {
    Value objVal = object->eval(env);
    if (objVal.is<Value::MutableObjectType>()) {
        auto instance = objVal.as<Value::MutableObjectType>();
        try {
            if (instance->fields.count(member.lexeme)) {
                return instance->fields.at(member.lexeme);
            }

            Value potential_method = instance->get(member.lexeme);
            if (potential_method.is<Value::FuncType>()) {
                if (auto func_val = std::dynamic_pointer_cast<FunctionValue>(potential_method.as<Value::FuncType>())) {
                    return Value(func_val->bind(instance));
                }
            }
            return potential_method;

        } catch (const std::runtime_error& e) {
            throw RuntimeError(line, e.what());
        }
    }
    if (objVal.is<Value::DictType>()) {
        auto& dict = objVal.as<Value::DictType>()->pairs;
        const std::string& key = member.lexeme;
        auto it = dict.find(key);
        if (it != dict.end()) {
            return it->second;
        }
        throw RuntimeError(line, "Undefined property '" + key + "'.");
    }
    throw RuntimeError(line, "Can only access properties on objects or dicts.");
}
std::optional<Value> BlockStmt::exec(Environment& env) const {
    auto blockEnv = std::make_shared<Environment>(env.shared_from_this());
    for (const auto& stmt : statements) {
        if (!stmt) continue;
        if (auto retVal = stmt->exec(*blockEnv); retVal.has_value()) return retVal;
    }
    return std::nullopt;
}
std::optional<Value> IfStmt::exec(Environment& env) const {
    if (condition->eval(env).toBool()) return thenBranch->exec(env);
    if (elseBranch) return elseBranch->exec(env);
    return std::nullopt;
}
std::optional<Value> WhileStmt::exec(Environment& env) const {
    while (condition->eval(env).toBool()) {
        try {
            if (auto retVal = body->exec(env); retVal.has_value()) {
                return retVal;
            }
        } catch (const BreakSignal&) {
            break;
        } catch (const ContinueSignal&) {
        }
    }
    return std::nullopt;
}
std::optional<Value> FuncStmt::exec(Environment& env) const {
    auto func = std::make_shared<FunctionValue>(params, body.get(), env.shared_from_this(), name == "init");
    env.define(name, Value(std::static_pointer_cast<Callable>(func)), std::nullopt);
    return std::nullopt;
}

std::optional<Value> ClassStmt::exec(Environment& env) const {
    std::shared_ptr<ClassValue> superclass_val = nullptr;
    if (superclass.has_value()) {
        Value sc_val = (*superclass)->eval(env);
        if (!sc_val.is<Value::FuncType>() || !(superclass_val = std::dynamic_pointer_cast<ClassValue>(sc_val.as<Value::FuncType>()))) {
            throw RuntimeError((*superclass)->line, "Superclass must be a class.");
        }
    }

    env.define(name, Value(), std::nullopt);

    auto class_env = std::make_shared<Environment>(env.shared_from_this());

    if (superclass_val) {
        class_env->define("super", Value(superclass_val), std::nullopt);
    }

    auto klass = std::make_shared<ClassValue>(name, superclass_val);

    for (const auto& method : methods) {
        bool is_init = (method->name == "init");
        auto func = std::make_shared<FunctionValue>(method->params, method->body.get(), class_env, is_init);
        
        if (is_init) {
            klass->initializer = func;
        }
        klass->prototype->set(method->name, Value(std::static_pointer_cast<Callable>(func)));
    }

    if (!env.assign(name, Value(std::static_pointer_cast<Callable>(klass)))) {
         throw std::runtime_error("Internal error: could not assign class value.");
    }

    return std::nullopt;
}

std::optional<Value> VarDeclStmt::exec(Environment& env) const {
    Value value;
    if (initializer) {
        value = initializer->eval(env);
    } else {
        if (type_token.has_value()) {
            switch (*type_token) {
                case TokenType::INT:    value = Value(0); break;
                case TokenType::FLOAT:  value = Value(0.0); break;
                case TokenType::BOOL:   value = Value(false); break;
                case TokenType::STRING: value = Value(""); break;
                case TokenType::ARRAY:  value = Value(std::make_shared<ArrayValue>()); break;
                case TokenType::DICT:   value = Value(std::make_shared<DictValue>()); break;
                case TokenType::OBJECT: value = Value(std::make_shared<MutableObject>(nullptr)); break;
                default: value = Value(); // nil
            }
        } else {
            value = Value(); // nil
        }
    }
    env.define(name, std::move(value), type_token);
    return std::nullopt;
}

std::shared_ptr<FunctionValue> FunctionValue::bind(std::shared_ptr<MutableObject> instance) {
    auto environment = std::make_shared<Environment>(closure);
    environment->define("this", Value(instance), std::nullopt);
    return std::make_shared<FunctionValue>(params, body, environment, is_initializer);
}

Value FunctionValue::call(const std::vector<Value>& args) {
    auto executionEnv = std::make_shared<Environment>(closure);
    for (size_t i = 0; i < params.size(); ++i) {
        if (params[i].type.has_value()) {
            if (!check_type(*(params[i].type), args[i])) {
                throw std::runtime_error("Argument type mismatch for parameter '" + params[i].name + "'.");
            }
        }
        executionEnv->define(params[i].name, args[i], std::nullopt);
    }
    try {
        if (auto retVal = body->exec(*executionEnv); retVal.has_value()) {
            if (is_initializer) return closure->getThis("this", Token(TokenType::THIS, "this", body->line));
            return *retVal;
        }
    } catch(const BreakSignal&) {
         throw RuntimeError(body->line, "Cannot 'break' from a function.");
    } catch(const ContinueSignal&) {
        throw RuntimeError(body->line, "Cannot 'continue' from a function.");
    }
    
    if (is_initializer) return closure->getThis("this", Token(TokenType::THIS, "this", body->line));
    return Value();
}

Value ClassValue::call(const std::vector<Value>& args) {
    auto instance = std::make_shared<MutableObject>(this->prototype);
    instance->klass = this->shared_from_this();

    if (initializer.has_value()) {
        (*initializer)->bind(instance)->call(args);
    } else if (!args.empty()) {
        throw std::runtime_error("Class " + name + " has no 'init' method and cannot be called with arguments.");
    }
    return Value(instance);
}

std::optional<Value> ForEachStmt::exec(Environment& env) const {
    auto loopEnv = std::make_shared<Environment>(env.shared_from_this());
    Value iterableVal = iterable->eval(env);

    auto iter_body = [&](const Value& element) -> std::optional<Value> {
        loopEnv->define(variableName, element, std::nullopt);
        try {
            if (auto retVal = body->exec(*loopEnv); retVal.has_value()) {
                return retVal;
            }
        } catch (const ContinueSignal&) {
        }
        return std::nullopt;
    };
    try {
        if (iterableVal.is<Value::ArrayType>()) {
            const auto& arr = iterableVal.as<Value::ArrayType>()->elements;
            for (const auto& element : arr) {
                if (auto retVal = iter_body(element); retVal.has_value()) return retVal;
            }
        } else if (iterableVal.is<StringData>()) {
            const auto& str = iterableVal.as<StringData>().get();
            for (char c : str) {
                if (auto retVal = iter_body(Value(std::string(1, c))); retVal.has_value()) return retVal;
            }
        } else {
            throw RuntimeError(this->line, "Value is not iterable. Can only iterate over arrays and strings.");
        }
    } catch (const BreakSignal&) {}
    return std::nullopt;
}
std::optional<Value> ForStmt::exec(Environment& env) const {
    auto loopEnv = std::make_shared<Environment>(env.shared_from_this());
    if (initializer) {
        initializer->exec(*loopEnv);
    }
    while(true) {
        if (condition && !condition->eval(*loopEnv).toBool()) {
            break;
        }
        try {
            if (auto retVal = body->exec(*loopEnv); retVal.has_value()) {
                return retVal;
            }
        } catch (const BreakSignal&) {
            break;
        } catch (const ContinueSignal&) {}
        if (increment) {
            increment->eval(*loopEnv);
        }
    }
    return std::nullopt;
}

// ===================================================================
// 7. 语法分析器 (Parser)
// ===================================================================
class Parser {
    std::vector<Token> tokens;
    size_t current = 0;
public:
    explicit Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}
    StmtList parse() {
        StmtList statements;
        while (!isAtEnd()) {
            statements.push_back(parseDeclaration());
        }
        return statements;
    }
private:
    bool match(std::initializer_list<TokenType> types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }
    Token& consume(TokenType type, std::string_view message) {
        if (check(type)) return advance();
        throw std::runtime_error(std::string(message) + " at line " + std::to_string(peek().line));
    }
    Token& advance() { if (!isAtEnd()) current++; return previous(); }
    bool isAtEnd() const { return peek().type == TokenType::END; }
    const Token& peek() const { return tokens[current]; }
    Token& previous() { return tokens[current - 1]; }
    bool check(TokenType type) const { return !isAtEnd() && peek().type == type; }
    bool checkAhead(std::initializer_list<TokenType> types) const {
        size_t lookahead = current;
        for (TokenType type : types) {
            if (lookahead >= tokens.size() || tokens[lookahead].type != type) {
                return false;
            }
            lookahead++;
        }
        return true;
    }

    void synchronize() {
        advance();
        while (!isAtEnd()) {
            if (previous().type == TokenType::SEMICOLON) return;
            switch (peek().type) {
                case TokenType::CLASS: case TokenType::FUNC: case TokenType::VAR: case TokenType::IF:
                case TokenType::WHILE: case TokenType::RETURN: case TokenType::FOR:
                case TokenType::BREAK: case TokenType::CONTINUE:
                case TokenType::INT: case TokenType::FLOAT: case TokenType::BOOL:
                case TokenType::STRING: case TokenType::ARRAY: case TokenType::DICT:
                case TokenType::OBJECT:
                    return;
                default: break;
            }
            advance();
        }
    }
    StmtPtr parseDeclaration() {
        try {
            if (match({TokenType::CLASS})) return parseClassDeclaration();
            if (match({TokenType::FUNC})) return parseFuncDeclaration("function");
            if (match({TokenType::VAR, TokenType::INT, TokenType::FLOAT, TokenType::BOOL, TokenType::STRING, TokenType::ARRAY, TokenType::DICT, TokenType::OBJECT})) {
                return parseVarDeclaration(previous());
            }
            return parseStatement();
        } catch (const std::runtime_error& e) {
            std::cerr << "Parse Error: " << e.what() << std::endl;
            synchronize();
            return nullptr;
        }
    }
    std::unique_ptr<FuncStmt> parseFuncDeclaration(const std::string& kind);
    StmtPtr parseClassDeclaration();
    StmtPtr parseVarDeclaration(Token type_token);
    StmtPtr parseStatement();
    StmtPtr parseIfStatement();
    StmtPtr parseWhileStatement();
    StmtPtr parseForStatement();
    StmtPtr parseBreakStatement();
    StmtPtr parseContinueStatement();
    std::unique_ptr<BlockStmt> parseBlock();
    StmtPtr parseReturnStatement();
    StmtPtr parseExprStatement();
    ExprPtr parseExpression();
    ExprPtr parseAssignment();
    ExprPtr parseLogicalOr();
    ExprPtr parseLogicalAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseTerm();
    ExprPtr parseFactor();
    ExprPtr parseUnary();
    ExprPtr parseCall();
    ExprPtr parsePrimary();
    ExprPtr parseArrayLiteral();
    ExprPtr parseDictLiteral();
};

std::unique_ptr<FuncStmt> Parser::parseFuncDeclaration(const std::string& kind) {
    int ln = previous().line;
    Token name = consume(TokenType::ID, "Expect " + kind + " name.");
    consume(TokenType::LPAREN, "Expect '(' after " + kind + " name.");
    std::vector<ParamInfo> parameters;
    if (!check(TokenType::RPAREN)) {
        do {
            ParamInfo param;
            if (match({TokenType::INT, TokenType::FLOAT, TokenType::BOOL, TokenType::STRING, TokenType::ARRAY, TokenType::DICT, TokenType::OBJECT})) {
                param.type = previous().type;
            }
            param.name = consume(TokenType::ID, "Expect parameter name.").lexeme;
            parameters.push_back(param);
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RPAREN, "Expect ')' after parameters.");
    consume(TokenType::LBRACE, "Expect '{' before " + kind + " body.");
    auto body = parseBlock();
    return std::make_unique<FuncStmt>(name.lexeme, std::move(parameters), std::move(body), ln);
}

StmtPtr Parser::parseClassDeclaration() {
    int ln = previous().line;
    Token name = consume(TokenType::ID, "Expect class name.");

    std::optional<std::unique_ptr<VarExpr>> superclass;
    if (match({TokenType::EXTENDS})) {
        consume(TokenType::ID, "Expect superclass name.");
        superclass = std::make_unique<VarExpr>(previous().lexeme, previous().line);
    }

    consume(TokenType::LBRACE, "Expect '{' before class body.");

    std::vector<std::unique_ptr<FuncStmt>> methods;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        consume(TokenType::FUNC, "Expect 'func' to define a method.");
        methods.push_back(parseFuncDeclaration("method"));
    }

    consume(TokenType::RBRACE, "Expect '}' after class body.");
    return std::make_unique<ClassStmt>(name.lexeme, std::move(superclass), std::move(methods), ln);
}
StmtPtr Parser::parseVarDeclaration(Token type_token) {
    int ln = type_token.line;
    Token name = consume(TokenType::ID, "Expect variable name.");

    std::optional<TokenType> static_type;
    if (type_token.type != TokenType::VAR) {
        static_type = type_token.type;
    }

    ExprPtr initializer = nullptr;
    if (match({TokenType::ASSIGN})) {
        initializer = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_unique<VarDeclStmt>(name.lexeme, static_type, std::move(initializer), ln);
}
StmtPtr Parser::parseStatement() {
    if (match({TokenType::IF})) return parseIfStatement();
    if (match({TokenType::WHILE})) return parseWhileStatement();
    if (match({TokenType::FOR})) return parseForStatement();
    if (match({TokenType::LBRACE})) return std::move(parseBlock());
    if (match({TokenType::RETURN})) return parseReturnStatement();
    if (match({TokenType::BREAK})) return parseBreakStatement();
    if (match({TokenType::CONTINUE})) return parseContinueStatement();
    if (match({TokenType::SEMICOLON})) return nullptr;
    return parseExprStatement();
}
StmtPtr Parser::parseIfStatement() {
    int ln = previous().line;
    consume(TokenType::LPAREN, "Expect '(' after 'if'.");
    ExprPtr condition = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after if condition.");
    StmtPtr thenBranch = parseStatement();
    StmtPtr elseBranch = nullptr;
    if (match({TokenType::ELSE})) {
        elseBranch = parseStatement();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), ln);
}
StmtPtr Parser::parseWhileStatement() {
    int ln = previous().line;
    consume(TokenType::LPAREN, "Expect '(' after 'while'.");
    ExprPtr condition = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after while condition.");
    StmtPtr body = parseStatement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), ln);
}
StmtPtr Parser::parseReturnStatement() {
    int ln = previous().line;
    ExprPtr value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after return value.");
    return std::make_unique<ReturnStmt>(std::move(value), ln);
}
std::unique_ptr<BlockStmt> Parser::parseBlock() {
    int ln = previous().line;
    StmtList statements;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(parseDeclaration());
    }
    consume(TokenType::RBRACE, "Expect '}' to end a block.");
    return std::make_unique<BlockStmt>(std::move(statements), ln);
}
StmtPtr Parser::parseExprStatement() {
    int ln = peek().line;
    ExprPtr expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr), ln);
}
ExprPtr Parser::parseExpression() { return parseAssignment(); }
ExprPtr Parser::parseAssignment() {
    ExprPtr expr = parseLogicalOr();
    if (match({TokenType::ASSIGN})) {
        Token equals = previous();
        ExprPtr value = parseAssignment();
        if (dynamic_cast<VarExpr*>(expr.get()) || dynamic_cast<IndexExpr*>(expr.get()) || dynamic_cast<MemberAccessExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(std::move(expr), std::move(value), equals.line);
        }
        throw std::runtime_error("Invalid assignment target at line " + std::to_string(equals.line));
    }
    return expr;
}
ExprPtr Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    while (match({TokenType::OR})) {
        Token op = previous();
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op.line);
    }
    return expr;
}
ExprPtr Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    while (match({TokenType::AND})) {
        Token op = previous();
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op.line);
    }
    return expr;
}
ExprPtr Parser::parseEquality() {
    auto expr = parseComparison();
    while (match({TokenType::EQ, TokenType::NE})) {
        Token op = previous();
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op.line);
    }
    return expr;
}
ExprPtr Parser::parseComparison() {
    auto expr = parseTerm();
    while (match({TokenType::LT, TokenType::LE, TokenType::GT, TokenType::GE})) {
        Token op = previous();
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op.line);
    }
    return expr;
}
ExprPtr Parser::parseTerm() {
    auto expr = parseFactor();
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        Token op = previous();
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op.line);
    }
    return expr;
}
ExprPtr Parser::parseFactor() {
    auto expr = parseUnary();
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        Token op = previous();
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right), op.line);
    }
    return expr;
}
ExprPtr Parser::parseUnary() {
    if (match({TokenType::MINUS, TokenType::NOT})) {
        Token op = previous();
        auto right = parseUnary();
        return std::make_unique<UnaryExpr>(op, std::move(right), op.line);
    }
    return parseCall();
}
ExprPtr Parser::parseCall() {
    ExprPtr expr = parsePrimary();
    while (true) {
        if (match({TokenType::LPAREN})) {
            Token paren = previous();
            std::vector<ExprPtr> arguments;
            if (!check(TokenType::RPAREN)) {
                do {
                    arguments.push_back(parseExpression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expect ')' after arguments.");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(arguments), paren.line);
        } else if (match({TokenType::LBRACKET})) {
            Token bracket = previous();
            ExprPtr index = parseExpression();
            consume(TokenType::RBRACKET, "Expect ']' after index.");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index), bracket.line);
        } else if (match({TokenType::DOT})) {
            Token name = consume(TokenType::ID, "Expect property name after '.'.");
            expr = std::make_unique<MemberAccessExpr>(std::move(expr), name, name.line);
        } else {
            break;
        }
    }
    return expr;
}
ExprPtr Parser::parsePrimary() {
    if (match({TokenType::INT_LITERAL})) return std::make_unique<LiteralExpr>(Value(std::stoi(previous().lexeme)), previous().line);
    if (match({TokenType::FLOAT_LITERAL})) return std::make_unique<LiteralExpr>(Value(std::stod(previous().lexeme)), previous().line);
    if (match({TokenType::STR})) return std::make_unique<LiteralExpr>(Value(StringData::from_literal(previous().lexeme)), previous().line);
    if (match({TokenType::TRUE})) return std::make_unique<LiteralExpr>(Value(true), previous().line);
    if (match({TokenType::FALSE})) return std::make_unique<LiteralExpr>(Value(false), previous().line);
    if (match({TokenType::THIS})) return std::make_unique<ThisExpr>(previous(), previous().line);
    if (match({TokenType::SUPER})) {
        Token keyword = previous();
        consume(TokenType::DOT, "Expect '.' after 'super'.");
        Token method = consume(TokenType::ID, "Expect superclass method name.");
        return std::make_unique<SuperExpr>(keyword, method, keyword.line);
    }
    if (match({TokenType::ID, TokenType::INT, TokenType::FLOAT, TokenType::BOOL, TokenType::STRING, TokenType::DICT, TokenType::OBJECT})) {
        return std::make_unique<VarExpr>(previous().lexeme, previous().line);
    }
    if (match({TokenType::LPAREN})) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after expression.");
        return expr;
    }
    if (match({TokenType::LBRACE})) {
        return parseDictLiteral();
    }
    if (match({TokenType::LBRACKET})) {
        return parseArrayLiteral();
    }
    throw std::runtime_error("Expect expression at line " + std::to_string(peek().line));
}
ExprPtr Parser::parseArrayLiteral() {
    int ln = previous().line;
    std::vector<ExprPtr> elements;
    if (!check(TokenType::RBRACKET)) {
        do {
            elements.push_back(parseExpression());
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RBRACKET, "Expect ']' after array elements.");
    return std::make_unique<ArrayLiteralExpr>(std::move(elements), ln);
}
ExprPtr Parser::parseDictLiteral() {
    int ln = previous().line;
    std::vector<std::pair<std::string, ExprPtr>> pairs;

    if (!check(TokenType::RBRACE)) {
        do {
            Token key = consume(TokenType::STR, "Expect string literal as dictionary key.");
            consume(TokenType::COLON, "Expect ':' after dictionary key.");
            ExprPtr value = parseExpression();
            pairs.emplace_back(key.lexeme, std::move(value));
        } while (match({TokenType::COMMA}));
    }

    consume(TokenType::RBRACE, "Expect '}' to end dictionary literal.");
    return std::make_unique<DictLiteralExpr>(std::move(pairs), ln);
}
StmtPtr Parser::parseBreakStatement() {
    int ln = previous().line;
    consume(TokenType::SEMICOLON, "Expect ';' after 'break'.");
    return std::make_unique<BreakStmt>(ln);
}
StmtPtr Parser::parseContinueStatement() {
    int ln = previous().line;
    consume(TokenType::SEMICOLON, "Expect ';' after 'continue'.");
    return std::make_unique<ContinueStmt>(ln);
}
StmtPtr Parser::parseForStatement() {
    int for_line = previous().line;
    consume(TokenType::LPAREN, "Expect '(' after 'for'.");
    if ((check(TokenType::VAR) || check(TokenType::INT) || check(TokenType::FLOAT) || check(TokenType::BOOL) || check(TokenType::STRING) || check(TokenType::ARRAY) || check(TokenType::DICT) || check(TokenType::OBJECT))
        && checkAhead({peek().type, TokenType::ID, TokenType::COLON})) {

        advance();
        Token name = consume(TokenType::ID, "Expect variable name in for-each loop.");
        consume(TokenType::COLON, "Expect ':' after variable name in for-each loop.");
        ExprPtr iterable = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after for-each clauses.");
        StmtPtr body = parseStatement();
        return std::make_unique<ForEachStmt>(name.lexeme, std::move(iterable), std::move(body), for_line);
    }
    StmtPtr initializer;
    if (match({TokenType::SEMICOLON})) {
        initializer = nullptr;
    } else if (match({TokenType::VAR, TokenType::INT, TokenType::FLOAT, TokenType::BOOL, TokenType::STRING, TokenType::ARRAY, TokenType::DICT, TokenType::OBJECT})) {
        initializer = parseVarDeclaration(previous());
    } else {
        initializer = parseExprStatement();
    }
    ExprPtr condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after for loop condition.");
    ExprPtr increment = nullptr;
    if (!check(TokenType::RPAREN)) {
        increment = parseExpression();
    }
    consume(TokenType::RPAREN, "Expect ')' after for clauses.");
    StmtPtr body = parseStatement();
    return std::make_unique<ForStmt>(std::move(initializer), std::move(condition), std::move(increment), std::move(body), for_line);
}

// ===================================================================
// 8. 解释器 (Interpreter)
// ===================================================================
Value deepcopy_recursive(const Value& val, std::unordered_map<const void*, Value>& memo) {
    return std::visit(overloaded{
        [](std::monostate) { return Value(); },
        [](int v) { return Value(v); },
        [](double v) { return Value(v); },
        [](bool v) { return Value(v); },
        [](const StringData& v) { return Value(v.get()); },
        [](const Value::FuncType& v) { return Value(v); },

        [&](const Value::ArrayType& arr) -> Value {
            const void* ptr = arr.get();
            if (memo.count(ptr)) {
                return memo.at(ptr);
            }

            auto newArr = std::make_shared<ArrayValue>();
            Value newArrVal(newArr);
            memo[ptr] = newArrVal;

            newArr->elements.reserve(arr->elements.size());
            for (const auto& elem : arr->elements) {
                newArr->elements.push_back(deepcopy_recursive(elem, memo));
            }

            return newArrVal;
        },

        [&](const Value::DictType& dict) -> Value {
            const void* ptr = dict.get();
            if (memo.count(ptr)) {
                return memo.at(ptr);
            }

            auto newDict = std::make_shared<DictValue>();
            Value newDictVal(newDict);
            memo[ptr] = newDictVal;

            for (const auto& pair : dict->pairs) {
                newDict->pairs[pair.first] = deepcopy_recursive(pair.second, memo);
            }

            return newDictVal;
        },

        [&](const Value::MutableObjectType& obj) -> Value {
            const void* ptr = obj.get();
            if (memo.count(ptr)) {
                return memo.at(ptr);
            }

            Value::MutableObjectType newParent = nullptr;
            if (obj->parent) {
                Value parentCopy = deepcopy_recursive(Value(obj->parent), memo);
                if (parentCopy.is<Value::MutableObjectType>()) {
                    newParent = parentCopy.as<Value::MutableObjectType>();
                }
            }

            auto newObj = std::make_shared<MutableObject>(newParent);
            newObj->klass = obj->klass;
            Value newObjVal(newObj);
            memo[ptr] = newObjVal;

            for (const auto& pair : obj->fields) {
                newObj->fields[pair.first] = deepcopy_recursive(pair.second, memo);
            }

            return newObjVal;
        }

    }, val.getVariant());
}


class Interpreter {
    std::shared_ptr<Environment> globalEnv = std::make_shared<Environment>();
    StmtList ast;
public:
    explicit Interpreter(StmtList programAst) : ast(std::move(programAst)) {
        defineNativeFunctions();
    }
    void interpret() {
        try {
            for (const auto& statement : ast) {
                if (!statement) continue;
                if (auto retVal = statement->exec(*globalEnv); retVal.has_value()) {
                    std::cerr << "Runtime Error: Cannot return from top-level code." << std::endl;
                    break;
                }
            }
        } catch (const RuntimeError& e) {
            std::cerr << e.what() << std::endl;
        } catch (const std::runtime_error& e) {
            std::cerr << "Runtime Error: " << e.what() << std::endl;
        }
    }
private:
    void defineNativeFunctions() {
        globalEnv->define("print", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                for (size_t i = 0; i < args.size(); ++i) {
                    std::cout << args[i].toString();
                    if (i < args.size() - 1) {
                        std::cout << " ";
                    }
                }
                std::cout << std::endl;
                return Value();
            }, -1, "print"
        )), std::nullopt);
        globalEnv->define("len", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                const auto& val = args[0];
                return std::visit(overloaded{
                    [](const StringData& s) { return Value(static_cast<int>(s.get().length())); },
                    [](const Value::ArrayType& a) { return Value(static_cast<int>(a->elements.size())); },
                    [](const Value::DictType& d) { return Value(static_cast<int>(d->pairs.size())); },
                    [](const Value::MutableObjectType& o) { return Value(static_cast<int>(o->fields.size())); },
                    [](const auto&) -> Value { throw std::runtime_error("Value has no length."); }
                }, val.getVariant());
            }, 1, "len"
        )), std::nullopt);
        globalEnv->define("type", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                const auto& val = args[0];
                 return std::visit(overloaded{
                    [](std::monostate) { return Value("nil"); },
                    [](int) { return Value("int"); },
                    [](double) { return Value("float"); },
                    [](bool) { return Value("bool"); },
                    [](const StringData&) { return Value("string"); },
                    [](const Value::ArrayType&) { return Value("array"); },
                    [](const Value::DictType&) { return Value("dict"); },
                    [](const Value::FuncType& v) {
                        if (std::dynamic_pointer_cast<ClassValue>(v)) return Value("class");
                        if (auto nf = std::dynamic_pointer_cast<NativeFunction>(v); nf && nf->toString() == "<native function: Object>") return Value("object_constructor");
                        return Value("function");
                    },
                    [](const Value::MutableObjectType& o) {
                        if (o && o->klass) return Value(o->klass->name);
                        return Value("object");
                    }
                }, val.getVariant());
            }, 1, "type"
        )), std::nullopt);
        globalEnv->define("str", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                return Value(args[0].toString());
            }, 1, "str"
        )), std::nullopt);
        globalEnv->define("int", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                const auto& val = args[0];
                return std::visit(overloaded{
                    [](int i) { return Value(i); },
                    [](double d) { return Value(static_cast<int>(d)); },
                    [](bool b) { return Value(static_cast<int>(b)); },
                    [](const StringData& s_data) -> Value {
                        const auto& s = s_data.get();
                        try { return Value(std::stoi(s)); }
                        catch (...) { throw std::runtime_error("Cannot convert string '" + s + "' to int."); }
                    },
                    [](const auto&) -> Value { throw std::runtime_error("Cannot convert type to int."); }
                }, val.getVariant());
            }, 1, "int"
        )), std::nullopt);
        globalEnv->define("bool", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                return Value(args[0].toBool());
            }, 1, "bool"
        )), std::nullopt);
        globalEnv->define("float", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                const auto& val = args[0];
                return std::visit(overloaded{
                    [](int i) { return Value(static_cast<double>(i)); },
                    [](double d) { return Value(d); },
                    [](const StringData& s_data) -> Value {
                        const auto& s = s_data.get();
                        try { return Value(std::stod(s)); }
                        catch (...) { throw std::runtime_error("Cannot convert string '" + s + "' to float."); }
                    },
                    [](const auto&) -> Value { throw std::runtime_error("Cannot convert type to float."); }
                }, val.getVariant());
            }, 1, "float"
        )), std::nullopt);
        globalEnv->define("append", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (args.size() != 2) throw std::runtime_error("append() takes exactly 2 arguments.");
                
                Value container = args[0];
                const Value& element = args[1];

                if (container.is<Value::ArrayType>()) {
                    auto& vec = container.as<Value::ArrayType>()->elements;
                    vec.push_back(element);
                    return container;
                }
                if (container.is<StringData>()) {
                    if (!element.is<StringData>()) throw std::runtime_error("Can only append a string to a string.");
                    
                    container.as<StringData>().writeable() += element.as<StringData>().get();
                    return container;
                }
                
                throw std::runtime_error("First argument to append must be an array or a string.");
            }, 2, "append"
        )), std::nullopt);
        globalEnv->define("pop", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (args.size() != 1 && args.size() != 2) throw std::runtime_error("pop() takes 1 or 2 arguments.");
                if (!args[0].is<Value::ArrayType>()) throw std::runtime_error("First argument to pop must be an array.");
                auto& vec = args[0].as<Value::ArrayType>()->elements;
                if (vec.empty()) throw std::runtime_error("pop from empty array.");
                if (args.size() == 1) {
                    Value back = vec.back();
                    vec.pop_back();
                    return back;
                } else {
                    if (!args[1].is<int>()) throw std::runtime_error("Index for pop must be an integer.");
                    int idx = args[1].as<int>();
                    if (idx < 0 || idx >= static_cast<int>(vec.size())) throw std::runtime_error("pop index out of range.");
                    Value val = vec[idx];
                    vec.erase(vec.begin() + idx);
                    return val;
                }
            }, -1, "pop"
        )), std::nullopt);
        globalEnv->define("slice", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (args.size() != 2 && args.size() != 3) throw std::runtime_error("slice() takes 2 or 3 arguments.");
                if (!args[0].is<Value::ArrayType>()) throw std::runtime_error("First argument to slice must be an array.");
                if (!args[1].is<int>()) throw std::runtime_error("Slice start index must be an integer.");
                const auto& src_vec = args[0].as<Value::ArrayType>()->elements;
                int start = args[1].as<int>();
                int end = src_vec.size();
                if (args.size() == 3) {
                    if (!args[2].is<int>()) throw std::runtime_error("Slice end index must be an integer.");
                    end = args[2].as<int>();
                }
                if (start < 0 || end > static_cast<int>(src_vec.size()) || start > end) {
                    throw std::runtime_error("Slice indices are out of bounds.");
                }
                auto new_arr_val = std::make_shared<ArrayValue>();
                new_arr_val->elements.assign(src_vec.begin() + start, src_vec.begin() + end);
                return Value(new_arr_val);
            }, -1, "slice"
        )), std::nullopt);
        globalEnv->define("input", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                 if (args.size() > 1) throw std::runtime_error("input() takes 0 or 1 argument.");
                 if (args.size() == 1) {
                     std::cout << args[0].toString();
                     std::cout.flush();
                 }
                 std::string line;
                 std::getline(std::cin, line);
                 return Value(line);
            }, -1, "input"
        )), std::nullopt);
        globalEnv->define("read_file", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (!args[0].is<StringData>()) throw std::runtime_error("Argument to read_file must be a string path.");
                const auto& path = args[0].as<StringData>().get();
                std::ifstream file(path);
                if (!file) throw std::runtime_error("Could not open file: " + path);
                std::stringstream buffer;
                buffer << file.rdbuf();
                return Value(buffer.str());
            }, 1, "read_file"
        )), std::nullopt);
        globalEnv->define("write_file", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (!args[0].is<StringData>()) throw std::runtime_error("Path for write_file must be a string.");
                if (!args[1].is<StringData>()) throw std::runtime_error("Content for write_file must be a string.");
                const auto& path = args[0].as<StringData>().get();
                const auto& content = args[1].as<StringData>().get();
                std::ofstream file(path);
                if (!file) throw std::runtime_error("Could not open file for writing: " + path);
                file << content;
                return Value();
            }, 2, "write_file"
        )), std::nullopt);
        static const auto start_time = std::chrono::high_resolution_clock::now();
        globalEnv->define("clock", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>&) -> Value {
                auto now = std::chrono::high_resolution_clock::now();
                auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                return Value(static_cast<int>(duration_ms));
            }, 0, "clock"
        )), std::nullopt);
        globalEnv->define("assert", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (args.size() != 1 && args.size() != 2) throw std::runtime_error("assert() takes 1 or 2 arguments.");
                if (!args[0].toBool()) {
                    std::string message = "Assertion failed.";
                    if (args.size() == 2) {
                        message += " " + args[1].toString();
                    }
                    throw std::runtime_error(message);
                }
                return Value();
            }, -1, "assert"
        )), std::nullopt);
        globalEnv->define("range", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (args.empty() || args.size() > 3) throw std::runtime_error("range() takes 1, 2, or 3 arguments.");
                int start = 0, end = 0, step = 1;
                if (args.size() == 1) {
                    if (!args[0].is<int>()) throw std::runtime_error("range() argument must be an integer.");
                    end = args[0].as<int>();
                } else {
                    if (!args[0].is<int>() || !args[1].is<int>()) throw std::runtime_error("range() arguments must be integers.");
                    start = args[0].as<int>();
                    end = args[1].as<int>();
                    if (args.size() == 3) {
                        if (!args[2].is<int>()) throw std::runtime_error("range() step must be an integer.");
                        step = args[2].as<int>();
                        if (step == 0) throw std::runtime_error("range() step cannot be zero.");
                    }
                }
                auto arr = std::make_shared<ArrayValue>();
                if (step > 0) {
                    for (int i = start; i < end; i += step) arr->elements.push_back(Value(i));
                } else {
                    for (int i = start; i > end; i += step) arr->elements.push_back(Value(i));
                }
                return Value(arr);
            }, -1, "range"
        )), std::nullopt);
        globalEnv->define("dict", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (!args.empty()) throw std::runtime_error("dict() takes no arguments.");
                return Value(std::make_shared<DictValue>());
            }, 0, "dict"
        )), std::nullopt);
        globalEnv->define("map", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (!args[0].is<Value::FuncType>()) throw std::runtime_error("First argument to map must be a function.");
                if (!args[1].is<Value::ArrayType>()) throw std::runtime_error("Second argument to map must be an array.");
                auto func = args[0].as<Value::FuncType>();
                if (func->arity() != 1) throw std::runtime_error("Function for map must take exactly one argument.");
                const auto& src_vec = args[1].as<Value::ArrayType>()->elements;
                auto res_arr = std::make_shared<ArrayValue>();
                res_arr->elements.reserve(src_vec.size());
                for (const auto& elem : src_vec) {
                    res_arr->elements.push_back(func->call({elem}));
                }
                return Value(res_arr);
            }, 2, "map"
        )), std::nullopt);
        globalEnv->define("filter", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (!args[0].is<Value::FuncType>()) throw std::runtime_error("First argument to filter must be a function.");
                if (!args[1].is<Value::ArrayType>()) throw std::runtime_error("Second argument to filter must be an array.");
                auto func = args[0].as<Value::FuncType>();
                if (func->arity() != 1) throw std::runtime_error("Function for filter must take exactly one argument.");
                const auto& src_vec = args[1].as<Value::ArrayType>()->elements;
                auto res_arr = std::make_shared<ArrayValue>();
                for (const auto& elem : src_vec) {
                    if (func->call({elem}).toBool()) {
                        res_arr->elements.push_back(elem);
                    }
                }
                return Value(res_arr);
            }, 2, "filter"
        )), std::nullopt);
        globalEnv->define("keys", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (!args[0].is<Value::DictType>()) throw std::runtime_error("Argument to keys() must be a dict.");
                const auto& dict_pairs = args[0].as<Value::DictType>()->pairs;
                auto arr = std::make_shared<ArrayValue>();
                arr->elements.reserve(dict_pairs.size());
                for (const auto& pair : dict_pairs) {
                    arr->elements.push_back(Value(pair.first));
                }
                return Value(arr);
            }, 1, "keys"
        )), std::nullopt);
        globalEnv->define("has", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (!args[1].is<StringData>()) throw std::runtime_error("Second argument to has() must be a string key.");
                const auto& key = args[1].as<StringData>().get();

                if (args[0].is<Value::DictType>()) {
                    const auto& dict = args[0].as<Value::DictType>()->pairs;
                    return Value(dict.count(key) > 0);
                }
                if (args[0].is<Value::MutableObjectType>()) {
                    const auto& obj = args[0].as<Value::MutableObjectType>();
                    return Value(obj->has(key));
                }
                throw std::runtime_error("First argument to has() must be a dict or object.");
            }, 2, "has"
        )), std::nullopt);
        globalEnv->define("del", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (!args[1].is<StringData>()) throw std::runtime_error("Second argument to del() must be a string key.");
                const auto& key = args[1].as<StringData>().get();

                if (args[0].is<Value::DictType>()) {
                    auto& dict = args[0].as<Value::DictType>()->pairs;
                    dict.erase(key);
                    return Value();
                }
                if (args[0].is<Value::MutableObjectType>()) {
                    auto& obj = args[0].as<Value::MutableObjectType>();
                    obj->fields.erase(key);
                    return Value();
                }
                throw std::runtime_error("First argument to del() must be a dict or object.");
            }, 2, "del"
        )), std::nullopt);

        globalEnv->define("deepcopy", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                std::unordered_map<const void*, Value> memo;
                return deepcopy_recursive(args[0], memo);
            }, 1, "deepcopy"
        )), std::nullopt);

        globalEnv->define("Object", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                if (args.size() > 1) {
                    throw std::runtime_error("Object() constructor takes 0 or 1 argument.");
                }
                if (args.empty()) {
                    return Value(std::make_shared<MutableObject>(nullptr));
                }
                if (!args[0].is<Value::MutableObjectType>()) {
                    throw std::runtime_error("Argument to Object() constructor must be another object to act as a prototype.");
                }
                auto parent = args[0].as<Value::MutableObjectType>();
                return Value(std::make_shared<MutableObject>(parent));

            }, -1, "Object"
        )), std::nullopt);

        globalEnv->define("dir", Value(std::make_shared<NativeFunction>(
            [](const std::vector<Value>& args) -> Value {
                 if (args.size() != 1) throw std::runtime_error("dir() takes exactly one argument.");
                 const auto& val = args[0];
                 std::set<std::string> keys;

                 if (val.is<Value::DictType>()) {
                     const auto& dict = val.as<Value::DictType>()->pairs;
                     for(const auto& pair : dict) keys.insert(pair.first);
                 } else if (val.is<Value::MutableObjectType>()) {
                     auto current = val.as<Value::MutableObjectType>();
                     while(current) {
                         for(const auto& pair : current->fields) keys.insert(pair.first);
                         current = current->parent;
                     }
                 } else {
                     throw std::runtime_error("Argument to dir() must be a dict, class instance, or object.");
                 }

                 auto result_arr = std::make_shared<ArrayValue>();
                 for(const auto& key : keys) result_arr->elements.push_back(Value(key));
                 return Value(result_arr);

            }, 1, "dir"
        )), std::nullopt);
    }
};

// ===================================================================
// 9. 主函数 (Main)
// ===================================================================
int main() {
    std::string_view program =
R"CODE(

    # minilang_primes.ml
    func sieve(int n) {
        var is_prime = [];
        for (var i = 0; i <= n; i = i + 1) {
            append(is_prime, true);
        }
        
        is_prime[0] = false;
        is_prime[1] = false;
        
        for (var p = 2; p * p <= n; p = p + 1) {
            if (is_prime[p]) {
                for (var i = p * p; i <= n; i = i + p) {
                    is_prime[i] = false;
                }
            }
        }
        
        int count = 0;
        for (var i = 2; i <= n; i = i + 1) {
            if (is_prime[i]) {
                count = count + 1;
            }
        }
        return count;
    }

    func main() {
        int limit = 1000000;
        print("MiniLang: Calculating primes up to", limit, "...");
        
        int start = clock();
        int prime_count = sieve(limit);
        int elapsed = clock() - start;
        
        print("Found", prime_count, "primes in", elapsed, "ms");
    }

    main();
)CODE";

    try {
        Lexer lexer(program);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto ast = parser.parse();
        if (ast.empty() && !tokens.empty() && tokens[0].type != TokenType::END) {
             std::cerr << "Parsing resulted in an empty AST. Check for parse errors." << std::endl;
        }
        Interpreter interpreter(std::move(ast));
        interpreter.interpret();
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}