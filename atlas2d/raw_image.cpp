#include "raw_image.hpp"
#include "pixel_format.hpp"
#include "pixel_converter.hpp"

#include <cstring>

using namespace ::atlas2d;

namespace {
    /// Allocates an pixels buffer
    raw_data_ptr allocate_data(raw_image_props const& props) {
        auto bpp = pixel_format_details(props.format).bpp;
        
        size_t dataSize = (size_t)(props.dimensions.width * props.dimensions.height * bpp);
        if(!dataSize) {
            return nullptr;
        }
        
        auto data = raw_data_ptr((unsigned char*)malloc(dataSize), [](unsigned char* ptr){free(ptr);});
        if(props.wipe_data)
            memset(data.get(), 0, dataSize);
        
        return data;
    }
    
    /// Calculates pixel's index of the area by its (x, y) coordinates.
    size_t pixel_index_of(pixel_area const& area, int x, int y) {
        // TODO: here is a bottleneck. Optimize it in future.
        auto dims = area.get_dimensions();
        auto bpp = pixel_format_details(area.get_pixel_format()).bpp;
        
        return (y * dims.width + x) * bpp;
    }
}

bool raw_image::fill_image(pixel_area const& pixels, image_filling_props const& base_props) {
    auto const& filling_props = dynamic_cast<raw_image_filling_props const&>(base_props);
    auto const& src_area = dynamic_cast<raw_pixel_area const&>(pixels);
    
    if(!_props.data)
        _props.data = allocate_data(_props);
    
    auto dst_size = get_dimensions();
    auto src_size = src_area.get_dimensions();
    
    unsigned char* dst_pixels = get_raw_pixels();
    unsigned char* src_pixels = src_area.get_raw_pixels();
    
    auto const& at_pos = filling_props.offset_pos;
    bool does_area_fit = (at_pos.x + src_size.width <= dst_size.width &&
                        at_pos.y + src_size.height <= dst_size.height);
    
    if(!src_pixels || !dst_pixels || !does_area_fit)
        return false;
    
    int padding_between_sprites = _props.padding_between_sprites;
    
    // width of the mirror on the left side of the image
    int left_margin = (std::min)(padding_between_sprites, src_size.width);
    left_margin = (std::min)(left_margin, at_pos.x);
    
    // width of the mirror on the right side of the image
    int right_margin = (std::min)(padding_between_sprites, src_size.width);
    right_margin = (std::min)(right_margin, dst_size.width - at_pos.x - src_size.width);
    
    // height of the mirror on the top side of the image
    int top_margin = (std::min)(padding_between_sprites, src_size.height);
    top_margin = (std::min)(top_margin, at_pos.y);
    
    // height of the bottom side of the mirror
    int bottom_margin = (std::min)(padding_between_sprites, src_size.height);
    bottom_margin = (std::min)(bottom_margin, dst_size.height - at_pos.y - src_size.height);
    
    auto converter = create_pixel_converter(set_converter_params()
                                            .set_src_fmt(src_area.get_pixel_format())
                                            .set_dst_fmt(get_pixel_format())
                                            .set_pixels_count(src_size.width)
                                            .set_margins(left_margin, right_margin)
                                            .enable_premultiple(filling_props.premultiple));

    
    size_t bpp = pixel_format_details(converter->props().dst_format).bpp;
    size_t pixels_in_block = src_size.width + left_margin + right_margin;
    
    size_t src_bpp = pixel_format_details(converter->props().src_format).bpp;
    raw_data_ptr src_row((unsigned char*)malloc(src_size.width * src_bpp),
                            [](unsigned char* ptr){free(ptr);});
    
    for(int y = 0; y < src_size.height; ++y) {
        size_t dst_index = pixel_index_of(*this,
                                          at_pos.x - left_margin,
                                          y + at_pos.y);
        
        src_area.read_row(src_row.get(), y);
        unsigned char* src_block = src_row.get();
        unsigned char* dst_block = &dst_pixels[dst_index];
        
        (*converter)(src_block, dst_block, src_size.width);
        
        // also mirror top and bottom rows
        src_block = dst_block;
        
        // the top rows
        if(top_margin > 0 && (y+1) <= top_margin) {
            size_t dst_index = pixel_index_of(*this,
                                              at_pos.x - left_margin,
                                              at_pos.y - y - 1);
            unsigned char* dst_block = &dst_pixels[dst_index];
            std::memcpy(dst_block, src_block, pixels_in_block * bpp);
        }
        
        // the bottom rows
        if(bottom_margin > 0 && (src_size.height - y - 1) < bottom_margin) {
            size_t dst_index = pixel_index_of(*this,
                                              at_pos.x - left_margin,
                                              at_pos.y + src_size.height + (src_size.height - y - 1));
            unsigned char* dst_block = &dst_pixels[dst_index];
            std::memcpy(dst_block, src_block, pixels_in_block * bpp);
        }
    }
    
    return true;
}


