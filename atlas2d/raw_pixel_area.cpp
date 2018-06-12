#include "raw_pixel_area.hpp"
#include "pixel_format.hpp"

#include <functional>
#include <map>
#include <cassert>
#include <cstring>

using namespace ::std;
using namespace ::atlas2d;
using namespace ::atlas2d::details;

namespace {
    
    struct row_selector;
    
    /// The generic row fetcher
    using row_fetcher = std::function<void(unsigned char*, row_selector const&)>;

    
    /// Selector for a raw pixels row
    struct row_selector {
        int row_index = 0;
        size dims;
        int bpp = 0;
        unsigned char* raw_pixels = nullptr;
        
        row_selector& set_raw_pixels(unsigned char* arg) { raw_pixels = arg; return *this; }
        row_selector& set_bpp(int arg) { bpp = arg; return *this; }
        row_selector& set_row_index(int arg) { row_index = arg; return *this; }
        row_selector& set_dims(size arg) { dims = std::move(arg); return *this; }

    };
    
    void read_row_rotated0(unsigned char* dst, row_selector const& src) {
        size_t index = src.dims.width * src.bpp * src.row_index;
        memcpy(dst, &src.raw_pixels[index], src.dims.width * src.bpp);
    }

    void read_row_rotated90(unsigned char* dst, row_selector const& src) {
        // read right edge of the image as the row
        for(int i = 0; i < src.dims.height; ++i) {
            size_t src_index = ((i + 1) * src.dims.width - 1 - src.row_index) * src.bpp;
            memcpy(&dst[i * src.bpp], &src.raw_pixels[src_index], src.bpp);
        }
    }
    
    void read_row_rotated180(unsigned char* dst, row_selector const& src) {
        // read starts from the bottom edge of the image in reverse order
        for(int i = 0; i < src.dims.width; ++i) {
            size_t src_index = ((src.dims.height - src.row_index) * src.dims.width - 1 - i) * src.bpp;
            memcpy(&dst[i * src.bpp], &src.raw_pixels[src_index], src.bpp);
        }
    }
    
    void read_row_rotated270(unsigned char* dst, row_selector const& src) {
        // read starts from the left bottom edge of the image
        for(int i = 0; i < src.dims.height; ++i) {
            size_t src_index = ((src.dims.height - 1 - i) * src.dims.width + src.row_index) * src.bpp;
            memcpy(&dst[i * src.bpp], &src.raw_pixels[src_index], src.bpp);
        }
    }
    

    /// Fetcher's dispatch table
    std::map<raw_pixel_area::rotation, row_fetcher> fetcher_table = {
        {raw_pixel_area::rotate_0_degree, &read_row_rotated0},
        {raw_pixel_area::rotate_90_degree, &read_row_rotated90},
        {raw_pixel_area::rotate_180_degree, &read_row_rotated180},
        {raw_pixel_area::rotate_270_degree, &read_row_rotated270},
    };
    
    
}

struct raw_pixel_area_impl::pimpl {
    raw_area_props* props;
    size        orig_dims;  ///< Original dimensions of the area
    row_fetcher fetcher;    ///< Pixel row fetcher
    int         bpp = 0;    ///< Bytes per pixel
};

raw_pixel_area_impl::raw_pixel_area_impl(): _pimpl(new pimpl) {
    ;;
}

raw_pixel_area_impl::~raw_pixel_area_impl() {
    ;;
}

void raw_pixel_area_impl::init_props_storage(raw_area_props* props) {
    _pimpl->props = props;
}

void raw_pixel_area_impl::reset() {
    assert(_pimpl->props && "Invalid props storage!");
    
    auto props = _pimpl->props;
    
    _pimpl->bpp = pixel_format_details(props->format).bpp;
    _pimpl->orig_dims = props->dimensions;
    _pimpl->fetcher = &read_row_rotated0;
}


size raw_pixel_area_impl::get_dimensions() const {
    assert(_pimpl->props && "Invalid props storage!");
    return _pimpl->props->dimensions;
}

void raw_pixel_area_impl::set_rotator(rotation r) {
    assert(_pimpl->props && "Invalid props storage!");
    _pimpl->fetcher = fetcher_table.find(r)->second;

    _pimpl->props->dimensions = _pimpl->orig_dims;
    if(r == rotate_90_degree || r == rotate_270_degree) {
        _pimpl->props->dimensions.width = _pimpl->orig_dims.height;
        _pimpl->props->dimensions.height = _pimpl->orig_dims.width;
    }
}

pixel_format raw_pixel_area_impl::get_pixel_format() const {
    return _pimpl->props->format;
}


unsigned char* raw_pixel_area_impl::get_raw_pixels() const {
    return _pimpl->props->data.get();
}


void raw_pixel_area_impl::read_row(unsigned char* dst, int row) const {
    _pimpl->fetcher(dst,
                    row_selector()
                    .set_row_index(row)
                    .set_bpp(_pimpl->bpp)
                    .set_raw_pixels(get_raw_pixels())
                    .set_dims(_pimpl->orig_dims)
                    );
}

