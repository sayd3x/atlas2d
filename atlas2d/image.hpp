#pragma once

#include "forwards.hpp"

namespace atlas2d {

    /// The pure interface of an area of pixels.
    class pixel_area {
    public:
        virtual ~pixel_area() { ;; }
        
        /// Returns area's dimensions
        virtual size get_dimensions() const = 0;
        
        /// Returns area's pixel format
        virtual pixel_format get_pixel_format() const = 0;

    };
    
    /// Describes extra properties of the filling process
    struct image_filling_props {
        virtual ~image_filling_props() { ;; }
        
        offset offset_pos;
    };
    
    /// The pure interface of an image
    class image: public pixel_area {
    public:
        virtual ~image() { ;; }
        
        virtual bool fill_image(pixel_area const& pixels, image_filling_props const& props) = 0;
        
    };
    
} // namespace atlas2d
