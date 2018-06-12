#pragma once

#include "image.hpp"

namespace atlas2d {
    
    struct raw_area_props {
        pixel_format    format;     ///< Pixel format
        size            dimensions; ///< Dimensions of the array
        raw_data_ptr    data;       ///< Pixel's raw data
    };
    
    namespace details {
        
        /// Raw pixel area's implementation
        class raw_pixel_area_impl {
        public:
            enum rotation {
                rotate_0_degree,
                rotate_90_degree,
                rotate_180_degree,
                rotate_270_degree
            };
            
            /// Set a rotator for pixels fetching
            virtual void set_rotator(rotation rot);

        protected:
            ~raw_pixel_area_impl();
            raw_pixel_area_impl();

            /// Set up the storage of properties
            virtual void init_props_storage(raw_area_props* props);
            
            /// Initialize the raw area by specific properties
            virtual void reset();
            
            /// Reads specific row to a preallocated buffer.
            void read_row(unsigned char* dst, int row) const;
            
            unsigned char* get_raw_pixels() const;
            
            size get_dimensions() const;
            pixel_format get_pixel_format() const;
            
        private:
            struct pimpl;
            std::unique_ptr<pimpl> _pimpl;
        };
        
        template<typename BaseT, typename PropsT>
        class raw_pixel_area_adapter: public BaseT, public raw_pixel_area_impl {
        protected:
            ~raw_pixel_area_adapter() { init_props_storage(&_props); }
            raw_pixel_area_adapter() { ;; }

        public:
            /// Initialize the area
            void init(PropsT p) {
                _props = std::move(p);
                init_props_storage(&_props);
                reset();
            }
            
            /// Returns the properties of the area
            PropsT const& props() { return _props; }
            
            // Just forward the calls above to the implementation class
            
            virtual void read_row(unsigned char* dst, int row) const { return raw_pixel_area_impl::read_row(dst, row); }
            virtual unsigned char* get_raw_pixels() const { return raw_pixel_area_impl::get_raw_pixels(); }
            virtual size get_dimensions() const { return raw_pixel_area_impl::get_dimensions(); }
            virtual pixel_format get_pixel_format() const { return raw_pixel_area_impl::get_pixel_format(); }
            
        protected:
            PropsT _props;
        };
        
    } // namespace detail
    
    /// Raw pixel area
    class raw_pixel_area: public details::raw_pixel_area_adapter<pixel_area, raw_area_props> {
        using base = details::raw_pixel_area_adapter<pixel_area, raw_area_props>;
        
    public:
        /// Area's properties
        struct init_props: raw_area_props {
            init_props& set_dims(size arg) { dimensions = std::move(arg); return *this;}
            init_props& set_pixel_format(pixel_format arg) { format = std::move(arg); return *this;}
            init_props& set_raw_data(raw_data_ptr arg) {data = std::move(arg); return *this;}
        };
    };
    
} // namespace atlas2d
