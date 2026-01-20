
#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include "parser.h"
#include "templates.h"

using namespace std;
using namespace routio::generator;

// Output languags enum
enum class OutputLanguage {
    CPP, Python
    // Future languages can be added here
};

std::string output_filename(const std::string& input_filename, OutputLanguage lang) {
    /* Derive output filename from input filename and language
    */

    std::string base;
    auto pos = input_filename.find_last_of("/\\");
    if (pos != std::string::npos) {
        base = input_filename.substr(pos + 1);
    } else {
        base = input_filename;
    }
    pos = base.find_last_of('.');
    if (pos != std::string::npos) {
        base = base.substr(0, pos);
    }

    switch (lang) {
        case OutputLanguage::CPP:
            return base + ".cpp";
        case OutputLanguage::Python:
            return base + ".py";
    }
    return "";
}

void generate_code(const Description& desc, const std::string& input_filename, OutputLanguage lang) {
    std::string out_filename = "";
    if (!input_filename.empty()) {
        out_filename = output_filename(input_filename, lang);
    }
    std::string generated_code;

    switch (lang) {
        case OutputLanguage::CPP:
            generated_code = generate_cpp(desc, out_filename);
            break;
        case OutputLanguage::Python:
            generated_code = generate_python(desc);
            break;
    }

    if (out_filename.empty()) {
        // Print to stdout
        std::cout << generated_code;
        return;
    } else {
        std::ofstream outfile(out_filename);
        if (!outfile) {
            std::cerr << "Failed to open output file: " << out_filename << "\n";
            return;
        }
        outfile << generated_code;
    }

}


int main(const int argc, const char** argv) {
    // Reads given files or stdin, parses them, and reports success or failure.

    if (!std::ios::sync_with_stdio(false)) {
        std::cerr << "Failed to disable sync_with_stdio\n";
        return 1;
    }

    std::vector<std::string> input_files;
    OutputLanguage out_lang = OutputLanguage::CPP; // Default to C++
    // Parse arguments into flags and input files
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--language" && i + 1 < argc) {
            std::string lang = argv[++i];
            if (lang == "cpp") {
                out_lang = OutputLanguage::CPP;
            } else if (lang == "python") {
                out_lang = OutputLanguage::Python;
            } else {
                std::cerr << "Unknown output language: " << lang << "\n";
                return 1;
            }
        } else {
            input_files.push_back(arg);
        }
    }

    if (input_files.size() > 1) {
        // Read from files
        for (const std::string& filename : input_files) {
            std::ifstream infile(filename);
            if (!infile) {
                std::cerr << "Failed to open input file: " << filename << "\n";
                return 1;
            }
            std::string input((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
            try {
                Description d = parse(input, filename);
                generate_code(d, filename, out_lang);
            } catch (const DescriptionError& e) {
                std::cerr << e.what() << "\n";
                return 1;
            }
        }
        return 0;
    } else {
        std::cin.tie(nullptr);

        std::string input((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());

        try {
            Description d = parse(input, "input");
            generate_code(d, "", out_lang);
        } catch (const DescriptionError& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }

    }

    return 0;
}
