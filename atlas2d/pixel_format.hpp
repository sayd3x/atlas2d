#pragma once

#include <memory>
#include <string>

namespace atlas2d {

    /// Supported pixel formats
    enum class pixel_format {
        unknown,
        rgb8,
        rgb565,
        rgba8,
        rgba4,
    };
    
    /// Format details
    struct format_details {
        format_details(): format(pixel_format::unknown), bpp(0) { ;; }
        
        std::string formatName; ///< String representation of the format
        pixel_format format;    ///< The format itself
        int bpp;                ///< Bytes per pixel
    };
    
    /// Returns a format details by the format itself
    format_details const& pixel_format_details(pixel_format f);
    
    /// Returns a format details by format's string representation
    format_details const& pixel_format_details(std::string const& name);
    
} // namespace atlas2d
