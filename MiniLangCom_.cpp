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
#include <cstdlib> 
#include <set>
#include <cstring>
#include <map>

// =================================================================================================
//
//                                PART 1: UNIFIED & COMPLETE BASE CODE
//                          (Lexer, Parser, AST with OO & Closure Support)
//
// =================================================================================================

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
constexpr auto make_overloaded(Ts&&... ts) {
    return overloaded<Ts...>{std::forward<Ts>(ts)...};
}

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
class Callable;
struct BlockStmt;
struct Stmt;
struct Expr;
struct FuncStmt;
struct ClassStmt;
struct MemberAccessExpr; 
struct ThisExpr;
struct SuperExpr;
struct VarExpr;
struct DictLiteralExpr;
struct FuncLiteralExpr;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using StmtList = std::vector<StmtPtr>;

struct ParamInfo {
    std::string name;
    std::optional<TokenType> type;
};

// ===================================================================
// 3. 词法分析器 (Lexer)
// ===================================================================
enum class TokenType {
    // 字面量类型
    ID, INT_LITERAL, FLOAT_LITERAL, STR,
    // 布尔/Null 字面量
    TRUE, FALSE,
    // 操作符
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQ, NE, LT, LE, GT, GE,
    AND, OR, NOT,
    ASSIGN, 
    // 控制流和声明关键字
    IF, ELSE, WHILE, FOR,
    FUNC, RETURN, VAR, BREAK, CONTINUE,
    CLASS, THIS, SUPER, EXTENDS,
    TRY, CATCH, THROW,
    // 标点符号
    LBRACE, RBRACE, LPAREN, RPAREN, COMMA, LBRACKET, RBRACKET, COLON, DOT,
    SEMICOLON,
    // 静态类型关键字
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
            {"try", TokenType::TRY}, {"catch", TokenType::CATCH}, {"throw", TokenType::THROW},
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
// 4. 动态类型系统 (with OO Support)
// ===================================================================
struct Object; // Forward declaration for the base of all class instances
struct NullType {}; 
inline bool operator==(const NullType&, const NullType&) { return true; }
inline bool operator!=(const NullType&, const NullType&) { return false; }

class Callable {
public:
    virtual ~Callable() = default;
    virtual int arity() const = 0;
    virtual Value call(const std::vector<Value>& args) = 0;
    virtual std::string toString() const = 0;
};
class Value {
public:
    using ArrayType = std::shared_ptr<std::vector<Value>>;
    using DictType = std::shared_ptr<std::unordered_map<std::string, Value>>;
    using FuncType = std::shared_ptr<Callable>; 
    using ObjectType = std::shared_ptr<Object>;
    using VariantType = std::variant<NullType, int, double, bool, std::string, FuncType, ArrayType, DictType, ObjectType>;
private:
    VariantType data;
public:
    Value() : data(NullType{}) {}
    Value(NullType) : data(NullType{}) {}
    Value(int v) : data(v) {}
    Value(double v) : data(v) {}
    Value(bool v) : data(v) {}
    Value(const std::string& v) : data(v) {}
    Value(const char* v) : data(std::string(v)) {}
    Value(FuncType v) : data(std::move(v)) {}
    Value(ArrayType v) : data(std::move(v)) {}
    Value(DictType v) : data(std::move(v)) {}
    Value(ObjectType v) : data(std::move(v)) {}

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
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
};

// Base struct for all transpiled class instances
struct Object : public std::enable_shared_from_this<Object> {
    std::unordered_map<std::string, Value> _fields; // For dynamic properties
    virtual ~Object() = default;
    virtual std::string _type_name() const { return "object"; }
};

// ===================================================================
// 5. 抽象语法树 (AST with OO & Closure Support)
// ===================================================================
struct Expr {
    const int line;
    explicit Expr(int ln) : line(ln) {}
    virtual ~Expr() = default;
};
struct Stmt {
    const int line;
    explicit Stmt(int ln) : line(ln) {}
    virtual ~Stmt() = default;
};
struct AssignExpr final : Expr {
    ExprPtr target;
    ExprPtr value;
    AssignExpr(ExprPtr t, ExprPtr v, int ln) : Expr(ln), target(std::move(t)), value(std::move(v)) {}
};
struct LiteralExpr final : Expr {
    Value value;
    explicit LiteralExpr(Value v, int ln) : Expr(ln), value(std::move(v)) {}
};
struct VarExpr final : Expr {
    std::string name;
    explicit VarExpr(std::string n, int ln) : Expr(ln), name(std::move(n)) {}
};
struct UnaryExpr final : Expr {
    Token op;
    ExprPtr expr;
    UnaryExpr(Token o, ExprPtr e, int ln) : Expr(ln), op(std::move(o)), expr(std::move(e)) {}
};
struct BinaryExpr final : Expr {
    Token op;
    ExprPtr left, right;
    BinaryExpr(Token o, ExprPtr l, ExprPtr r, int ln) : Expr(ln), op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
};
struct CallExpr final : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    CallExpr(ExprPtr c, std::vector<ExprPtr> a, int ln) : Expr(ln), callee(std::move(c)), args(std::move(a)) {}
};
struct ArrayLiteralExpr final : Expr {
    std::vector<ExprPtr> elements;
    explicit ArrayLiteralExpr(std::vector<ExprPtr> elems, int ln) : Expr(ln), elements(std::move(elems)) {}
};
struct DictLiteralExpr final : Expr {
    std::vector<std::pair<std::string, ExprPtr>> pairs;
    DictLiteralExpr(std::vector<std::pair<std::string, ExprPtr>> p, int ln)
        : Expr(ln), pairs(std::move(p)) {}
};
struct IndexExpr final : Expr {
    ExprPtr object;
    ExprPtr index;
    IndexExpr(ExprPtr a, ExprPtr i, int ln) : Expr(ln), object(std::move(a)), index(std::move(i)) {}
};
struct MemberAccessExpr final : Expr {
    ExprPtr object;
    Token member;
    MemberAccessExpr(ExprPtr obj, Token mem, int ln)
        : Expr(ln), object(std::move(obj)), member(std::move(mem)) {}
};
struct ThisExpr final : Expr {
    Token keyword;
    ThisExpr(Token kw, int ln) : Expr(ln), keyword(std::move(kw)) {}
};
struct SuperExpr final : Expr {
    Token keyword;
    Token method;
    SuperExpr(Token kw, Token m, int ln) : Expr(ln), keyword(std::move(kw)), method(std::move(m)) {}
};
struct BlockStmt final : Stmt {
    StmtList statements;
    explicit BlockStmt(StmtList stmts, int ln) : Stmt(ln), statements(std::move(stmts)) {}
};
struct ExprStmt final : Stmt {
    ExprPtr expr;
    explicit ExprStmt(ExprPtr e, int ln) : Stmt(ln), expr(std::move(e)) {}
};
struct IfStmt final : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e, int ln) : Stmt(ln), condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
};
struct WhileStmt final : Stmt {
    ExprPtr condition;
    StmtPtr body;
    WhileStmt(ExprPtr c, StmtPtr b, int ln) : Stmt(ln), condition(std::move(c)), body(std::move(b)) {}
};
struct FuncStmt final : Stmt {
    std::string name;
    std::vector<ParamInfo> params;
    std::unique_ptr<BlockStmt> body;
    FuncStmt(std::string n, std::vector<ParamInfo> p, std::unique_ptr<BlockStmt> b, int ln)
        : Stmt(ln), name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
};
struct FuncLiteralExpr final : Expr {
    std::vector<ParamInfo> params;
    std::unique_ptr<BlockStmt> body;
    FuncLiteralExpr(std::vector<ParamInfo> p, std::unique_ptr<BlockStmt> b, int ln)
        : Expr(ln), params(std::move(p)), body(std::move(b)) {}
};
struct ClassStmt final : Stmt {
    std::string name;
    std::optional<std::unique_ptr<VarExpr>> superclass;
    std::vector<std::unique_ptr<FuncStmt>> methods;
    ClassStmt(std::string n, std::optional<std::unique_ptr<VarExpr>> sc, std::vector<std::unique_ptr<FuncStmt>> m, int ln)
        : Stmt(ln), name(std::move(n)), superclass(std::move(sc)), methods(std::move(m)) {}
};
struct ReturnStmt final : Stmt {
    ExprPtr expr; 
    explicit ReturnStmt(ExprPtr e, int ln) : Stmt(ln), expr(std::move(e)) {}
};
struct VarDeclStmt final : Stmt {
    std::string name;
    std::optional<TokenType> type_token;
    ExprPtr initializer;
    VarDeclStmt(std::string n, std::optional<TokenType> tt, ExprPtr init, int ln)
        : Stmt(ln), name(std::move(n)), type_token(tt), initializer(std::move(init)) {}
};
struct ForEachStmt final : Stmt {
    std::string variableName;
    ExprPtr iterable;
    StmtPtr body;
    ForEachStmt(std::string varName, ExprPtr iter, StmtPtr b, int ln)
        : Stmt(ln), variableName(std::move(varName)), iterable(std::move(iter)), body(std::move(b)) {}
};
struct ForStmt final : Stmt {
    StmtPtr initializer;
    ExprPtr condition;
    ExprPtr increment;
    StmtPtr body;
    ForStmt(StmtPtr init, ExprPtr cond, ExprPtr incr, StmtPtr b, int ln)
        : Stmt(ln), initializer(std::move(init)), condition(std::move(cond)), 
          increment(std::move(incr)), body(std::move(b)) {}
};
struct BreakStmt final : Stmt {
    explicit BreakStmt(int ln) : Stmt(ln) {}
};
struct ContinueStmt final : Stmt {
    explicit ContinueStmt(int ln) : Stmt(ln) {}
};
struct ThrowStmt final : Stmt {
    ExprPtr expr;
    explicit ThrowStmt(ExprPtr e, int ln) : Stmt(ln), expr(std::move(e)) {}
};
// FIX: `try_block` and `catch_block` are now explicitly BlockStmt
struct TryStmt final : Stmt {
    std::unique_ptr<BlockStmt> try_block;
    Token catch_variable;
    std::unique_ptr<BlockStmt> catch_block;
    TryStmt(std::unique_ptr<BlockStmt> try_b, Token catch_v, std::unique_ptr<BlockStmt> catch_b, int ln)
        : Stmt(ln), try_block(std::move(try_b)), catch_variable(std::move(catch_v)), catch_block(std::move(catch_b)) {}
};

// Deferred implementation of Value methods
bool Value::toBool() const {
    return std::visit(make_overloaded(
        [](NullType) { return false; },
        [](int v) { return v != 0; },
        [](double v) { return v != 0.0; },
        [](bool v) { return v; },
        [](const std::string& v) { return !v.empty(); },
        [](const FuncType& v) { return v != nullptr; },
        [](const ArrayType& v) { return v && !v->empty(); },
        [](const DictType& v) { return v && !v->empty(); },
        [](const ObjectType& v) { return v != nullptr; }
    ), data);
}
std::string Value::toString() const {
    return std::visit(make_overloaded(
        [](NullType) -> std::string { return std::string("nil"); },
        [](int v) -> std::string { return std::to_string(v); },
        [](double v) -> std::string { 
            std::ostringstream oss;
            oss << v;
            return oss.str();
        },
        [](bool v) -> std::string { return v ? std::string("true") : std::string("false"); },
        [](const std::string& v) -> std::string { return v; },
        [](const FuncType& v) -> std::string { return v ? v->toString() : std::string("<null function>"); },
        [this](const ArrayType& v) -> std::string {
            std::string result = "[";
            if (v) {
                for (size_t i = 0; i < v->size(); ++i) {
                    if (i > 0) result += ", ";
                    result += (*v)[i].toString();
                }
            }
            return result + "]";
        },
        [this](const DictType& v) -> std::string {
            std::string result = "{";
            if (v) {
                bool first = true;
                for (const auto& pair : *v) {
                    if (!first) result += ", ";
                    result += "\"" + pair.first + "\": ";
                    result += pair.second.toString();
                    first = false;
                }
            }
            return result + "}";
        },
        [](const ObjectType& v) -> std::string {
            if (!v) return std::string("nil");
            return "<" + v->_type_name() + " instance>";
        }
    ), data);
}
bool Value::operator==(const Value& other) const {
    if (data.index() != other.data.index()) return false;
    return data == other.data;
}

// ===================================================================
// 7. 语法分析器 (Parser with OO & Closure Support)
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
    
    void synchronize() {
        advance();
        while (!isAtEnd()) {
            if (previous().type == TokenType::SEMICOLON) return;
            switch (peek().type) {
                case TokenType::CLASS: case TokenType::FUNC: case TokenType::VAR: case TokenType::IF:
                case TokenType::WHILE: case TokenType::RETURN: case TokenType::FOR:
                case TokenType::BREAK: case TokenType::CONTINUE: case TokenType::TRY: case TokenType::THROW:
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
            if (check(TokenType::FUNC) && tokens[current + 1].type == TokenType::ID) {
                advance();
                return parseFuncDeclaration("function");
            }
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
    ExprPtr parseFuncLiteral();
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
    StmtPtr parseThrowStatement();
    StmtPtr parseTryStatement();
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
    ExprPtr finishCall(ExprPtr callee);
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

ExprPtr Parser::parseFuncLiteral() {
    int ln = previous().line;
    consume(TokenType::LPAREN, "Expect '(' after 'func' for function literal.");
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
    consume(TokenType::LBRACE, "Expect '{' before function literal body.");
    auto body = parseBlock();
    return std::make_unique<FuncLiteralExpr>(std::move(parameters), std::move(body), ln);
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
        consume(TokenType::FUNC, "Expect 'func' keyword to define a method.");
        methods.push_back(parseFuncDeclaration("method"));
    }

    consume(TokenType::RBRACE, "Expect '}' after class body.");
    return std::make_unique<ClassStmt>(name.lexeme, std::move(superclass), std::move(methods), ln);
}

StmtPtr Parser::parseVarDeclaration(Token type_token) {
    int ln = type_token.line;
    Token name = consume(TokenType::ID, "Expect variable name.");
    std::optional<TokenType> static_type;
    if (type_token.type != TokenType::VAR) { static_type = type_token.type; }
    ExprPtr initializer = nullptr;
    if (match({TokenType::ASSIGN})) { initializer = parseExpression(); }
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
    if (match({TokenType::THROW})) return parseThrowStatement();
    if (match({TokenType::TRY})) return parseTryStatement();
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
    if (match({TokenType::ELSE})) { elseBranch = parseStatement(); }
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
    if (!check(TokenType::SEMICOLON)) { value = parseExpression(); }
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
StmtPtr Parser::parseThrowStatement() {
    int ln = previous().line;
    ExprPtr value = parseExpression();
    consume(TokenType::SEMICOLON, "Expect ';' after throw value.");
    return std::make_unique<ThrowStmt>(std::move(value), ln);
}

// =================================================================================
// ================================ FIXED FUNCTION =================================
// =================================================================================
StmtPtr Parser::parseTryStatement() {
    int ln = previous().line;

    consume(TokenType::LBRACE, "Expect '{' after 'try'.");
    auto try_block = parseBlock();

    consume(TokenType::CATCH, "Expect 'catch' after try block.");
    consume(TokenType::LPAREN, "Expect '(' after 'catch'.");
    Token catch_variable = consume(TokenType::ID, "Expect variable name in catch clause.");
    consume(TokenType::RPAREN, "Expect ')' after catch variable.");

    consume(TokenType::LBRACE, "Expect '{' after catch clause.");
    auto catch_block = parseBlock();

    return std::make_unique<TryStmt>(std::move(try_block), std::move(catch_variable), std::move(catch_block), ln);
}
// =================================================================================
// =================================================================================
// =================================================================================

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

ExprPtr Parser::finishCall(ExprPtr callee) {
    std::vector<ExprPtr> arguments;
    if (!check(TokenType::RPAREN)) {
        do {
            arguments.push_back(parseExpression());
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RPAREN, "Expect ')' after arguments.");
    return std::make_unique<CallExpr>(std::move(callee), std::move(arguments), previous().line);
}

ExprPtr Parser::parseCall() {
    ExprPtr expr = parsePrimary();
    while (true) {
        if (match({TokenType::LPAREN})) {
            expr = finishCall(std::move(expr));
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
    if (match({TokenType::TRUE})) return std::make_unique<LiteralExpr>(Value(true), previous().line);
    if (match({TokenType::FALSE})) return std::make_unique<LiteralExpr>(Value(false), previous().line);
    if (match({TokenType::INT_LITERAL})) return std::make_unique<LiteralExpr>(Value(std::stoi(previous().lexeme)), previous().line);
    if (match({TokenType::FLOAT_LITERAL})) return std::make_unique<LiteralExpr>(Value(std::stod(previous().lexeme)), previous().line);
    if (match({TokenType::STR})) return std::make_unique<LiteralExpr>(Value(previous().lexeme), previous().line);
    if (match({TokenType::THIS})) return std::make_unique<ThisExpr>(previous(), previous().line);
    if (match({TokenType::SUPER})) {
        Token keyword = previous();
        consume(TokenType::DOT, "Expect '.' after 'super'.");
        Token method = consume(TokenType::ID, "Expect superclass method name.");
        return std::make_unique<SuperExpr>(keyword, method, keyword.line);
    }
    if (match({TokenType::FUNC})) {
        return parseFuncLiteral();
    }
    
    if (match({TokenType::ID, TokenType::INT, TokenType::FLOAT, TokenType::BOOL, TokenType::STRING, TokenType::ARRAY, TokenType::DICT, TokenType::OBJECT})) {
        return std::make_unique<VarExpr>(previous().lexeme, previous().line);
    }

    if (match({TokenType::LPAREN})) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after expression.");
        return expr;
    }
    if (match({TokenType::LBRACKET})) {
        return parseArrayLiteral();
    }
    if (match({TokenType::LBRACE})) {
        return parseDictLiteral();
    }

    throw std::runtime_error("Expect expression at line " + std::to_string(peek().line));
}
ExprPtr Parser::parseArrayLiteral() {
    int ln = previous().line;
    std::vector<ExprPtr> elements;
    if (!check(TokenType::RBRACKET)) {
        do { elements.push_back(parseExpression()); } while (match({TokenType::COMMA}));
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
    
    bool is_for_each = false;
    if (check(TokenType::VAR) || check(TokenType::INT) || check(TokenType::FLOAT) || 
        check(TokenType::BOOL) || check(TokenType::STRING) || check(TokenType::ARRAY) || 
        check(TokenType::DICT) || check(TokenType::OBJECT)) {
        if (current + 2 < tokens.size() && tokens[current + 1].type == TokenType::ID && tokens[current + 2].type == TokenType::COLON) {
            is_for_each = true;
        }
    }

    if (is_for_each) {
        advance();
        Token name = consume(TokenType::ID, "Expect variable name in for-each loop.");
        consume(TokenType::COLON, "Expect ':' after variable name in for-each loop.");
        ExprPtr iterable = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after for-each clauses.");
        StmtPtr body = parseStatement();
        return std::make_unique<ForEachStmt>(name.lexeme, std::move(iterable), std::move(body), for_line);
    }
    
    StmtPtr initializer;
    if (match({TokenType::SEMICOLON})) { initializer = nullptr; }
    else if (match({TokenType::VAR, TokenType::INT, TokenType::FLOAT, TokenType::BOOL, TokenType::STRING, TokenType::ARRAY, TokenType::DICT, TokenType::OBJECT})) {
        initializer = parseVarDeclaration(previous());
    }
    else { initializer = parseExprStatement(); }
    
    ExprPtr condition = nullptr;
    if (!check(TokenType::SEMICOLON)) { condition = parseExpression(); }
    consume(TokenType::SEMICOLON, "Expect ';' after for loop condition.");

    ExprPtr increment = nullptr;
    if (!check(TokenType::RPAREN)) { increment = parseExpression(); }
    consume(TokenType::RPAREN, "Expect ')' after for clauses.");
    StmtPtr body = parseStatement();
    return std::make_unique<ForStmt>(std::move(initializer), std::move(condition), std::move(increment), std::move(body), for_line);
}


// =================================================================================================
//
//                                      PART 2: UPGRADED TRANSPILER
//
// =================================================================================================

class UserFunction : public Callable {
    int _arity;
    std::string name;
    std::function<Value(const std::vector<Value>&)> function;
public:
    UserFunction(int arity, std::string name, std::function<Value(const std::vector<Value>&)> func) 
        : _arity(arity), name(std::move(name)), function(std::move(func)) {}
    int arity() const override { return _arity; }
    Value call(const std::vector<Value>& args) override { return function(args); }
    std::string toString() const override { return "<function: " + name + ">"; }
};

const std::string CPP_PRELUDE = R"CPP(
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

// --- Runtime Type System ---
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> constexpr auto make_overloaded(Ts&&... ts) { return overloaded<Ts...>{std::forward<Ts>(ts)...}; }
class Value;
class Callable;
struct Object;
struct NullType {};
inline bool operator==(const NullType&, const NullType&) { return true; }
inline bool operator!=(const NullType&, const NullType&) { return false; }

class Value {
public:
    using ArrayType = std::shared_ptr<std::vector<Value>>; using DictType = std::shared_ptr<std::unordered_map<std::string, Value>>; using FuncType = std::shared_ptr<Callable>; using ObjectType = std::shared_ptr<Object>; using VariantType = std::variant<NullType, int, double, bool, std::string, FuncType, ArrayType, DictType, ObjectType>;
private: VariantType data;
public:
    Value() : data(NullType{}) {} Value(NullType) : data(NullType{}) {} Value(int v) : data(v) {} Value(double v) : data(v) {} Value(bool v) : data(v) {} Value(const std::string& v) : data(v) {} Value(const char* v) : data(std::string(v)) {} Value(FuncType v) : data(std::move(v)) {} Value(ArrayType v) : data(std::move(v)) {} Value(DictType v) : data(std::move(v)) {} Value(ObjectType v) : data(std::move(v)) {}
    template <typename T> bool is() const { return std::holds_alternative<T>(data); }
    template <typename T> const T& as() const { if (auto* val = std::get_if<T>(&data)) return *val; throw std::runtime_error("Invalid type cast in Value::as()"); }
    template <typename T> T& as() { if (auto* val = std::get_if<T>(&data)) return *val; throw std::runtime_error("Invalid type cast in Value::as()"); }
    const VariantType& getVariant() const { return data; }
    bool toBool() const;
    std::string toString() const;
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
};

class _ThrowSignal : public std::exception {
public:
    const Value thrown_value;
    explicit _ThrowSignal(Value val) : thrown_value(std::move(val)) {}
};

struct Object : public std::enable_shared_from_this<Object> { std::unordered_map<std::string, Value> _fields; virtual ~Object() = default; virtual std::string _type_name() const { return "object"; } };
class Callable { public: virtual ~Callable() = default; virtual int arity() const = 0; virtual Value call(const std::vector<Value>& args) = 0; virtual std::string toString() const = 0; };

bool Value::toBool() const { return std::visit(make_overloaded([](NullType){ return false; }, [](int v) { return v != 0; }, [](double v) { return v != 0.0; }, [](bool v) { return v; }, [](const std::string& v) { return !v.empty(); }, [](const FuncType& v) { return v != nullptr; }, [](const ArrayType& v) { return v && !v->empty(); }, [](const DictType& v) { return v && !v->empty(); }, [](const ObjectType& v){ return v != nullptr; }), data); }
std::string Value::toString() const { return std::visit(make_overloaded([](NullType) -> std::string { return std::string("nil"); }, [](int v) -> std::string { return std::to_string(v); }, [](double v) -> std::string { std::ostringstream oss; oss << v; return oss.str(); }, [](bool v) -> std::string { return v ? std::string("true") : std::string("false"); }, [](const std::string& v) -> std::string { return v; }, [](const FuncType& v) -> std::string { return v ? v->toString() : std::string("<null function>"); }, [this](const ArrayType& v) -> std::string { std::string result = "["; if (v) { for (size_t i = 0; i < v->size(); ++i) { if (i > 0) result += ", "; result += (*v)[i].toString(); } } return result + "]"; }, [this](const DictType& v) -> std::string { std::string result = "{"; if (v) { bool first = true; for (const auto& pair : *v) { if (!first) result += ", "; result += "\"" + pair.first + "\": "; result += pair.second.toString(); first = false; } } return result + "}"; }, [](const ObjectType& v){ if (!v) return std::string("nil"); return "<" + v->_type_name() + " instance>"; }), data); }
bool Value::operator==(const Value& other) const { if (data.index() != other.data.index()) return false; return data == other.data; }

class NativeFunction : public Callable {
public:
    using NativeFn = std::function<Value(const std::vector<Value>&)>;
private:
    NativeFn function;
    int _arity;
    std::string name;
public:
    NativeFunction(NativeFn fn, int arity, std::string name) : function(std::move(fn)), _arity(arity), name(std::move(name)) {}
    int arity() const override { return _arity; }
    Value call(const std::vector<Value>& args) override { return function(args); }
    std::string toString() const override { return "<native function: " + name + ">"; }
};
class UserFunction : public Callable { int _arity; std::string name; std::function<Value(const std::vector<Value>&)> function; public: UserFunction(int arity, std::string name, std::function<Value(const std::vector<Value>&)> func) : _arity(arity), name(std::move(name)), function(std::move(func)) {} int arity() const override { return _arity; } Value call(const std::vector<Value>& args) override { return function(args); } std::string toString() const override { return "<function: " + name + ">"; } };

const Value _V_TRUE(true);
const Value _V_FALSE(false);
const Value _V_NULL(NullType{});

// --- Runtime Operator Helpers ---
Value _op_add(const Value& l, const Value& r) {
    if (l.is<std::string>() || r.is<std::string>()) return Value(l.toString() + r.toString());
    if (l.is<Value::ArrayType>() && r.is<Value::ArrayType>()) { auto new_arr = std::make_shared<std::vector<Value>>(*l.as<Value::ArrayType>()); new_arr->insert(new_arr->end(), r.as<Value::ArrayType>()->begin(), r.as<Value::ArrayType>()->end()); return Value(new_arr); }
    if ((l.is<double>() || l.is<int>()) && (r.is<double>() || r.is<int>())) {
        double L = l.is<double>() ? l.as<double>() : static_cast<double>(l.as<int>());
        double R = r.is<double>() ? r.as<double>() : static_cast<double>(r.as<int>());
        if (l.is<int>() && r.is<int>()) return Value(l.as<int>() + r.as<int>());
        return Value(L + R);
    }
    throw std::runtime_error("Unsupported operands for +");
}
Value _op_binary(const Value& l, const Value& r, char op) {
    if ((l.is<double>() || l.is<int>()) && (r.is<double>() || r.is<int>())) {
        double L = l.is<double>() ? l.as<double>() : static_cast<double>(l.as<int>());
        double R = r.is<double>() ? r.as<double>() : static_cast<double>(r.as<int>());
        switch(op) {
            case '-': return (l.is<int>() && r.is<int>()) ? Value(l.as<int>()-r.as<int>()) : Value(L-R);
            case '*': return (l.is<int>() && r.is<int>()) ? Value(l.as<int>()*r.as<int>()) : Value(L*R);
            case '/': if (R == 0.0) throw std::runtime_error("Division by zero"); return Value(L / R);
            case '%': if (!l.is<int>() || !r.is<int>()) throw std::runtime_error("Operands of % must be integers"); if (r.as<int>() == 0) throw std::runtime_error("Modulo by zero"); return Value(l.as<int>() % r.as<int>());
            case '<': return Value(L < R); case '>': return Value(L > R);
            case 'L': return Value(L <= R); case 'G': return Value(L >= R);
        }
    }
    if (l.is<std::string>() && r.is<std::string>()) {
        const auto& L = l.as<std::string>(); const auto& R = r.as<std::string>();
        switch(op) { case '<': return Value(L < R); case '>': return Value(L > R); case 'L': return Value(L <= R); case 'G': return Value(L >= R); }
    }
    throw std::runtime_error(std::string("Unsupported operands for binary operator ") + op);
}
Value _op_eq(const Value& l, const Value& r) {
    if (l.is<NullType>() || r.is<NullType>()) return Value(l.is<NullType>() && r.is<NullType>());
    if ((l.is<double>() || l.is<int>()) && (r.is<double>() || r.is<int>())) {
        double L = l.is<double>() ? l.as<double>() : static_cast<double>(l.as<int>());
        double R = r.is<double>() ? r.as<double>() : static_cast<double>(r.as<int>());
        return Value(L == R);
    }
    return Value(l == r);
}
Value _op_neq(const Value& l, const Value& r) { return Value(!_op_eq(l, r).toBool()); }
Value _op_unary_minus(const Value& v) { if(v.is<int>()) return Value(-v.as<int>()); if(v.is<double>()) return Value(-v.as<double>()); throw std::runtime_error("Operand for unary minus must be a number."); }
Value _op_not(const Value& v) { return Value(!v.toBool()); }
Value _op_index_get(const Value& obj, const Value& idx) {
    if (obj.is<Value::ArrayType>() && idx.is<int>()) { const auto& arr = *obj.as<Value::ArrayType>(); int i = idx.as<int>(); if (i >= 0 && (size_t)i < arr.size()) return arr[i]; throw std::runtime_error("Array index out of bounds"); }
    if (obj.is<std::string>() && idx.is<int>()) { const auto& str = obj.as<std::string>(); int i = idx.as<int>(); if (i >= 0 && (size_t)i < str.length()) return Value(std::string(1, str[i])); throw std::runtime_error("String index out of bounds"); }
    if (obj.is<Value::DictType>() && idx.is<std::string>()) { const auto& dict = *obj.as<Value::DictType>(); const auto& key = idx.as<std::string>(); auto it = dict.find(key); if (it != dict.end()) return it->second; throw std::runtime_error("Undefined property '" + key + "'."); }
    throw std::runtime_error("Value is not indexable or index type is wrong.");
}
Value& _op_index_set(Value& obj, const Value& idx, const Value& val) {
    if (obj.is<Value::ArrayType>() && idx.is<int>()) { auto& arr = *obj.as<Value::ArrayType>(); int i = idx.as<int>(); if (i >= 0 && (size_t)i < arr.size()) { return arr[i] = val; } throw std::runtime_error("Array index out of bounds"); }
    if (obj.is<Value::DictType>() && idx.is<std::string>()) { return (*obj.as<Value::DictType>())[idx.as<std::string>()] = val; }
    if (obj.is<std::string>() && idx.is<int>()) { if (!val.is<std::string>() || val.as<std::string>().length() != 1) throw std::runtime_error("Can only assign a single-character string to a string index."); auto& str = obj.as<std::string>(); int i = idx.as<int>(); if (i >= 0 && (size_t)i < str.length()) { str[i] = val.as<std::string>()[0]; return obj; } throw std::runtime_error("String index out of bounds for assignment."); }
    throw std::runtime_error("Value cannot be assigned by index or index type is wrong.");
}
Value _get_member(const Value& obj, const std::string& key) {
    if (!obj.is<Value::ObjectType>()) throw std::runtime_error("Can only access properties on objects.");
    auto instance = obj.as<Value::ObjectType>();
    if (instance->_fields.count(key)) return instance->_fields.at(key);
    throw std::runtime_error("Undefined property '" + key + "'.");
}
Value _set_member(const Value& obj, const std::string& key, const Value& val) {
    if (!obj.is<Value::ObjectType>()) throw std::runtime_error("Can only set properties on objects.");
    auto instance = obj.as<Value::ObjectType>();
    return instance->_fields[key] = val;
}
Value _call(const Value& callee, const std::vector<Value>& args) {
    if (!callee.is<Value::FuncType>()) throw std::runtime_error("Can only call functions.");
    auto func = callee.as<Value::FuncType>();
    if (func->arity() != -1 && (size_t)func->arity() != args.size()) throw std::runtime_error("Expected " + std::to_string(func->arity()) + " args but got " + std::to_string(args.size()));
    return func->call(args);
}
Value deepcopy_recursive(const Value& val, std::unordered_map<const void*, Value>& memo) {
    return std::visit(make_overloaded(
        [&](NullType) { return Value(NullType{}); },
        [&](int v) { return Value(v); },
        [&](double v) { return Value(v); },
        [&](bool v) { return Value(v); },
        [&](const std::string& v) { return Value(v); },
        [&](const Value::FuncType& v) { return Value(v); },
        [&](const Value::ArrayType& arr) -> Value {
            const void* ptr = arr.get(); if (memo.count(ptr)) return memo.at(ptr);
            auto newArr = std::make_shared<std::vector<Value>>(); Value newArrVal(newArr); memo[ptr] = newArrVal;
            newArr->reserve(arr->size()); for (const auto& elem : *arr) { newArr->push_back(deepcopy_recursive(elem, memo)); }
            return newArrVal;
        },
        [&](const Value::DictType& dict) -> Value {
            const void* ptr = dict.get(); if (memo.count(ptr)) return memo.at(ptr);
            auto newDict = std::make_shared<std::unordered_map<std::string, Value>>(); Value newDictVal(newDict); memo[ptr] = newDictVal;
            for (const auto& pair : *dict) { (*newDict)[pair.first] = deepcopy_recursive(pair.second, memo); }
            return newDictVal;
        },
        [&](const Value::ObjectType& obj) -> Value {
            const void* ptr = obj.get(); if (memo.count(ptr)) return memo.at(ptr);
            throw std::runtime_error("Deepcopy for class instances is not supported in this version.");
            return Value(NullType{});
        }
    ), val.getVariant());
}

// FORWARD_DECLARATIONS_AND_CLASS_DEFINITIONS

// --- Main Entry Point & Global Environment ---
static const auto _start_time = std::chrono::high_resolution_clock::now();
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    auto _global_env = std::make_shared<std::unordered_map<std::string, Value>>();
    try {
        // Core functions
        (*_global_env)["print"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { for (size_t i = 0; i < args.size(); ++i) { std::cout << args[i].toString() << (i < args.size() - 1 ? " " : ""); } std::cout << std::endl; return _V_NULL; }, -1, "print"));
        (*_global_env)["len"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { const auto& v = args[0]; if(v.is<std::string>()) return Value((int)v.as<std::string>().length()); if(v.is<Value::ArrayType>()) return Value((int)v.as<Value::ArrayType>()->size()); if(v.is<Value::DictType>()) return Value((int)v.as<Value::DictType>()->size()); throw std::runtime_error("Value has no length."); }, 1, "len"));
        (*_global_env)["type"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { return std::visit(make_overloaded([](NullType){return Value("nil");}, [](int){return Value("int");}, [](double){return Value("float");}, [](bool){return Value("bool");}, [](const std::string&){return Value("string");}, [](const Value::ArrayType&){return Value("array");}, [](const Value::DictType&){return Value("dict");}, [](const Value::FuncType&){return Value("function");}, [](const Value::ObjectType& o){ if(!o) return Value("nil"); return Value(o->_type_name()); }), args[0].getVariant()); }, 1, "type"));
        (*_global_env)["assert"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { if (args.size() != 1 && args.size() != 2) throw std::runtime_error("assert() takes 1 or 2 arguments."); if (!args[0].toBool()) { std::string msg = "Assertion failed."; if (args.size() == 2) msg += " " + args[1].toString(); throw std::runtime_error(msg); } return _V_NULL; }, -1, "assert"));
        // Type conversion functions
        (*_global_env)["str"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { return Value(args[0].toString()); }, 1, "str"));
        (*_global_env)["int"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { const auto& val = args[0]; return std::visit(make_overloaded([](int i) { return Value(i); }, [](double d) { return Value(static_cast<int>(d)); }, [](bool b) { return Value(static_cast<int>(b)); }, [](const std::string& s) -> Value { try { return Value(std::stoi(s)); } catch (...) { throw std::runtime_error("Cannot convert string '" + s + "' to int."); } }, [](const auto&) -> Value { throw std::runtime_error("Cannot convert type to float."); }), val.getVariant()); }, 1, "int"));
        (*_global_env)["bool"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { return Value(args[0].toBool()); }, 1, "bool"));
        (*_global_env)["float"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { const auto& val = args[0]; return std::visit(make_overloaded([](int i) { return Value(static_cast<double>(i)); }, [](double d) { return Value(d); }, [](const std::string& s) -> Value { try { return Value(std::stod(s)); } catch (...) { throw std::runtime_error("Cannot convert string '" + s + "' to float."); } }, [](const auto&) -> Value { throw std::runtime_error("Cannot convert type to float."); }), val.getVariant()); }, 1, "float"));
        // Array/String functions
        (*_global_env)["append"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { if (!args[0].is<Value::ArrayType>()) throw std::runtime_error("First argument to append must be an array."); args[0].as<Value::ArrayType>()->push_back(args[1]); return _V_NULL; }, 2, "append"));
        (*_global_env)["pop"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { if (args.size() != 1 && args.size() != 2) throw std::runtime_error("pop() takes 1 or 2 arguments."); if (!args[0].is<Value::ArrayType>()) throw std::runtime_error("First argument to pop must be an array."); auto& vec = *args[0].as<Value::ArrayType>(); if (vec.empty()) throw std::runtime_error("pop from empty array."); if (args.size() == 1) { Value back = vec.back(); vec.pop_back(); return back; } else { if (!args[1].is<int>()) throw std::runtime_error("Index for pop must be an integer."); int idx = args[1].as<int>(); if (idx < 0 || (size_t)idx >= vec.size()) throw std::runtime_error("pop index out of range."); Value val = vec[idx]; vec.erase(vec.begin() + idx); return val; } }, -1, "pop"));
        (*_global_env)["slice"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { if (args.size() < 2 || args.size() > 3) throw std::runtime_error("slice() takes 2 or 3 arguments."); int start_idx = args[1].as<int>(); int end_idx; if (args[0].is<Value::ArrayType>()) { const auto& src = *args[0].as<Value::ArrayType>(); end_idx = args.size() == 3 ? args[2].as<int>() : src.size(); if (start_idx < 0 || end_idx > (int)src.size() || start_idx > end_idx) throw std::runtime_error("Slice indices out of bounds."); auto new_arr = std::make_shared<std::vector<Value>>(src.begin() + start_idx, src.begin() + end_idx); return Value(new_arr); } if (args[0].is<std::string>()) { const auto& src = args[0].as<std::string>(); end_idx = args.size() == 3 ? args[2].as<int>() : src.length(); if (start_idx < 0 || end_idx > (int)src.length() || start_idx > end_idx) throw std::runtime_error("Slice indices out of bounds."); return Value(src.substr(start_idx, end_idx - start_idx)); } throw std::runtime_error("First argument to slice must be an array or string."); }, -1, "slice"));
        (*_global_env)["range"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { if (args.empty() || args.size() > 3) throw std::runtime_error("range() takes 1, 2, or 3 arguments."); int start = 0, end = 0, step = 1; if (args.size() == 1) { end = args[0].as<int>(); } else { start = args[0].as<int>(); end = args[1].as<int>(); if (args.size() == 3) step = args[2].as<int>(); } auto arr = std::make_shared<std::vector<Value>>(); if (step > 0) for (int i = start; i < end; i += step) arr->push_back(Value(i)); else for (int i = start; i > end; i += step) arr->push_back(Value(i)); return Value(arr); }, -1, "range"));
        // Dict/Object functions
        (*_global_env)["dict"] = Value(std::make_shared<NativeFunction>([]([[maybe_unused]] const std::vector<Value>& args) -> Value { if (!args.empty()) throw std::runtime_error("dict() takes no arguments."); return Value(std::make_shared<std::unordered_map<std::string, Value>>()); }, 0, "dict"));
        (*_global_env)["keys"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { if (!args[0].is<Value::DictType>()) throw std::runtime_error("Argument to keys() must be a dict."); const auto& dict = *args[0].as<Value::DictType>(); auto arr = std::make_shared<std::vector<Value>>(); arr->reserve(dict.size()); for (const auto& pair : dict) { arr->push_back(Value(pair.first)); } return Value(arr); }, 1, "keys"));
        (*_global_env)["has"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { if (!args[1].is<std::string>()) throw std::runtime_error("Second argument must be a string key."); const auto& key = args[1].as<std::string>(); if (args[0].is<Value::DictType>()) return Value(args[0].as<Value::DictType>()->count(key) > 0); if (args[0].is<Value::ObjectType>()) return Value(args[0].as<Value::ObjectType>()->_fields.count(key) > 0); throw std::runtime_error("First argument to has() must be a dict or object."); }, 2, "has"));
        (*_global_env)["del"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { if (!args[1].is<std::string>()) throw std::runtime_error("Second argument must be a string key."); const auto& key = args[1].as<std::string>(); if (args[0].is<Value::DictType>()) { args[0].as<Value::DictType>()->erase(key); return _V_NULL; } if (args[0].is<Value::ObjectType>()) { args[0].as<Value::ObjectType>()->_fields.erase(key); return _V_NULL; } throw std::runtime_error("First argument to del() must be a dict or object."); }, 2, "del"));
        (*_global_env)["dir"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { if (args.size() != 1) throw std::runtime_error("dir() takes one argument."); std::set<std::string> keys; if (args[0].is<Value::DictType>()) { for(const auto& p : *args[0].as<Value::DictType>()) keys.insert(p.first); } else if (args[0].is<Value::ObjectType>()) { for(const auto& p : args[0].as<Value::ObjectType>()->_fields) keys.insert(p.first); } else { throw std::runtime_error("Argument to dir() must be a dict or object."); } auto arr = std::make_shared<std::vector<Value>>(); for(const auto& k : keys) arr->push_back(Value(k)); return Value(arr); }, 1, "dir"));
        // Functional
        (*_global_env)["map"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { auto func = args[0].as<Value::FuncType>(); const auto& src = *args[1].as<Value::ArrayType>(); auto res = std::make_shared<std::vector<Value>>(); res->reserve(src.size()); for(const auto& e : src) res->push_back(func->call({e})); return Value(res); }, 2, "map"));
        (*_global_env)["filter"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { auto func = args[0].as<Value::FuncType>(); const auto& src = *args[1].as<Value::ArrayType>(); auto res = std::make_shared<std::vector<Value>>(); for(const auto& e : src) if(func->call({e}).toBool()) res->push_back(e); return Value(res); }, 2, "filter"));
        (*_global_env)["deepcopy"] = Value(std::make_shared<NativeFunction>([](const std::vector<Value>& args) -> Value { std::unordered_map<const void*, Value> memo; return deepcopy_recursive(args[0], memo); }, 1, "deepcopy"));
        
        {
// MAIN_CODE_GOES_HERE
        }
    } catch (const _ThrowSignal& signal) {
        std::cerr << "Unhandled Exception: " << signal.thrown_value.toString() << std::endl; return 1;
    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl; return 1;
    }
    return 0;
}
)CPP";

class Transpiler {
    struct VarInfo {
        std::string type_name = "Value";
        bool is_heap_allocated = false;
    };
    using TypeMap = std::map<std::string, VarInfo>;

    std::set<std::string> class_names;
    std::map<std::string, const ClassStmt*> class_nodes;

    std::string compile(const Stmt& s, TypeMap& types, bool is_in_function_scope, bool is_method = false, const std::string& class_name = "");
    std::string compile(const Expr& e, TypeMap& types, bool is_in_function_scope, bool is_method = false, const std::string& class_name = "");
    std::string compile_function_body(const std::vector<ParamInfo>& params, const BlockStmt& body, const std::string& func_name, TypeMap& types, bool is_in_function_scope, bool is_method, const std::string& class_name);
    
public:
    std::string transpile(const StmtList& ast) {
        std::stringstream ss_global;
        std::stringstream ss_main;
        TypeMap globalTypes;
        
        for (const auto& s : ast) {
            if (!s) continue;
            if (auto p = dynamic_cast<const VarDeclStmt*>(s.get())) {
                globalTypes[p->name] = {"Value", false};
            } else if (auto p = dynamic_cast<const FuncStmt*>(s.get())) {
                globalTypes[p->name] = {"Function", false};
            } else if (auto p = dynamic_cast<const ClassStmt*>(s.get())) {
                class_names.insert(p->name);
                class_nodes[p->name] = p;
                globalTypes[p->name] = {p->name, false};
            }
        }
        
        for (const auto& name : class_names) {
            ss_global << "struct _class_" << name << ";\n";
        }

        for (const auto& s : ast) {
            if (s && dynamic_cast<const ClassStmt*>(s.get())) {
                ss_global << compile(*s, globalTypes, false) << "\n";
            }
        }

        for (const auto& s : ast) {
            if(s && !dynamic_cast<const ClassStmt*>(s.get())) {
                ss_main << "        " << compile(*s, globalTypes, false) << "\n";
            }
        }
        
        std::string final_code = CPP_PRELUDE;
        auto placeholder_pos = final_code.find("// FORWARD_DECLARATIONS_AND_CLASS_DEFINITIONS");
        if(placeholder_pos != std::string::npos) {
            final_code.replace(placeholder_pos, strlen("// FORWARD_DECLARATIONS_AND_CLASS_DEFINITIONS"), ss_global.str());
        }
        placeholder_pos = final_code.find("// MAIN_CODE_GOES_HERE");
        if(placeholder_pos != std::string::npos) {
            final_code.replace(placeholder_pos, strlen("// MAIN_CODE_GOES_HERE"), ss_main.str());
        }
        return final_code;
    }
private:
    std::string sanitize(const std::string& n) { return "_var_" + n; }
};

std::string Transpiler::compile(const Stmt& s, TypeMap& types, bool is_in_function_scope, bool is_method, const std::string& class_name) {
    if (auto p = dynamic_cast<const ExprStmt*>(&s)) {
        return compile(*p->expr, types, is_in_function_scope, is_method, class_name) + ";";
    }
    if (auto p = dynamic_cast<const VarDeclStmt*>(&s)) {
        std::stringstream ss;
        std::string var_type = "Value";
        if (p->initializer) {
            if (auto* call = dynamic_cast<const CallExpr*>(p->initializer.get())) {
                if (auto* callee = dynamic_cast<const VarExpr*>(call->callee.get())) {
                    if (class_names.count(callee->name)) {
                        var_type = callee->name;
                    }
                }
            } else if (dynamic_cast<const FuncLiteralExpr*>(p->initializer.get())) {
                var_type = "Function";
            }
        }
        
        if (is_in_function_scope) {
            types[p->name] = {var_type, true};
            ss << "auto " << sanitize(p->name) << " = std::make_shared<Value>(";
            if (p->initializer) {
                ss << compile(*p->initializer, types, is_in_function_scope, is_method, class_name);
            } else {
                ss << "_V_NULL";
            }
            ss << ");";
        } else {
            types[p->name] = {var_type, false};
            ss << "Value " << sanitize(p->name);
            if (p->initializer) {
                ss << " = " << compile(*p->initializer, types, is_in_function_scope, is_method, class_name) << ";";
            } else {
                ss << " = _V_NULL;";
            }
        }
        return ss.str();
    }
    if (auto p = dynamic_cast<const BlockStmt*>(&s)) {
        std::stringstream ss;
        TypeMap blockTypes = types;
        ss << "{\n";
        for (const auto& st : p->statements) {
            if (st) ss << "            " << compile(*st, blockTypes, is_in_function_scope, is_method, class_name) << "\n";
        }
        ss << "        }";
        return ss.str();
    }
    if (auto p = dynamic_cast<const IfStmt*>(&s)) {
         std::string else_str = p->elseBranch ? " else " + compile(*p->elseBranch, types, is_in_function_scope, is_method, class_name) : "";
         return "if ((" + compile(*p->condition, types, is_in_function_scope, is_method, class_name) + ").toBool()) " + compile(*p->thenBranch, types, is_in_function_scope, is_method, class_name) + else_str;
    }
    if (auto p = dynamic_cast<const WhileStmt*>(&s)) {
        return "while ((" + compile(*p->condition, types, is_in_function_scope, is_method, class_name) + ").toBool()) " + compile(*p->body, types, is_in_function_scope, is_method, class_name);
    }
    if (auto p = dynamic_cast<const ForStmt*>(&s)) {
        std::stringstream ss;
        TypeMap for_scope_types = types;
        ss << "{\n";
        if (p->initializer) {
            ss << "            " << compile(*p->initializer, for_scope_types, is_in_function_scope, is_method, class_name) << "\n";
        }
        ss << "            for (; "
           << (p->condition ? "(" + compile(*p->condition, for_scope_types, is_in_function_scope, is_method, class_name) + ").toBool()" : "true") << "; "
           << (p->increment ? compile(*p->increment, for_scope_types, is_in_function_scope, is_method, class_name) : "") << ") "
           << compile(*p->body, for_scope_types, is_in_function_scope, is_method, class_name) << "\n";
        ss << "        }";
        return ss.str();
    }
    if (auto p = dynamic_cast<const ForEachStmt*>(&s)) {
        std::stringstream ss;
        std::string iter_var = "_iter_" + p->variableName;
        std::string item_var_sanitized = sanitize(p->variableName);
        
        TypeMap body_scope_types = types;
        body_scope_types[p->variableName] = {"Value", true};

        ss << "{\n";
        ss << "            auto " << iter_var << " = " << compile(*p->iterable, types, is_in_function_scope, is_method, class_name) << ";\n";
        ss << "            if (" << iter_var << ".is<Value::ArrayType>()) {\n";
        ss << "                for (const auto& _item : *(" << iter_var << ".as<Value::ArrayType>())) {\n";
        ss << "                    auto " << item_var_sanitized << " = std::make_shared<Value>(_item);\n";
        ss << "                    " << compile(*p->body, body_scope_types, is_in_function_scope, is_method, class_name) << "\n";
        ss << "                }\n";
        ss << "            } else if (" << iter_var << ".is<std::string>()) {\n";
        ss << "                for (const char& _c : " << iter_var << ".as<std::string>()) {\n";
        ss << "                    auto " << item_var_sanitized << " = std::make_shared<Value>(Value(std::string(1, _c)));\n";
        ss << "                    " << compile(*p->body, body_scope_types, is_in_function_scope, is_method, class_name) << "\n";
        ss << "                }\n";
        ss << "            } else {\n";
        ss << "                throw std::runtime_error(\"Value is not iterable. Can only iterate over arrays and strings.\");\n";
        ss << "            }\n";
        ss << "        }";
        return ss.str();
    }
    if (auto p = dynamic_cast<const FuncStmt*>(&s)) {
        std::stringstream ss;
        types[p->name] = {"Function", false};
        ss << "Value " << sanitize(p->name) << " = " << compile_function_body(p->params, *p->body, p->name, types, true, is_method, class_name) << ";";
        return ss.str();
    }
    if (auto p = dynamic_cast<const ClassStmt*>(&s)) {
        std::stringstream ss;
        ss << "struct _class_" << p->name << " : public " << (p->superclass ? "_class_" + (*p->superclass)->name : "Object") << " {\n";
        
        ss << "    std::string _type_name() const override { return \"" << p->name << "\"; }\n";

        const FuncStmt* init_method = nullptr;
        for (const auto& method : p->methods) {
            if (method->name == "init") {
                init_method = method.get();
                break;
            }
        }

        if (init_method) {
            ss << "    _class_" << p->name << "(";
            for (size_t i = 0; i < init_method->params.size(); ++i) {
                ss << "Value " << sanitize(init_method->params[i].name) << (i < init_method->params.size() - 1 ? ", " : "");
            }
            ss << ")";

            bool has_super_call = false;
            if (p->superclass) {
                if (!init_method->body->statements.empty()) {
                    if (auto* es = dynamic_cast<ExprStmt*>(init_method->body->statements[0].get())) {
                        if (auto* call = dynamic_cast<CallExpr*>(es->expr.get())) {
                            if (auto* super = dynamic_cast<SuperExpr*>(call->callee.get())) {
                                if (super->method.lexeme == "init") {
                                    has_super_call = true;
                                    ss << " : _class_" << (*p->superclass)->name << "(";
                                    TypeMap temp_types = types;
                                    for(const auto& param : init_method->params) temp_types[param.name] = {"Value", false};
                                    for (size_t i = 0; i < call->args.size(); ++i) {
                                        ss << compile(*call->args[i], temp_types, true, true, p->name) << (i < call->args.size() - 1 ? ", " : "");
                                    }
                                    ss << ")";
                                }
                            }
                        }
                    }
                }
            }
            ss << " {\n";
            TypeMap methodTypes = types;
            for(const auto& param : init_method->params) methodTypes[param.name] = {"Value", false};
            for (size_t i = (has_super_call ? 1 : 0); i < init_method->body->statements.size(); ++i) {
                ss << "        " << compile(*init_method->body->statements[i], methodTypes, true, true, p->name) << "\n";
            }
            ss << "    }\n";
        } else {
             ss << "    _class_" << p->name << "() {}\n";
        }

        for (const auto& method : p->methods) {
            if (method->name == "init") continue;
            ss << "    Value " << method->name << "(";
            for (size_t i = 0; i < method->params.size(); ++i) {
                ss << "Value " << sanitize(method->params[i].name) << (i < method->params.size() - 1 ? ", " : "");
            }
            ss << ") {\n";
            TypeMap methodTypes = types;
            for(const auto& param : method->params) methodTypes[param.name] = {"Value", false};
            ss << "        " << compile(*method->body, methodTypes, true, true, p->name) << "\n";
            ss << "        return _V_NULL;\n";
            ss << "    }\n";
        }

        ss << "};";
        return ss.str();
    }
    if (auto p = dynamic_cast<const ReturnStmt*>(&s)) {
        return p->expr ? "return " + compile(*p->expr, types, is_in_function_scope, is_method, class_name) + ";" : "return _V_NULL;";
    }
    if (dynamic_cast<const BreakStmt*>(&s)) return "break;";
    if (dynamic_cast<const ContinueStmt*>(&s)) return "continue;";
    if (auto p = dynamic_cast<const ThrowStmt*>(&s)) {
        return "throw _ThrowSignal(" + compile(*p->expr, types, is_in_function_scope, is_method, class_name) + ");";
    }
    if (auto p = dynamic_cast<const TryStmt*>(&s)) {
        std::stringstream ss;
        ss << "try " << compile(*p->try_block, types, is_in_function_scope, is_method, class_name);
        ss << " catch (const _ThrowSignal& _signal) ";
        
        TypeMap catch_types = types;
        catch_types[p->catch_variable.lexeme] = {"Value", true};

        ss << "{\n";
        ss << "            auto " << sanitize(p->catch_variable.lexeme) << " = std::make_shared<Value>(_signal.thrown_value);\n";
        ss << "            " << compile(*p->catch_block, catch_types, is_in_function_scope, is_method, class_name) << "\n";
        ss << "        }";
        return ss.str();
    }
    
    return "// Unsupported Stmt\n";
}

std::string Transpiler::compile(const Expr& e, TypeMap& types, bool is_in_function_scope, bool is_method, const std::string& class_name) {
    if (auto p = dynamic_cast<const LiteralExpr*>(&e)) {
        std::stringstream ss;
        std::visit(make_overloaded(
            [&](NullType){ ss << "_V_NULL"; },
            [&](int v){ ss << "Value(" << v << ")"; },
            [&](double v){ ss << "Value(" << v << ")"; },
            [&](bool v){ ss << (v ? "_V_TRUE" : "_V_FALSE"); },
            [&](const std::string& v){
                std::string escaped;
                for(char c : v) {
                    switch(c) {
                        case '"':  escaped += "\\\""; break; case '\\': escaped += "\\\\"; break;
                        case '\n': escaped += "\\n"; break; case '\t': escaped += "\\t"; break;
                        case '\r': escaped += "\\r"; break; default: escaped += c;
                    }
                }
                ss << "Value(std::string(\"" << escaped << "\"))";
            },
            [&](const auto&){ ss << "Value()"; }
        ), p->value.getVariant());
        return ss.str();
    }
    if (auto p = dynamic_cast<const BinaryExpr*>(&e)) {
        std::string l_str = compile(*p->left, types, is_in_function_scope, is_method, class_name);
        std::string r_str = compile(*p->right, types, is_in_function_scope, is_method, class_name);
         switch (p->op.type) {
            case TokenType::PLUS: return "_op_add(" + l_str + ", " + r_str + ")";
            case TokenType::MINUS: return "_op_binary(" + l_str + ", " + r_str + ", '-')";
            case TokenType::STAR: return "_op_binary(" + l_str + ", " + r_str + ", '*')";
            case TokenType::SLASH: return "_op_binary(" + l_str + ", " + r_str + ", '/')";
            case TokenType::PERCENT: return "_op_binary(" + l_str + ", " + r_str + ", '%')";
            case TokenType::EQ: return "_op_eq(" + l_str + ", " + r_str + ")";
            case TokenType::NE: return "_op_neq(" + l_str + ", " + r_str + ")";
            case TokenType::LT: return "_op_binary(" + l_str + ", " + r_str + ", '<')";
            case TokenType::LE: return "_op_binary(" + l_str + ", " + r_str + ", 'L')";
            case TokenType::GT: return "_op_binary(" + l_str + ", " + r_str + ", '>')";
            case TokenType::GE: return "_op_binary(" + l_str + ", " + r_str + ", 'G')";
            case TokenType::AND: return "Value(" + l_str + ".toBool() && " + r_str + ".toBool())";
            case TokenType::OR: return "Value(" + l_str + ".toBool() || " + r_str + ".toBool())";
            default: return "/* unsupported binary op */";
        }
    }
    if (auto p = dynamic_cast<const UnaryExpr*>(&e)) {
        if (p->op.type == TokenType::MINUS) return "_op_unary_minus(" + compile(*p->expr, types, is_in_function_scope, is_method, class_name) + ")";
        if (p->op.type == TokenType::NOT) return "_op_not(" + compile(*p->expr, types, is_in_function_scope, is_method, class_name) + ")";
    }
    if (auto p = dynamic_cast<const VarExpr*>(&e)) {
        if (types.count(p->name)) {
            if (types.at(p->name).is_heap_allocated) {
                return "(*" + sanitize(p->name) + ")";
            }
            return sanitize(p->name);
        }
        return "(*_global_env)[\"" + p->name + "\"]";
    }
    if (auto p = dynamic_cast<const CallExpr*>(&e)) {
        std::stringstream ss;
        if (auto* var_callee = dynamic_cast<const VarExpr*>(p->callee.get())) {
            if (class_names.count(var_callee->name)) {
                ss << "Value(std::make_shared<_class_" << var_callee->name << ">(";
                for (size_t i = 0; i < p->args.size(); ++i) {
                    ss << compile(*p->args[i], types, is_in_function_scope, is_method, class_name) << (i < p->args.size() - 1 ? ", " : "");
                }
                ss << "))";
                return ss.str();
            }
        }
        if (auto* member_callee = dynamic_cast<const MemberAccessExpr*>(p->callee.get())) {
            std::string obj_code = compile(*member_callee->object, types, is_in_function_scope, is_method, class_name);
            std::string class_type = "Object";
            if (auto* var_obj = dynamic_cast<const VarExpr*>(member_callee->object.get())) {
                if (types.count(var_obj->name)) class_type = types.at(var_obj->name).type_name;
            } else if (dynamic_cast<const ThisExpr*>(member_callee->object.get())) {
                class_type = class_name;
            }
            if (class_names.count(class_type)) {
                ss << "std::dynamic_pointer_cast<_class_" << class_type << ">(" << obj_code << ".as<Value::ObjectType>())->" << member_callee->member.lexeme << "(";
                for (size_t i = 0; i < p->args.size(); ++i) {
                   ss << compile(*p->args[i], types, is_in_function_scope, is_method, class_name) << (i < p->args.size() - 1 ? ", " : "");
                }
                ss << ")";
                return ss.str();
            }
        }
        if (auto* super_callee = dynamic_cast<SuperExpr*>(p->callee.get())) {
            auto it = class_nodes.find(class_name);
            if (it != class_nodes.end() && it->second->superclass) {
                std::string super_name = (*it->second->superclass)->name;
                ss << "this->_class_" << super_name << "::" << super_callee->method.lexeme << "(";
                for (size_t i = 0; i < p->args.size(); ++i) {
                    ss << compile(*p->args[i], types, is_in_function_scope, is_method, class_name) << (i < p->args.size() - 1 ? ", " : "");
                }
                ss << ")";
                return ss.str();
            }
        }

        ss << "_call(" << compile(*p->callee, types, is_in_function_scope, is_method, class_name) << ", {";
        for (size_t i = 0; i < p->args.size(); ++i) {
            ss << compile(*p->args[i], types, is_in_function_scope, is_method, class_name) << (i < p->args.size() - 1 ? ", " : "");
        }
        ss << "})";
        return ss.str();
    }
    if (auto p = dynamic_cast<const AssignExpr*>(&e)) {
        std::string value_str = compile(*p->value, types, is_in_function_scope, is_method, class_name);
        if (auto* var_target = dynamic_cast<const VarExpr*>(p->target.get())) {
            if (types.count(var_target->name)) {
                if (types.at(var_target->name).is_heap_allocated) {
                    return "(*" + sanitize(var_target->name) + " = " + value_str + ")";
                }
                return "(" + sanitize(var_target->name) + " = " + value_str + ")";
            }
            return "((*_global_env)[\"" + var_target->name + "\"] = " + value_str + ")";
        }
        if (auto* index_target = dynamic_cast<const IndexExpr*>(p->target.get())) {
            std::string obj_str;
            if (auto* var_obj = dynamic_cast<const VarExpr*>(index_target->object.get())) {
                 if (types.count(var_obj->name)) {
                    if (types.at(var_obj->name).is_heap_allocated) obj_str = "(*" + sanitize(var_obj->name) + ")";
                    else obj_str = sanitize(var_obj->name);
                } else {
                    obj_str = "(*_global_env)[\"" + var_obj->name + "\"]";
                }
            } else {
                obj_str = compile(*index_target->object, types, is_in_function_scope, is_method, class_name);
            }
            std::string idx_str = compile(*index_target->index, types, is_in_function_scope, is_method, class_name);
            return "_op_index_set(" + obj_str + ", " + idx_str + ", " + value_str + ")";
        }
        if (auto* member_target = dynamic_cast<const MemberAccessExpr*>(p->target.get())) {
            if (dynamic_cast<const ThisExpr*>(member_target->object.get())) {
                if (!is_method) throw std::runtime_error("Cannot use 'this' outside of a method.");
                return "(this->_fields[\"" + member_target->member.lexeme + "\"] = " + value_str + ")";
            }
            std::string obj_code = compile(*member_target->object, types, is_in_function_scope, is_method, class_name);
            return "_set_member(" + obj_code + ", \"" + member_target->member.lexeme + "\", " + value_str + ")";
        }
    }
    if (auto p = dynamic_cast<const ArrayLiteralExpr*>(&e)) {
         std::stringstream ss;
        ss << "Value(std::make_shared<std::vector<Value>>(std::initializer_list<Value>{";
        for (size_t i = 0; i < p->elements.size(); ++i) {
            ss << compile(*p->elements[i], types, is_in_function_scope, is_method, class_name) << (i < p->elements.size() - 1 ? ", " : "");
        }
        ss << "}))";
        return ss.str();
    }
    if (auto p = dynamic_cast<const DictLiteralExpr*>(&e)) {
        std::stringstream ss;
        ss << "Value(std::make_shared<std::unordered_map<std::string, Value>>(std::unordered_map<std::string, Value>({";
        for (size_t i = 0; i < p->pairs.size(); ++i) {
            ss << "{\"" << p->pairs[i].first << "\", " << compile(*p->pairs[i].second, types, is_in_function_scope, is_method, class_name) << "}" << (i < p->pairs.size() - 1 ? ", " : "");
        }
        ss << "})))";
        return ss.str();
    }
    if (auto p = dynamic_cast<const IndexExpr*>(&e)) {
        return "_op_index_get(" + compile(*p->object, types, is_in_function_scope, is_method, class_name) + ", " + compile(*p->index, types, is_in_function_scope, is_method, class_name) + ")";
    }
    if (auto p = dynamic_cast<const MemberAccessExpr*>(&e)) {
        if (dynamic_cast<const ThisExpr*>(p->object.get())) {
            if (!is_method) throw std::runtime_error("Cannot use 'this' outside of a method.");
            return "([&]() -> Value { "
                   "    auto it = this->_fields.find(\"" + p->member.lexeme + "\"); "
                   "    if (it != this->_fields.end()) return it->second; "
                   "    throw std::runtime_error(\"Undefined property '" + p->member.lexeme + "'.\"); "
                   "})()";
        }
        std::string obj_code = compile(*p->object, types, is_in_function_scope, is_method, class_name);
        return "_get_member(" + obj_code + ", \"" + p->member.lexeme + "\")";
    }
    if (auto p = dynamic_cast<const ThisExpr*>(&e)) {
        if (!is_method) throw std::runtime_error("Cannot use 'this' outside of a method.");
        return "Value(this->shared_from_this())";
    }
    if (auto p = dynamic_cast<const FuncLiteralExpr*>(&e)) {
        return compile_function_body(p->params, *p->body, "<lambda>", types, true, is_method, class_name);
    }
    return "/* Unsupported Expr */";
}

std::string Transpiler::compile_function_body(const std::vector<ParamInfo>& params, const BlockStmt& body, const std::string& func_name, TypeMap& types, bool is_in_function_scope, bool is_method, const std::string& class_name) {
    std::stringstream ss;
    ss << "Value(std::make_shared<UserFunction>(" << params.size() << ", \"" << func_name << "\", "
       << "[=](const std::vector<Value>& args) -> Value {\n";
    
    TypeMap bodyTypes = types;
    for (size_t i = 0; i < params.size(); ++i) {
        bodyTypes[params[i].name] = {"Value", false};
        ss << "            Value " << sanitize(params[i].name) << " = args[" << i << "];\n";
    }
    
    ss << "        " << compile(body, bodyTypes, true, is_method, class_name) << "\n";
    ss << "\n            return _V_NULL;\n        }))";
    return ss.str();
}


// =================================================================================================
//
//                                      PART 3: THE DRIVER PROGRAM
//
// =================================================================================================
int main() {
    std::string_view source_code = 
R"CODE(
# Comprehensive Test for try-catch-throw

print("--- Test 1: Basic throw and catch ---");
try {
    print("Inside try block, about to throw...");
    throw "This is a test exception!";
    print("This line should not be printed.");
} catch (e) {
    print("Caught exception:", e);
    assert(e == "This is a test exception!", "Test 1 Failed");
}
print("Test 1 Passed.\n");


print("--- Test 2: No exception thrown ---");
var x = 10;
try {
    print("Inside try block, no throw.");
    x = 20;
} catch (e) {
    print("This catch block should not execute.");
    x = 30;
}
print("Value of x after try:", x);
assert(x == 20, "Test 2 Failed");
print("Test 2 Passed.\n");


print("--- Test 3: Nested try-catch and re-throw ---");
try {
    print("Outer try block started.");
    try {
        print("Inner try block started.");
        throw {"code": 404, "message": "Not Found"};
    } catch (inner_e) {
        print("Inner catch caught:", str(inner_e));
        assert(type(inner_e) == "dict", "Test 3.1 Failed: type mismatch");
        print("Re-throwing the exception...");
        throw inner_e; // Re-throw
    }
    print("This line in outer try should not be reached.");
} catch (outer_e) {
    print("Outer catch caught:", str(outer_e));
    assert(type(outer_e) == "dict", "Test 3.2 Failed: type mismatch");
}
print("Test 3 Passed.\n");


print("--- Test 4: Return from within a try block ---");
func test_return() {
    try {
        print("About to return from inside a try block.");
        return "Success";
    } catch (e) {
        print("This should not be caught.");
        return "Failure";
    }
    return "Should not reach here.";
}
var result = test_return();
print("Result from test_return():", result);
assert(result == "Success", "Test 4 Failed");
print("Test 4 Passed.\n");

print("--- All try-catch tests passed! ---");
)CODE";

    std::cout << "--- Stage 1: Parsing Source Code ---\n";
    std::cout << "Source:\n" << source_code << "\n\n";

    StmtList ast;
    try {
        Lexer lexer(source_code);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        ast = parser.parse();
        bool has_errors = false;
        for(const auto& stmt : ast) {
            if (!stmt) {
                has_errors = true;
                break;
            }
        }
        if (has_errors) {
            std::cerr << "Parsing failed. Null statements found in AST, likely due to parse errors noted above.\n";
            return 1;
        }
        std::cout << "Parsing successful. AST created.\n\n";
    } catch (const std::exception& e) {
        std::cerr << "Error during parsing: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "--- Stage 2: Transpiling AST to C++ ---\n";
    Transpiler transpiler;
    std::string generated_cpp;
    try {
        generated_cpp = transpiler.transpile(ast);
    } catch (const std::exception& e) {
        std::cerr << "Error during transpilation: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Transpiling successful. Generated C++ code is ready.\n\n";

    const char* output_filename = "output.cpp";
    std::cout << "--- Stage 3: Writing C++ code to " << output_filename << " ---\n";
    std::ofstream out_file(output_filename);
    if (!out_file) {
        std::cerr << "Failed to open " << output_filename << " for writing.\n";
        return 1;
    }
    out_file << generated_cpp;
    out_file.close();
    std::cout << "Successfully wrote to " << output_filename << ".\n\n";

#ifdef _WIN32
    const char* executable_name = "output.exe";
    const char* compile_command = "g++ -std=c++17 -O2 -Wno-unused-variable -Wno-uninitialized -Wno-sign-compare output.cpp -o output.exe";
#else
    const char* executable_name = "./output";
    const char* compile_command = "g++ -std=c++17 -O2 -Wno-unused-variable -Wno-uninitialized -Wno-sign-compare output.cpp -o output";
#endif

    std::cout << "--- Stage 4: Compiling " << output_filename << " ---\n";
    std::cout << "Executing: " << compile_command << std::endl;
    int compile_result = system(compile_command);
    if (compile_result != 0) {
        std::cerr << "C++ compilation failed! Check the compiler output above for errors.\n";
        return 1;
    }
    std::cout << "Compilation successful. Executable created.\n\n";

    std::cout << "--- Stage 5: Running the compiled program ---\n";
    std::cout << "Output of the final program is:\n";
    std::cout << "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n";
    system(executable_name);
    std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
    std::cout << "--- Transpiler finished ---\n";

    return 0;
}