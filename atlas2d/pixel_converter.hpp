#pragma once

#include "pixel_format.hpp"
#include <functional>
#include <array>

namespace atlas2d {
    
    class pixel_converter;
    using pixel_converter_ptr = std::shared_ptr<pixel_converter>;

    /// A set of parameters for creating a pixel converter
    struct converter_params {
        pixel_format src_fmt;           ///< Origin pixel format
        pixel_format dst_fmt;           ///< Destination pixel format
        size_t pixels_count;            ///< Pixels count
        std::array<size_t,2> margins;   ///< Destination buffer's extra pixels (left and right side)
        bool premultiple = false;       ///< Premultiple each pixel with it's alpha channel
    };
    
    // Helper
    struct set_converter_params: converter_params {
        using self = set_converter_params;
        self& set_src_fmt(pixel_format const& arg) {src_fmt=arg; return *this;}
        self& set_dst_fmt(pixel_format const& arg) {dst_fmt=arg; return *this;}
        self& set_pixels_count(size_t const& arg) {pixels_count=arg; return *this;}
        self& set_margins(size_t left, size_t right=0) {margins[0]=left; margins[1]=right; return *this;}
        self& enable_premultiple(bool arg=true) {premultiple=arg; return *this;}
    };
    
    /// Creates a converter to convert pixels of <src_fmt> format to <dst_fmt> pixel format
    pixel_converter_ptr create_pixel_converter(converter_params const& params);

    
    /// Generic pixel converter
    class pixel_converter {
    protected:
        struct basic_props {
            std::function<void(unsigned char*, unsigned char*, size_t)> cb;
            pixel_format src_format;
            pixel_format dst_format;
        };
        
    public:
        using callback = std::function<void(unsigned char*, unsigned char*, size_t)>;
        
        struct properties: basic_props {
            properties& set_callback(callback arg) {cb = std::move(arg); return *this;}
            properties& set_src_format(pixel_format fmt) {src_format = fmt; return *this;}
            properties& set_dst_format(pixel_format fmt) {dst_format = fmt; return *this;}
        };
        
        virtual ~pixel_converter() { ;; }
        pixel_converter() { ;; }
        explicit pixel_converter(basic_props p) {
            reset(std::move(p));
        }
        
        /// Set up the converter
        void reset(basic_props p) {
            _props = std::move(p);
        }
        
        /// Convert <pixels_count> pixels from the <src_buffer> to the <dst_buffer>
        void operator()(unsigned char* src_pixels, unsigned char* dst_pixels, size_t pixels_count) const {
            _props.cb(src_pixels, dst_pixels, pixels_count);
        }
        
        /// Return converter's properties
        basic_props const& props() const {
            return _props;
        }
        
    private:
        basic_props _props;
        
    };
    
} // namespace atlas2d
