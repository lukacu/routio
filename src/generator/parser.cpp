// parser.cpp
// Standalone modern C++ implementation of a parser for the message description language.
// Provides a parser for the message description language, producing an AST.

#include "parser.h"

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

using namespace std;

namespace routio::generator {

// --------------------------- Error type implementation ---------------------------

DescriptionError::DescriptionError(std::string file, int line, int column, std::string message)
    : std::runtime_error(file + " (line: " + std::to_string(line) + ", col: " + std::to_string(column) + "): " + message),
      file_(std::move(file)),
      line_(line),
      column_(column),
      message_(std::move(message)) {}

const std::string& DescriptionError::file() const noexcept { return file_; }
int DescriptionError::line() const noexcept { return line_; }
int DescriptionError::column() const noexcept { return column_; }
const std::string& DescriptionError::message() const noexcept { return message_; }

// --------------------------- Lexer ---------------------------

enum class TokKind {
    End,
    Ident,      // [A-Za-z0-9_]+ (also used for keywords)
    Number,     // numeric literal (+/-? digits ( .digits )? (e +/-? digits)?)
    String,     // "..."
    LBrack, RBrack,
    LBrace, RBrace,
    LParen, RParen,   // used as < > in pyparsing code (LANGLE/RANGLE) but actual chars are "()"
    Colon,
    Semicolon,
    Equals,
    Comma,
    Dot,
};

struct Token {
    TokKind kind = TokKind::End;
    std::string_view lexeme{};
    Span span{};
};

class Lexer {
public:
    explicit Lexer(std::string_view input) : input_(input) {}

    Token peek(std::size_t k = 0) {
        while (lookahead_.size() <= k) {
            lookahead_.push_back(nextImpl_());
        }
        return lookahead_[k];
    }

    Token next() {
        if (!lookahead_.empty()) {
            Token t = lookahead_.front();
            lookahead_.erase(lookahead_.begin());
            return t;
        }
        return nextImpl_();
    }

private:
    std::string_view input_;
    std::size_t i_ = 0;
    int line_ = 1;
    int col_ = 1;
    std::vector<Token> lookahead_;

    char ch_() const {
        if (i_ >= input_.size()) return '\0';
        return input_[i_];
    }

    char chNext_() const {
        if (i_ + 1 >= input_.size()) return '\0';
        return input_[i_ + 1];
    }

    void advance_() {
        if (i_ >= input_.size()) return;
        if (input_[i_] == '\n') {
            ++line_;
            col_ = 1;
        } else {
            ++col_;
        }
        ++i_;
    }

    void skipWsAndComments_() {
        for (;;) {
            // whitespace
            while (std::isspace(static_cast<unsigned char>(ch_()))) {
                advance_();
            }
            // pythonStyleComment: starts with '#', runs to end of line
            if (ch_() == '#') {
                while (ch_() != '\0' && ch_() != '\n') advance_();
                continue;
            }
            break;
        }
    }

    static bool isIdentChar_(char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    Token make_(TokKind k, std::size_t start, Span sp) const {
        Token t;
        t.kind = k;
        t.lexeme = input_.substr(start, i_ - start);
        t.span = sp;
        return t;
    }

    Token lexString_() {
        Span sp{ i_, line_, col_ };
        std::size_t start = i_;
        advance_(); // consume opening "
        bool escaped = false;
        while (true) {
            char c = ch_();
            if (c == '\0' || c == '\n') {
                // unterminated string
                // Return as String token and let parser raise a better error; but safer to throw here
                throw DescriptionError("<input>", sp.line, sp.col, "Unterminated string literal");
            }
            if (escaped) {
                escaped = false;
                advance_();
                continue;
            }
            if (c == '\\') {
                escaped = true;
                advance_();
                continue;
            }
            if (c == '"') {
                advance_(); // consume closing "
                break;
            }
            advance_();
        }
        return make_(TokKind::String, start, sp);
    }

    Token lexNumber_() {
        Span sp{ i_, line_, col_ };
        std::size_t start = i_;

        // optional sign
        if (ch_() == '+' || ch_() == '-') advance_();

        // digits
        if (!std::isdigit(static_cast<unsigned char>(ch_()))) {
            // Not a valid number start; fall back to ident/symbol handling upstream
            // but we only call lexNumber_ when number is expected.
        }
        while (std::isdigit(static_cast<unsigned char>(ch_()))) advance_();

        // optional fractional: '.' digits?
        if (ch_() == '.') {
            advance_();
            while (std::isdigit(static_cast<unsigned char>(ch_()))) advance_();
        }

        // optional exponent: e/E (+/-)? digits
        if (ch_() == 'e' || ch_() == 'E') {
            advance_();
            if (ch_() == '+' || ch_() == '-') advance_();
            if (!std::isdigit(static_cast<unsigned char>(ch_()))) {
                throw DescriptionError("<input>", sp.line, sp.col, "Malformed exponent in number literal");
            }
            while (std::isdigit(static_cast<unsigned char>(ch_()))) advance_();
        }

        return make_(TokKind::Number, start, sp);
    }

    Token lexIdent_() {
        Span sp{ i_, line_, col_ };
        std::size_t start = i_;
        while (isIdentChar_(ch_())) advance_();
        return make_(TokKind::Ident, start, sp);
    }

    Token nextImpl_() {
        skipWsAndComments_();

        Span sp{ i_, line_, col_ };
        char c = ch_();
        if (c == '\0') return Token{TokKind::End, {}, sp};

        // string
        if (c == '"') return lexString_();

        // number: starts with digit OR starts with +/- followed by digit
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            ((c == '+' || c == '-') && std::isdigit(static_cast<unsigned char>(chNext_())))) {
            return lexNumber_();
        }

        // identifier: alnum/_ (NOTE: pyparsing Word(alphanums + "_") allows starting digit,
        // but that's OK because digits are already lexed as numbers above.)
        if (isIdentChar_(c)) return lexIdent_();

        // symbols
        std::size_t start = i_;
        advance_();
        switch (c) {
            case '[': return make_(TokKind::LBrack, start, sp);
            case ']': return make_(TokKind::RBrack, start, sp);
            case '{': return make_(TokKind::LBrace, start, sp);
            case '}': return make_(TokKind::RBrace, start, sp);
            case '(': return make_(TokKind::LParen, start, sp);
            case ')': return make_(TokKind::RParen, start, sp);
            case ':': return make_(TokKind::Colon, start, sp);
            case ';': return make_(TokKind::Semicolon, start, sp);
            case '=': return make_(TokKind::Equals, start, sp);
            case ',': return make_(TokKind::Comma, start, sp);
            case '.': return make_(TokKind::Dot, start, sp);
            default:
                throw DescriptionError("<input>", sp.line, sp.col, std::string("Unexpected character: '") + c + "'");
        }
    }
};

// --------------------------- Parser ---------------------------

class Parser {
public:
    Parser(std::string_view input, std::string filename)
        : lex_(input), filename_(std::move(filename)) {}

    Description parseDescription() {
        Description out;

        // Optional Namespace
        if (isKeyword_("namespace")) {
            out.ns = parseNamespace_();
        }

        // ZeroOrMore decls until End
        while (lex_.peek().kind != TokKind::End) {
            out.decls.push_back(parseDecl_());
        }

        // must end
        expect_(TokKind::End, "Expected end of input");
        return out;
    }

private:
    Lexer lex_;
    std::string filename_;

    [[noreturn]] void errorAt_(const Token& t, const std::string& msg) const {
        throw DescriptionError(filename_, t.span.line, t.span.col, msg);
    }

    Token expect_(TokKind k, const std::string& msg) {
        Token t = lex_.next();
        if (t.kind != k) errorAt_(t, msg);
        return t;
    }

    bool match_(TokKind k) {
        if (lex_.peek().kind == k) {
            lex_.next();
            return true;
        }
        return false;
    }

    bool isKeyword_(std::string_view kw) {
        Token t = lex_.peek();
        return t.kind == TokKind::Ident && t.lexeme == kw;
    }

    Token expectKeyword_(std::string_view kw) {
        Token t = lex_.next();
        if (t.kind != TokKind::Ident || t.lexeme != kw) {
            errorAt_(t, "Expected keyword '" + std::string(kw) + "'");
        }
        return t;
    }

    static std::string unquote_(std::string_view s, const Token& t, const std::string& filename) {
        // s includes the quotes "..."
        if (s.size() < 2 || s.front() != '"' || s.back() != '"') {
            throw DescriptionError(filename, t.span.line, t.span.col, "Internal error: invalid string token");
        }
        std::string out;
        out.reserve(s.size() - 2);

        for (std::size_t i = 1; i + 1 < s.size(); ++i) {
            char c = s[i];
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (i + 1 >= s.size() - 1) {
                throw DescriptionError(filename, t.span.line, t.span.col, "Invalid escape sequence in string");
            }
            char e = s[++i];
            switch (e) {
                case '\\': out.push_back('\\'); break;
                case '"':  out.push_back('"');  break;
                case 'n':  out.push_back('\n'); break;
                case 'r':  out.push_back('\r'); break;
                case 't':  out.push_back('\t'); break;
                default:
                    // Keep unknown escapes as-is (or error if you want stricter behavior)
                    out.push_back(e);
                    break;
            }
        }
        return out;
    }

    static double parseDouble_(std::string_view sv, const Token& t, const std::string& filename) {
        // std::from_chars for floating is C++17 but not consistently implemented across libstdc++ versions.
        // Use strtod for robustness.
        std::string tmp(sv);
        char* end = nullptr;
        double v = std::strtod(tmp.c_str(), &end);
        if (end == tmp.c_str() || *end != '\0' || !std::isfinite(v)) {
            throw DescriptionError(filename, t.span.line, t.span.col, "Invalid numeric literal");
        }
        return v;
    }

    // ---- Grammar pieces ----

    Value parseValue_() {
        Token t = lex_.peek();

        if (t.kind == TokKind::Number) {
            t = lex_.next();
            return parseDouble_(t.lexeme, t, filename_);
        }

        if (t.kind == TokKind::String) {
            t = lex_.next();
            return unquote_(t.lexeme, t, filename_);
        }

        if (t.kind == TokKind::Ident && (t.lexeme == "true" || t.lexeme == "false")) {
            lex_.next();
            return (t.lexeme == "true");
        }

        errorAt_(t, "Expected value (number, string, or boolean)");
        std::abort();
    }

    // PropertyList:
    // '('  (  (KeywordProperty (':' KeywordProperty)*)  |  (PositionalValue (':' PositionalValue)* ) (':' KeywordProperty)* )  ')'
    Properties parseProperties_() {
        Properties props;
        expect_(TokKind::LParen, "Expected '(' to start property list");

        // Decide whether first item is keyword property: Ident '=' ...
        // Or positional: Value
        if (lex_.peek().kind == TokKind::Ident && lex_.peek(1).kind == TokKind::Equals) {
            // keyword-only list
            parseKeywordPropertyInto_(props);
            while (match_(TokKind::Colon)) {
                if (!(lex_.peek().kind == TokKind::Ident && lex_.peek(1).kind == TokKind::Equals)) {
                    errorAt_(lex_.peek(), "Expected keyword property name=value after ':'");
                }
                parseKeywordPropertyInto_(props);
            }
        } else if (lex_.peek().kind != TokKind::RParen) {
            // positional first
            props.args.push_back(parseValue_());
            while (match_(TokKind::Colon)) {
                // after ':' could be more positional OR start of kwargs
                if (lex_.peek().kind == TokKind::Ident && lex_.peek(1).kind == TokKind::Equals) {
                    // switch to kwargs; parse all remaining kwargs separated by ':'
                    parseKeywordPropertyInto_(props);
                    while (match_(TokKind::Colon)) {
                        if (!(lex_.peek().kind == TokKind::Ident && lex_.peek(1).kind == TokKind::Equals)) {
                            errorAt_(lex_.peek(), "Expected keyword property name=value after ':'");
                        }
                        parseKeywordPropertyInto_(props);
                    }
                    break;
                } else {
                    props.args.push_back(parseValue_());
                }
            }
        }

        expect_(TokKind::RParen, "Expected ')' to end property list");
        return props;
    }

    void parseKeywordPropertyInto_(Properties& props) {
        Token nameTok = expect_(TokKind::Ident, "Expected property name");
        expect_(TokKind::Equals, "Expected '=' in keyword property");
        Value v = parseValue_();
        props.kwargs.push_back(KeywordArg{std::string(nameTok.lexeme), std::move(v)});
    }

    std::optional<FieldArray> parseOptionalArray_() {
        if (!match_(TokKind::LBrack)) return std::nullopt;

        FieldArray arr;
        // Optional(ArrayLength) = digits; in our lexer digits are Number, but could be float if written weirdly.
        if (lex_.peek().kind == TokKind::Number) {
            Token t = lex_.next();
            // only accept integer length
            std::string_view sv = t.lexeme;
            // allow leading sign? grammar doesn't, but we can reject if sign present
            if (!sv.empty() && (sv.front() == '+' || sv.front() == '-')) {
                errorAt_(t, "Array length must be a non-negative integer");
            }
            std::size_t len = 0;
            auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), len);
            if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
                errorAt_(t, "Array length must be an integer");
            }
            arr.length = len;
        }

        expect_(TokKind::RBrack, "Expected ']' after array specifier");
        return arr;
    }

    Field parseField_() {
        Field f;

        // FieldType: Word(alphanums)
        Token typeTok = expect_(TokKind::Ident, "Expected field type");
        f.type = std::string(typeTok.lexeme);

        // Optional array: '[' Optional(length) ']'
        f.array = parseOptionalArray_();

        // FieldName: Word(alphanums + "_")
        Token nameTok = expect_(TokKind::Ident, "Expected field name");
        f.name = std::string(nameTok.lexeme);

        // Optional PropertyList
        if (lex_.peek().kind == TokKind::LParen) {
            f.properties = parseProperties_();
        }

        // Optional default: '=' Value
        if (match_(TokKind::Equals)) {
            f.default_value = parseValue_();
        }

        expect_(TokKind::Semicolon, "Expected ';' after field");
        return f;
    }

    FieldList parseFieldList_() {
        FieldList fl;
        expect_(TokKind::LBrace, "Expected '{' to start field list");
        while (lex_.peek().kind != TokKind::RBrace) {
            if (lex_.peek().kind == TokKind::End) {
                errorAt_(lex_.peek(), "Unterminated field list; expected '}'");
            }
            fl.fields.push_back(parseField_());
        }
        expect_(TokKind::RBrace, "Expected '}' to end field list");
        return fl;
    }

    Enumerate parseEnumerate_() {
        expectKeyword_("enumerate");
        Token nameTok = expect_(TokKind::Ident, "Expected enumerate name");
        Enumerate e;
        e.name = std::string(nameTok.lexeme);

        expect_(TokKind::LBrace, "Expected '{' after enumerate name");

        // delimitedList(EnumerateValue) where EnumerateValue = Word(alphanums + "_") as "name"
        if (lex_.peek().kind != TokKind::RBrace) {
            e.values.push_back(EnumerateValue{std::string(expect_(TokKind::Ident, "Expected enumerate value").lexeme)});
            while (match_(TokKind::Comma)) {
                e.values.push_back(EnumerateValue{std::string(expect_(TokKind::Ident, "Expected enumerate value").lexeme)});
            }
        }

        expect_(TokKind::RBrace, "Expected '}' to end enumerate");
        return e;
    }

    Include parseInclude_() {
        expectKeyword_("include");
        Token fileTok = expect_(TokKind::String, "Expected quoted filename after 'include'");
        Include inc;
        inc.name = unquote_(fileTok.lexeme, fileTok, filename_);

        if (lex_.peek().kind == TokKind::LParen) {
            inc.properties = parseProperties_();
        }

        expect_(TokKind::Semicolon, "Expected ';' after include");
        return inc;
    }

    Import parseImport_() {
        expectKeyword_("import");
        Token fileTok = expect_(TokKind::String, "Expected quoted filename after 'import'");
        Import imp;
        imp.name = unquote_(fileTok.lexeme, fileTok, filename_);
        expect_(TokKind::Semicolon, "Expected ';' after import");
        return imp;
    }

    ExternalLanguage parseExternalLanguage_() {
        // ExternalLanguage:
        // 'language' LanguageName StringLiteral
        // [ 'from' OneOrMore(StringLiteral) ]
        // [ 'default' StringLiteral ]
        // [ 'read' StringLiteral 'write' StringLiteral ]
        // ';'
        expectKeyword_("language");

        Token langTok = expect_(TokKind::Ident, "Expected language name after 'language'");
        Token contTok = expect_(TokKind::String, "Expected container string after language name");

        ExternalLanguage el;
        el.language = std::string(langTok.lexeme);
        el.container = unquote_(contTok.lexeme, contTok, filename_);

        if (isKeyword_("from")) {
            lex_.next();
            // OneOrMore strings
            if (lex_.peek().kind != TokKind::String) {
                errorAt_(lex_.peek(), "Expected at least one source string after 'from'");
            }
            while (lex_.peek().kind == TokKind::String) {
                Token s = lex_.next();
                el.sources.push_back(unquote_(s.lexeme, s, filename_));
            }
        }

        if (isKeyword_("default")) {
            lex_.next();
            Token d = expect_(TokKind::String, "Expected default string after 'default'");
            el.deflt = unquote_(d.lexeme, d, filename_);
        }

        if (isKeyword_("read")) {
            lex_.next();
            Token r = expect_(TokKind::String, "Expected read string after 'read'");
            el.read = unquote_(r.lexeme, r, filename_);

            expectKeyword_("write");
            Token w = expect_(TokKind::String, "Expected write string after 'write'");
            el.write = unquote_(w.lexeme, w, filename_);
        }

        expect_(TokKind::Semicolon, "Expected ';' after language entry");
        return el;
    }

    std::vector<ExternalLanguage> parseExternalLanguageList_() {
        // '(' ZeroOrMore(ExternalLanguage) ')'
        expect_(TokKind::LParen, "Expected '(' to start external language list");
        std::vector<ExternalLanguage> langs;

        while (lex_.peek().kind != TokKind::RParen) {
            if (lex_.peek().kind == TokKind::End) {
                errorAt_(lex_.peek(), "Unterminated external language list; expected ')'");
            }
            if (!isKeyword_("language")) {
                errorAt_(lex_.peek(), "Expected 'language' entry inside external language list");
            }
            langs.push_back(parseExternalLanguage_());
        }
        expect_(TokKind::RParen, "Expected ')' to end external language list");
        return langs;
    }

    External parseExternal_() {
        expectKeyword_("external");
        Token nameTok = expect_(TokKind::Ident, "Expected external structure name");
        External ex;
        ex.name = std::string(nameTok.lexeme);
        ex.languages = parseExternalLanguageList_();
        expect_(TokKind::Semicolon, "Expected ';' after external");
        return ex;
    }

    Structure parseStructure_() {
        expectKeyword_("structure");
        Token nameTok = expect_(TokKind::Ident, "Expected structure name");
        Structure s;
        s.name = std::string(nameTok.lexeme);
        s.fields = parseFieldList_();
        return s;
    }

    Message parseMessage_() {
        expectKeyword_("message");
        Token nameTok = expect_(TokKind::Ident, "Expected message name");
        Message m;
        m.name = std::string(nameTok.lexeme);
        m.fields = parseFieldList_();
        return m;
    }

    Namespace parseNamespace_() {
        expectKeyword_("namespace");

        // Word(alphanums + ".") in the original grammar.
        // We'll accept a sequence like Ident ('.' Ident)*, but note the original disallows '_' in namespace.
        // If you need strict behavior, validate characters here.
        Token first = expect_(TokKind::Ident, "Expected namespace name");
        std::string name(first.lexeme);

        while (match_(TokKind::Dot)) {
            Token part = expect_(TokKind::Ident, "Expected namespace segment after '.'");
            name.push_back('.');
            name.append(part.lexeme);
        }

        expect_(TokKind::Semicolon, "Expected ';' after namespace");
        return Namespace{std::move(name)};
    }

    Decl parseDecl_() {
        Token t = lex_.peek();
        if (t.kind != TokKind::Ident) {
            errorAt_(t, "Expected a declaration keyword");
        }

        if (t.lexeme == "enumerate") return parseEnumerate_();
        if (t.lexeme == "include")   return parseInclude_();
        if (t.lexeme == "import")    return parseImport_();
        if (t.lexeme == "external")  return parseExternal_();
        if (t.lexeme == "structure") return parseStructure_();
        if (t.lexeme == "message")   return parseMessage_();

        errorAt_(t, "Unknown declaration keyword: " + std::string(t.lexeme));
        std::abort();
    }
};

// --------------------------- Public API implementation ---------------------------

Description parse(std::string_view text, std::string filename) {
    Parser p(text, std::move(filename));
    return p.parseDescription();
}

} // namespace routio::generator
// End of parser.cpp