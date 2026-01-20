// generator.h
// C++ code generator for the message description language.
// Generates C++ header files from parsed message descriptions.

#pragma once

#include "parser.h"
#include <string>
#include <map>
#include <vector>
#include <optional>
#include <memory>
#include <functional>

namespace routio::generator {

// Language-specific container type
struct ContainerMapping {
    std::map<std::string, std::string> containers;  // language -> container type
    std::map<std::string, std::string> defaults;    // language -> default value
    std::map<std::string, std::string> readers;     // language -> reader function
    std::map<std::string, std::string> writers;     // language -> writer function
};

// Type metadata for code generation
struct TypeMetadata {
    std::string name;
    std::string hash;                               // Type hash for message identification
    ContainerMapping containers;                     // Language-specific mappings
    std::vector<std::string> sources;               // Include files or imports
    bool is_builtin = false;                        // Is this a built-in type?
    bool is_external = false;                       // Is this an external type?
    
    // Convenience getters for single language
    std::string getContainer(const std::string& language = "cpp") const;
    std::string getDefault(const std::string& language = "cpp") const;
    std::string getReader(const std::string& language = "cpp") const;
    std::string getWriter(const std::string& language = "cpp") const;
};

// Registry of types, enums, structs, and messages
class TypeRegistry {
public:
    TypeRegistry();
    
    void registerType(const TypeMetadata& metadata);
    void registerEnum(const std::string& name, const std::map<std::string, int>& values);
    void registerStruct(const std::string& name, const std::map<std::string, Field>& fields);
    void registerMessage(const std::string& name, const std::map<std::string, Field>& fields);
    void registerExternal(const External& ext);
    void registerBuiltinTypes_();
    
    const TypeMetadata* getType(const std::string& name) const;
    const std::map<std::string, int>* getEnum(const std::string& name) const;
    const std::map<std::string, Field>* getStruct(const std::string& name) const;
    bool isMessage(const std::string& name) const;
    std::string getHash(const std::string& name) const;
    
    const std::map<std::string, TypeMetadata>& getTypes() const { return types_; }
    const std::map<std::string, std::map<std::string, int>>& getEnums() const { return enums_; }
    const std::map<std::string, std::map<std::string, Field>>& getStructs() const { return structs_; }
    const std::vector<std::string>& getMessages() const { return messages_; }
    
    std::vector<std::string> getSources(const std::string& language = "cpp") const;
    std::string computeHash(const std::string& content) const;
    
private:
    TypeMetadata createBuiltinType_(const std::string& name,
                                   const std::map<std::string, std::string>& containers,
                                   const std::map<std::string, std::string>& defaults = {});
    
    std::map<std::string, TypeMetadata> types_;
    std::map<std::string, std::map<std::string, int>> enums_;
    std::map<std::string, std::map<std::string, Field>> structs_;
    std::vector<std::string> messages_;
};

// C++ code generator
class CppGenerator {
public:
    explicit CppGenerator(const Description& desc);
    
    std::string generate(const std::string& basename);
    
private:
    void processDescription_();
    void buildRegistry_();
    
    std::string generateHeader_(const std::string& basename);
    std::string generateIncludes_();
    std::string generateTypeSpecializations_();
    std::string generateNamespaceOpen_();
    std::string generateNamespaceClose_();
    std::string generateEnums_();
    std::string generateForwardDeclarations_();
    std::string generateStructs_();
    std::string generateEnumSerializers_();
    std::string generateStructSerializers_();
    std::string generateMessageSpecializations_();
    
    std::string formatValue_(const Value& val) const;
    std::string getFieldType_(const Field& field) const;
    std::string getDefaultValue_(const Field& field) const;
    std::string getCppNamespace_() const;
    
    const Description& desc_;
    TypeRegistry registry_;
    std::string namespace_;
};

// Python code generator
class PythonGenerator {
public:
    explicit PythonGenerator(const Description& desc);
    
    std::string generate();
    
private:
    void buildRegistry_();
    
    std::string generateHeader_();
    std::string generateImports_();
    std::string generateEnumHelper_();
    std::string generateEnums_();
    std::string generateExternalTypes_();
    std::string generateStructs_();
    std::string generateMessages_();
    
    std::string formatValue_(const Value& val) const;
    std::string getPythonType_(const Field& field) const;
    std::string getDefaultValue_(const Field& field) const;
    bool isArray_(const Field& field) const;
    bool isFixedArray_(const Field& field) const;
    
    const Description& desc_;
    TypeRegistry registry_;
};

// Generate C++ code from a description
std::string generate_cpp(const Description& desc, const std::string& basename);

// Generate Python code from a description
std::string generate_python(const Description& desc);

} // namespace routio::generator
