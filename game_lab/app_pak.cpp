#include "app_media.hpp"
#include "original_pak.hpp"
namespace game_lab {
OriginalMapCatalog AppMedia::originalMapCatalog() const{return enumerateOriginalMapSources(root_);}
OriginalMapSelection AppMedia::originalMapSource(const OriginalMapSource& source) const{return loadOriginalMapSource(root_,source);}
}
