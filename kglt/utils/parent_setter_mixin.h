#ifndef PARENT_SETTER_MIXIN_H
#define PARENT_SETTER_MIXIN_H

#include "../types.h"

namespace kglt {

//Subclassing this allows us to set parents using
//IDs, rather than references which makes for a safer API
template<typename T>
class ParentSetterMixin : public T {
public:
    ParentSetterMixin(Stage* stage):
        T(stage) {}

    void set_parent(ActorID actor) {
        T::set_parent(*(T::stage().actor(actor).__object));
    }

    void set_parent(LightID light) {
        T::set_parent(T::stage().light(light));
    }

    void set_parent(CameraID camera) {
        T::set_parent(*T::stage().camera(camera).__object);
    }

private:
    using T::set_parent;

    friend class Stage;
};

}
#endif // PARENT_SETTER_MIXIN_H