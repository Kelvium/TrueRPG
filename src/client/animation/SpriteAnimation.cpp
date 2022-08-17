#include "../../pch.h"
#include "SpriteAnimation.h"

namespace YAML
{

bool convert<SpriteAnimationFrame>::decode(const Node &node, SpriteAnimationFrame &rhs)
{
    if (!node.IsMap())
    {
        return false;
    }

    rhs.rect = node["rect"].as<IntRect>();
    rhs.duration = node["duration"].as<float>();
    return true;
}

bool convert<SpriteAnimation>::decode(const Node &node, SpriteAnimation &rhs)
{
    if (!node.IsMap())
    {
        return false;
    }

    rhs.frames = node["frames"].as<std::vector<SpriteAnimationFrame>>();
    return true;
}

} // namespace YAML
