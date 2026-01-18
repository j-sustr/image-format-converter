/**
 * HEIC to WebP Converter
 * Uses libheif for HEIC decoding and libwebp for WebP encoding
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <libheif/heif.h>
#include <webp/encode.h>

namespace fs = std::filesystem;

struct Options {
    std::string input;
    std::string output_dir;
    int quality = 85;
    bool recursive = false;
    bool verbose = false;
};

void print_usage(const char* program_name) {
    std::cout << R"(
🖼️  HEIC to WebP Converter

Usage:
  )" << program_name << R"( <input> [options]

Arguments:
  <input>              HEIC file or directory containing HEIC files

Options:
  -o, --output <dir>   Output directory (default: same as input)
  -q, --quality <n>    WebP quality 1-100 (default: 85)
  -r, --recursive      Process directories recursively
  -v, --verbose        Show detailed progress
  -h, --help           Show this help message

Examples:
  )" << program_name << R"( photo.heic
  )" << program_name << R"( photos/ -r -v
  )" << program_name << R"( photos/ -o converted/ -q 90
)";
}

std::string format_bytes(size_t bytes) {
    char buf[64];
    if (bytes < 1024) {
        snprintf(buf, sizeof(buf), "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
    } else {
        snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
    }
    return buf;
}

bool is_heic_file(const fs::path& path) {
    if (!fs::is_regular_file(path)) return false;
    std::string ext = path.extension().string();
    // Convert to lowercase
    for (char& c : ext) c = std::tolower(c);
    return ext == ".heic" || ext == ".heif";
}

std::vector<fs::path> find_heic_files(const fs::path& dir, bool recursive) {
    std::vector<fs::path> files;
    
    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (is_heic_file(entry.path())) {
                files.push_back(entry.path());
            }
        }
    } else {
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (is_heic_file(entry.path())) {
                files.push_back(entry.path());
            }
        }
    }
    
    return files;
}

fs::path get_output_path(const fs::path& input, const std::string& output_dir) {
    fs::path dir = output_dir.empty() ? input.parent_path() : fs::path(output_dir);
    return dir / (input.stem().string() + ".webp");
}

bool convert_heic_to_webp(const fs::path& input_path, const fs::path& output_path, 
                          const Options& opts) {
    if (opts.verbose) {
        std::cout << "📸 Decoding: " << input_path << std::endl;
    }

    // Open HEIC file
    heif_context* ctx = heif_context_alloc();
    heif_error err = heif_context_read_from_file(ctx, input_path.c_str(), nullptr);
    
    if (err.code != heif_error_Ok) {
        std::cerr << "❌ Failed to read HEIC: " << err.message << std::endl;
        heif_context_free(ctx);
        return false;
    }

    // Get primary image handle
    heif_image_handle* handle;
    err = heif_context_get_primary_image_handle(ctx, &handle);
    
    if (err.code != heif_error_Ok) {
        std::cerr << "❌ Failed to get image handle: " << err.message << std::endl;
        heif_context_free(ctx);
        return false;
    }

    // Decode to RGB
    heif_image* img;
    err = heif_decode_image(handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGB, nullptr);
    
    if (err.code != heif_error_Ok) {
        std::cerr << "❌ Failed to decode image: " << err.message << std::endl;
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return false;
    }

    int width = heif_image_get_width(img, heif_channel_interleaved);
    int height = heif_image_get_height(img, heif_channel_interleaved);
    
    if (opts.verbose) {
        std::cout << "   Dimensions: " << width << "x" << height << std::endl;
        std::cout << "💾 Encoding WebP: " << output_path << std::endl;
    }

    int stride;
    const uint8_t* rgb_data = heif_image_get_plane_readonly(img, heif_channel_interleaved, &stride);

    // Encode to WebP
    uint8_t* webp_data = nullptr;
    size_t webp_size = WebPEncodeRGB(rgb_data, width, height, stride, 
                                      static_cast<float>(opts.quality), &webp_data);

    if (webp_size == 0) {
        std::cerr << "❌ Failed to encode WebP" << std::endl;
        heif_image_release(img);
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return false;
    }

    // Write output file
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "❌ Failed to create output file: " << output_path << std::endl;
        WebPFree(webp_data);
        heif_image_release(img);
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return false;
    }
    
    out.write(reinterpret_cast<char*>(webp_data), webp_size);
    out.close();

    if (opts.verbose) {
        size_t input_size = fs::file_size(input_path);
        double ratio = (1.0 - static_cast<double>(webp_size) / input_size) * 100.0;
        std::cout << "   Size: " << format_bytes(input_size) << " → " 
                  << format_bytes(webp_size) << " (" << std::fixed 
                  << std::setprecision(1) << ratio << "% smaller)" << std::endl;
    }

    // Cleanup
    WebPFree(webp_data);
    heif_image_release(img);
    heif_image_handle_release(handle);
    heif_context_free(ctx);

    return true;
}

Options parse_args(int argc, char* argv[]) {
    Options opts;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            exit(0);
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                opts.output_dir = argv[++i];
            } else {
                std::cerr << "❌ Missing argument for " << arg << std::endl;
                exit(1);
            }
        } else if (arg == "-q" || arg == "--quality") {
            if (i + 1 < argc) {
                opts.quality = std::stoi(argv[++i]);
                if (opts.quality < 1 || opts.quality > 100) {
                    std::cerr << "❌ Quality must be between 1 and 100" << std::endl;
                    exit(1);
                }
            } else {
                std::cerr << "❌ Missing argument for " << arg << std::endl;
                exit(1);
            }
        } else if (arg == "-r" || arg == "--recursive") {
            opts.recursive = true;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg[0] == '-') {
            std::cerr << "❌ Unknown option: " << arg << std::endl;
            exit(1);
        } else {
            opts.input = arg;
        }
    }
    
    return opts;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    Options opts = parse_args(argc, argv);
    
    if (opts.input.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    fs::path input_path = fs::absolute(opts.input);
    
    if (!fs::exists(input_path)) {
        std::cerr << "❌ Input not found: " << input_path << std::endl;
        return 1;
    }

    // Create output directory if specified
    if (!opts.output_dir.empty()) {
        fs::create_directories(opts.output_dir);
    }

    std::vector<fs::path> files;
    
    if (fs::is_directory(input_path)) {
        files = find_heic_files(input_path, opts.recursive);
        if (files.empty()) {
            std::cout << "📭 No HEIC files found" << std::endl;
            return 0;
        }
        std::cout << "📂 Found " << files.size() << " HEIC file(s)" << std::endl;
    } else {
        files.push_back(input_path);
    }

    int success_count = 0;
    int error_count = 0;

    for (const auto& file : files) {
        fs::path output_path = get_output_path(file, opts.output_dir);
        
        if (convert_heic_to_webp(file, output_path, opts)) {
            success_count++;
            if (opts.verbose) {
                std::cout << "✅ Done\n" << std::endl;
            } else {
                std::cout << "✅ " << file.filename().string() << " → " 
                          << output_path.filename().string() << std::endl;
            }
        } else {
            error_count++;
            std::cerr << "❌ Failed: " << file.filename().string() << std::endl;
        }
    }

    std::cout << "\n📊 Converted: " << success_count << "/" << files.size() << " files" << std::endl;
    
    return error_count > 0 ? 1 : 0;
}
