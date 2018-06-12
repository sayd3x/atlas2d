#pragma once

#include "raw_pixel_area.hpp"
#include "image.hpp"

namespace atlas2d {

    struct raw_image_props: raw_area_props {
        bool wipe_data = false;
        int padding_between_sprites = 0;
    };
    
    struct raw_image_filling_props: image_filling_props {
        bool premultiple = false;
    };
    
    /// Represents a memory allocated raw image
    class raw_image: public details::raw_pixel_area_adapter<image, raw_image_props> {
        using base = details::raw_pixel_area_adapter<image, raw_image_props>;
        
    public:
        /// Area's properties
        struct init_props: raw_image_props {
            using props = init_props;
            
            props& set_dims(size arg) { dimensions = std::move(arg); return *this;}
            props& set_pixel_format(pixel_format arg) { format = std::move(arg); return *this;}
            props& set_raw_data(raw_data_ptr arg) {data = std::move(arg); return *this;}
            props& wipe_allocated_data(bool arg=true) {wipe_data = arg; return *this;}
            props& set_sprites_padding(int arg) {padding_between_sprites = arg; return *this;}
        };
        
        struct filling_props: raw_image_filling_props {
            using props = filling_props;
            
            props& set_offset(offset arg) {offset_pos = std::move(arg); return *this;}
            props& enable_premultiple(bool arg=true) {premultiple = arg; return *this;}
        };
        
        virtual bool fill_image(pixel_area const& pixels, image_filling_props const& filling_props) override;
        
    };
    
} // namespace atlas2d
