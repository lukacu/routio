// parser.h
// Header for the message description language parser.
// Provides data structures (AST) and parsing API.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace routio::generator {

// --------------------------- Error type ---------------------------

class DescriptionError : public std::runtime_error {
public:
    DescriptionError(std::string file, int line, int column, std::string message);

    const std::string& file() const noexcept;
    int line() const noexcept;
    int column() const noexcept;
    const std::string& message() const noexcept;

private:
    std::string file_;
    int line_;
    int column_;
    std::string message_;
};

// --------------------------- AST ---------------------------

struct Span {
    std::size_t offset = 0; // byte offset
    int line = 1;           // 1-based
    int col = 1;            // 1-based
};

using Value = std::variant<double, std::string, bool>;

struct KeywordArg {
    std::string name;
    Value value;
};

struct Properties {
    std::vector<Value> args;          // positional
    std::vector<KeywordArg> kwargs;   // keyword
};

struct FieldArray {
    // In grammar: [ Optional(ArrayLength) ]
    // length is optional; if missing, treat as "unspecified".
    std::optional<std::size_t> length;
};

struct Field {
    std::string type;                      // FieldType
    std::optional<FieldArray> array;       // [len]
    std::string name;                      // FieldName
    std::optional<Properties> properties;  // ( ... )
    std::optional<Value> default_value;    // = Value
};

struct FieldList {
    std::vector<Field> fields;
};

struct EnumerateValue {
    std::string name;
};

struct Enumerate {
    std::string name;
    std::vector<EnumerateValue> values;
};

struct Include {
    std::string name; // message file (string literal)
    std::optional<Properties> properties;
};

struct Import {
    std::string name; // message file (string literal)
};

struct ExternalLanguage {
    std::string language;                 // LanguageName
    std::string container;                // StringLiteral
    std::vector<std::string> sources;     // after "from" (0..n)
    std::optional<std::string> deflt;     // after "default"
    std::optional<std::string> read;
    std::optional<std::string> write;
};

struct External {
    std::string name;                       // StructureName
    std::vector<ExternalLanguage> languages; // in ( ... )
};

struct Structure {
    std::string name;
    FieldList fields;
};

struct Message {
    std::string name;
    FieldList fields;
};

struct Namespace {
    std::string name; // Word(alphanums + ".")
};

using Decl = std::variant<Enumerate, Include, Import, External, Structure, Message>;

struct Description {
    std::optional<Namespace> ns;
    std::vector<Decl> decls;
};

// --------------------------- Public API ---------------------------

// Parse a message description language text and return the AST.
// Throws DescriptionError on parse errors.
Description parse(std::string_view text, std::string filename = "<input>");

} // namespace routio::generator
