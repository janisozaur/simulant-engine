#ifndef WAVEFRONT_LOADER_H_INCLUDED
#define WAVEFRONT_LOADER_H_INCLUDED

#include "../loader.h"

namespace GL {

class WavefrontLoader : public Loader {
public:
    void load_into(Resource& resource, const std::string& filename);


};

}


#endif // WAVEFRONT_LOADER_H_INCLUDED
