#pragma once

#include <memory>

namespace atlas2d {
    
    namespace details {
        
        // 2d vector of int
        struct vec2i {
            vec2i() { ;; }
            explicit vec2i(int c0, int c1) { v[0]=c0; v[1]=c1; }
            
            union {
                int v[2];
                struct { int x, y; };
                struct { int width, height; };
            };
        };
        
        inline std::shared_ptr<unsigned char> unowned_ptr(unsigned char* ptr) {
            return std::shared_ptr<unsigned char>(ptr, [](unsigned char*){});
        }
        
    } // details
    
    
    using raw_data_ptr = std::shared_ptr<unsigned char>;
    using offset = details::vec2i;
    using size = details::vec2i;

    enum class pixel_format;
    struct format_info;
    class pixels_array;
    class image;

} // namespace atlas2d
