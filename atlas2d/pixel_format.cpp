#include "pixel_format.hpp"
#include <algorithm>
#include <set>

using namespace ::atlas2d;
using namespace ::std;

namespace {
    
    /// Wrapper for the format_details to put it into a set
    struct format_item: format_details {
        format_item() { ;; }
        explicit format_item(pixel_format f) {
            this->format = f;
        }
        
        bool operator <(format_item const& rhs) const {
            return format < rhs.format;
        }

        format_item& set_name(string a) {
            formatName = move(a);
            return *this;
        }
        
        format_item& set_format(pixel_format a) {
            format = a;
            return *this;
        }
        
        format_item& set_bpp(int a) {
            bpp = a;
            return *this;
        }
    };
    
    /// The table of known format details
    static set<format_item> tbl = {
        format_item().set_format(pixel_format::rgb565).set_name("rgb565").set_bpp(2),
        format_item().set_format(pixel_format::rgb8).set_name("rgb8").set_bpp(3),
        format_item().set_format(pixel_format::rgba8).set_name("rgba8").set_bpp(4),
        format_item().set_format(pixel_format::rgba4).set_name("rgba4").set_bpp(2),
    };
    
    /// The invalid value
    static format_item not_found;
}


format_details const& atlas2d::pixel_format_details(pixel_format f) {
    auto p = tbl.find(format_item(f));
    
    return p == tbl.end() ? not_found : *p;
}

format_details const& atlas2d::pixel_format_details(std::string const& name) {
    auto p = find_if(tbl.begin(), tbl.end(), [&](format_item const& i){
        return i.formatName == name;
    });
    
    return p == tbl.end() ? not_found : *p;
}
