#ifndef RPG_SPRITEANIMATION_HPP
#define RPG_SPRITEANIMATION_HPP

#include "../graphics/Rect.h"
#include <vector>
#include <yaml-cpp/yaml.h>

struct SpriteAnimationFrame
{
    IntRect rect;
    float duration;
};

struct SpriteAnimation
{
    std::vector<SpriteAnimationFrame> frames;
};

namespace YAML
{

#define __IMPL(T)                                                                                                                          \
    template <> struct convert<T>                                                                                                          \
    {                                                                                                                                      \
        static bool decode(const Node &node, T &rhs);                                                                                      \
    }

__IMPL(SpriteAnimationFrame);
__IMPL(SpriteAnimation);

#undef __IMPL

}

#endif // RPG_SPRITEANIMATION_HPP
