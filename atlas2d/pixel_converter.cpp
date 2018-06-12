#include "pixel_converter.hpp"
#include "pixel_format.hpp"

#include <cassert>
#include <set>
#include <memory>
#include <deque>
#include <vector>
#include <algorithm>

using namespace ::atlas2d;
using namespace ::std;

namespace {
    
    // Functions to convert pixel formats
    
    void mirror_rgba8(unsigned char* src, unsigned char* dst, size_t count) {
        for(size_t i = 0; i < count; ++i) {
            uint32_t* dstPixel = (uint32_t*)&dst[i*4];
            uint32_t* srcPixel = (uint32_t*)&src[(count - i - 1)*4];
            *dstPixel = *srcPixel;
        }
    }

    void mirror_rgba4(unsigned char* src, unsigned char* dst, size_t count) {
        for(size_t i = 0; i < count; ++i) {
            uint16_t* dstPixel = (uint16_t*)&dst[i*2];
            uint16_t* srcPixel = (uint16_t*)&src[(count - i - 1)*2];
            *dstPixel = *srcPixel;
        }
    }
    
    void mirror_pixels(unsigned char* src, unsigned char* dst, size_t count, size_t bpp) {
        switch (bpp) {
            case 4:
                mirror_rgba8(src, dst, count);
                break;
            case 2:
                mirror_rgba4(src, dst, count);
                break;
            default:
                // For other bpp
                for(size_t i = 0; i < count; ++i) {
                    unsigned char* dstPixel = (unsigned char*)&dst[i*bpp];
                    unsigned char* srcPixel = (unsigned char*)&src[(count - i - 1)*bpp];
                    std::memcpy(dstPixel, srcPixel, bpp);
                }
                break;
        }
    }
    
    unsigned char* mirror_to_margins(unsigned char* buffer,
                                     size_t count,
                                     size_t bpp,
                                     size_t left_margin,
                                     size_t right_margin)
    {
        // mirror to the left margin
        auto min_left_margin = (std::min)(left_margin, count);
        mirror_pixels(&buffer[left_margin*bpp],
                      &buffer[0],
                      min_left_margin,
                      bpp);
        
        // mirror to the right size
        auto min_right_margin = (std::min)(right_margin, count);
        mirror_pixels(&buffer[(left_margin + count - min_right_margin)*bpp],
                      &buffer[(left_margin + count)*bpp],
                      min_right_margin,
                      bpp);
        
        return buffer;
    }
    
    void copy_pixels(int bpp, unsigned char* src, unsigned char* dst, size_t count) {
        memcpy(dst, src, count * bpp);
    }

    void premultiple_rgba8(unsigned char* src, unsigned char* dst, size_t count) {
        memcpy(dst, src, count * 4);

        for (int i = 0; i < count; ++i) {
            int index = i * 4;
            float factor = (float)dst[index + 3] / 255.0f;
            
            dst[index + 0] = (unsigned char)((float)dst[index + 0] * factor);
            dst[index + 1] = (unsigned char)((float)dst[index + 1] * factor);
            dst[index + 2] = (unsigned char)((float)dst[index + 2] * factor);
        }
    }

    void rgb8_to_rgba8(unsigned char* src, unsigned char* dst, size_t count) {
        const unsigned char alpha = 0xff;
        for(size_t i = 0; i < count; ++i) {
            auto index3 = i * 3;
            auto index4 = i * 4;
            dst[index4] = src[index3];
            dst[index4 + 1] = src[index3 + 1];
            dst[index4 + 2] = src[index3 + 2];
            dst[index4 + 3] = alpha;
        }
    }

    void rgba8_to_rgba4(unsigned char* src, unsigned char* dst, size_t count) {
        uint32_t* inPixel32 = (uint32_t*)src;
        uint16_t* outPixel16 = (uint16_t*)dst;
        
        for(unsigned int i = 0; i < count; ++i, ++inPixel32) {
            *outPixel16++ =
            ((((*inPixel32 >> 0) & 0xFF) >> 4) << 12) | // R
            ((((*inPixel32 >> 8) & 0xFF) >> 4) <<  8) | // G
            ((((*inPixel32 >> 16) & 0xFF) >> 4) << 4) | // B
            ((((*inPixel32 >> 24) & 0xFF) >> 4) << 0);  // A
        }
    }

    void rgba8_to_alpha_grayscale(unsigned char* src, unsigned char* dst, size_t count) {
        uint32_t* inPixel32 = (uint32_t*)src;
        uint16_t* outPixel16 = (uint16_t*)dst;
        
        for(unsigned int i = 0; i < count; ++i, ++inPixel32) {
            uint16_t pixel = ((*inPixel32 >> 0) & 0xFF);
            pixel += ((*inPixel32 >> 8) & 0xFF);
            pixel += ((*inPixel32 >> 16) & 0xFF);
            pixel /= 3;
            pixel = pixel & 0x00FF;
            
            pixel += ((*inPixel32 >> 24) & 0xFF) << 8;
            
            *outPixel16++ = pixel;
        }
    }

} // namespace

namespace {
    
    // Graph API to build the right converter
    
    /// An entry for a graph that describes the way to convert from the one format to another
    class graph_entry: public pixel_converter {
    public:
        graph_entry() { ;; }
        graph_entry(pixel_format src) {
            reset(properties().set_src_format(src));
        }
        graph_entry(basic_props const& p) {
            reset(p);
        }
        
        bool operator<(graph_entry const& lhs) const {
            return src_format() < lhs.src_format();
        }
        
        pixel_format dst_format() const { return props().dst_format; }
        pixel_format src_format() const { return props().src_format; }
    };
    
    /// Conversion graph
    using convgraph = multiset<graph_entry>;
    
    /// Returns the entire graph.
    convgraph const& conversion_graph() {
        static convgraph g = {
            {graph_entry::properties()
                .set_src_format(pixel_format::rgba8)
                .set_dst_format(pixel_format::rgba8)
                .set_callback(bind(&copy_pixels, 4, placeholders::_1, placeholders::_2, placeholders::_3))
            },
            {graph_entry::properties()
                .set_src_format(pixel_format::rgba8)
                .set_dst_format(pixel_format::rgba4)
                .set_callback(&rgba8_to_rgba4)
            },
            /*{graph_entry::properties()
                .set_src_format(pixel_format::rgba8)
                .set_dst_format(pixel_format::ai8)
                .set_callback(&convertRgba8ToGrayscaleWithAlpha)
            },*/
            {graph_entry::properties()
                .set_src_format(pixel_format::rgb8)
                .set_dst_format(pixel_format::rgba8)
                .set_callback(&rgb8_to_rgba8)
            },
        };
        
        return g;
    }
    
    vector<graph_entry> find_conversion_path(pixel_format src, pixel_format dst) {
        convgraph const& graph = conversion_graph();
        
        using range = std::pair<convgraph::const_iterator, convgraph::const_iterator>;
        deque<range> traverse;
        
        traverse.push_front(graph.equal_range(src));
        
        while(!traverse.empty()) {
            auto& childs = traverse.front();
            if(childs.first == childs.second) {
                // Remove iterated node
                traverse.pop_front();

                // Move parent to a next node
                if(!traverse.empty()) {
                    auto& prev_childs = traverse.front();
                    ++(prev_childs.first);
                }
                continue;
            }
            
            auto& node = childs.first;
            auto const& details = *node;
            if(details.dst_format() == dst)
                // Found the way!
                break;
            
            auto next_childs = graph.equal_range(graph_entry(details.dst_format()));
            if(next_childs.first == node)
                // Prevent loops
                next_childs.first = next_childs.second = graph.end();
            
            traverse.push_front(next_childs);
        }

        // Copy results to the appropriate format
        vector<graph_entry> path(traverse.size());
        transform(traverse.rbegin(),
                  traverse.rend(),
                  path.begin(),
                  [](range const& childs){
                      return *(childs.first);
                  });
        
        return path;
    }
    
    pixel_converter_ptr create_format_converter(pixel_format src_fmt,
                                                pixel_format dst_fmt,
                                                size_t pixels_count,
                                                bool premultiple)
    {
        auto converters = find_conversion_path(src_fmt, dst_fmt);
        if(!converters.empty() && premultiple && converters[0].src_format() == pixel_format::rgba8) {
            // add premultiple stage
            auto props = converters[0].props();
            props.dst_format = props.src_format;
            props.cb = &premultiple_rgba8;
            
            converters.insert(converters.begin(), graph_entry(props));
        }
        
        // Select the highest bpp of convertion path
        int bpp = pixel_format_details(dst_fmt).bpp;
        for(auto const& details : converters)
            bpp = (std::max)(bpp, pixel_format_details(details.dst_format()).bpp);
        
        const size_t buffer_size = pixels_count * bpp;
        
        // Allocate buffers
        using buffer_ptr = std::shared_ptr<unsigned char>;

        const int BUFFERS_COUNT = 2;
        const int MIN_STEPS_FOR_BUFFERING = 2;

        std::array<buffer_ptr, 2> buffers;
        if(converters.size() == MIN_STEPS_FOR_BUFFERING) {
            // We are needed buffering only when there are at least 2 conversion steps.
            buffers[0].reset((unsigned char*)std::malloc(buffer_size), [](unsigned char* buff){ std::free(buff);});
        }
        
        if(converters.size() > MIN_STEPS_FOR_BUFFERING) {
            // We are needed the second buffer only when there are 3 or more conversion steps.
            buffers[1].reset((unsigned char*)std::malloc(buffer_size), [](unsigned char* buff){ std::free(buff);});
        }
        
        auto const& max_pixels_count = pixels_count;
        
        auto convert_fn = [=](unsigned char* src_buf, unsigned char* dst_buf, size_t pixels_count) {
            assert(pixels_count <= max_pixels_count);
            
            // Setup first step
            unsigned char* input_buff = src_buf;
            
            int next_buffer_id = 0;
            for(size_t i = 0; i < converters.size(); ++i) {
                const bool is_next_to_last = (i + 1 == converters.size());
				unsigned char* output_buff = is_next_to_last ? dst_buf : buffers[next_buffer_id].get();

                converters[i](input_buff, output_buff, pixels_count);
                
                // swap buffers
                input_buff = output_buff;
                
                // Roll the buffers
                if(++next_buffer_id > BUFFERS_COUNT)
                    next_buffer_id = 0;
            }
            
        };
        
        return make_shared<pixel_converter>(pixel_converter::properties()
                                            .set_src_format(src_fmt)
                                            .set_dst_format(dst_fmt)
                                            .set_callback(convert_fn));
    }

} // namespace

pixel_converter_ptr atlas2d::create_pixel_converter(converter_params const& params) {
    
    // Here we need a data converter
    pixel_converter_ptr converter = create_format_converter(params.src_fmt,
                                                            params.dst_fmt,
                                                            params.pixels_count,
                                                            params.premultiple);
    if(!converter) {
        // No situable converter found
        return nullptr;
    }
    
    auto margins = params.margins;
    size_t bpp = pixel_format_details(converter->props().dst_format).bpp;
    
    auto convert_and_mirror = [=]\
    (unsigned char* src_buf, unsigned char* dst_buf, size_t pixels_num){
        
        // Convert the only pixels_num
        (*converter)(src_buf,
                     &dst_buf[margins[0] * bpp],
                     pixels_num);
        
        // Mirror the content of the dst_buffer to its margins
        mirror_to_margins(dst_buf,
                          pixels_num,
                          bpp,
                          margins[0],
                          margins[1]);
    };
    
    return make_shared<pixel_converter>(pixel_converter::properties()
                                        .set_callback(convert_and_mirror)
                                        .set_src_format(params.src_fmt)
                                        .set_dst_format(params.dst_fmt));
}
