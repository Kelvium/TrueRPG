#include "SpriteAnimator.h"

SpriteAnimatorBuilder::SpriteAnimatorBuilder()
{
    m_animator.nodes.push_back({"entry"});
}

SpriteAnimatorBuilder::Node SpriteAnimatorBuilder::entry()
{
    return {&m_animator, &m_animator.nodes.front()};
}

SpriteAnimatorBuilder::Node SpriteAnimatorBuilder::node(std::string name, std::vector<SpriteAnimationFrame> frames)
{
    auto *n = &m_animator.nodes.emplace_back(SpriteAnimatorNode{std::move(name), std::move(frames)});

    return {&m_animator, n};
}

SpriteAnimatorBuilder::Parameter SpriteAnimatorBuilder::parameter(std::string name, SpriteAnimatorParameterType type)
{
    auto *p = &m_animator.parameters.emplace_back(SpriteAnimatorParameter{std::move(name), type});

    return {&m_animator, p};
}

SpriteAnimator SpriteAnimatorBuilder::build()
{
    return std::move(m_animator);
}

SpriteAnimatorBuilder::Node::Node(SpriteAnimator *animator, SpriteAnimatorNode *node)
    : m_animator(animator),
      m_node(node)
{
}

void SpriteAnimatorBuilder::Node::transition(
    SpriteAnimatorBuilder::Node &destination, std::vector<std::function<bool(const SpriteAnimatorParameterStorage &)>> conditions)
{
    SpriteAnimatorTransition *t =
        &m_animator->transitions.emplace_back(SpriteAnimatorTransition{m_node, destination.m_node, std::move(conditions)});

    m_node->fromTransitions.push_back(t);
    destination.m_node->toTransitions.push_back(t);
}

SpriteAnimatorBuilder::Parameter::Parameter(SpriteAnimator *animator, SpriteAnimatorParameter *parameter)
    : m_animator(animator),
      m_parameter(parameter)
{
}

namespace YAML
{

// FIXME: this is bad!
static SpriteAnimator *animator = nullptr;

static SpriteAnimatorNode *findNodeByName(const std::string& name)
{
    for (auto &node : animator->nodes)
    {
        if (node.name == name)
        {
            return &node;
        }
    }
    return nullptr;
}

static SpriteAnimatorParameter *findParameterByName(const std::string& name)
{
    for (auto &parameter : animator->parameters)
    {
        if (parameter.name == name)
        {
            return &parameter;
        }
    }
    return nullptr;
}

bool convert<SpriteAnimatorParameterType>::decode(const Node &node, SpriteAnimatorParameterType &rhs)
{
    const auto &type = node.as<std::string>();
    if (type == "vec2")
    {
        rhs = SpriteAnimatorParameterType::Vec2;
    }
    else
    {
        return false;
    }
    return true;
}

bool convert<SpriteAnimatorParameter>::decode(const Node &node, SpriteAnimatorParameter &rhs)
{
    if (!node.IsMap())
    {
        return false;
    }

    rhs.name = node["name"].as<std::string>();
    rhs.type = node["type"].as<SpriteAnimatorParameterType>();
    return true;
}

bool convert<SpriteAnimatorNode>::decode(const Node &node, SpriteAnimatorNode &rhs)
{
    if (!node.IsMap())
    {
        return false;
    }

    rhs.name = node["name"].as<std::string>();
    rhs.animation = node.as<SpriteAnimation>();
    return true;
}

bool convert<SpriteAnimatorTransition>::decode(const Node &node, SpriteAnimatorTransition &rhs)
{
    if (!node.IsMap())
    {
        return false;
    }

    const auto &from = node["from"];
    const auto &to = node["to"];
    assert(from && to);

    rhs.source = findNodeByName(from.as<std::string>());
    rhs.destination = findNodeByName(to.as<std::string>());

    const auto &conditions = node["conditions"];
    if (conditions)
    {
        for (auto &condition : conditions)
        {
            const auto &parameterNode = condition["parameter"];
            if (!parameterNode)
            {
                return false;
            }
            const auto *parameter = findParameterByName(parameterNode.as<std::string>());

            const auto &xNode = condition["x"];
            const auto &yNode = condition["y"];
            if (!xNode && !yNode)
            {
                return false;
            }
#define __CONDITION(kind, x, x1) \
            if (x[#kind]) \
            { \
                /* FIXME: T!!! */ \
                return std::get<glm::vec2>(storage.at(parameter->name)).x1 kind x[#kind].as<float>(); \
            }

            if (xNode)
            {
                rhs.conditions.push_back([parameter = findParameterByName(parameterNode.as<std::string>()), xNode, yNode](const SpriteAnimatorParameterStorage &storage)
                {
                    __CONDITION(>, xNode, x);
                    __CONDITION(>=, xNode, x);
                    __CONDITION(<, xNode, x);
                    __CONDITION(<=, xNode, x);
                    __CONDITION(!=, xNode, x);
                    __CONDITION(==, xNode, x);

                    return false;
                });
            }
            else if (yNode)
            {
                rhs.conditions.push_back([parameter = findParameterByName(parameterNode.as<std::string>()), xNode, yNode](const SpriteAnimatorParameterStorage &storage)
                {
                    __CONDITION(>, yNode, y);
                    __CONDITION(>=, yNode, y);
                    __CONDITION(<, yNode, y);
                    __CONDITION(<=, yNode, y);
                    __CONDITION(!=, yNode, y);
                    __CONDITION(==, yNode, y);

                    return false;
                });
            }

#undef __CONDITION
        }
    }
    else
    {
        // no conditions defined means always true
        rhs.conditions.push_back([](const SpriteAnimatorParameterStorage &) { return true; });
    }

    return true;
}

bool convert<SpriteAnimator>::decode(const Node &node, SpriteAnimator &rhs)
{
    if (!node.IsMap())
    {
        return false;
    }

    animator = &rhs;

    rhs.nodes.push_back({"entry"});
    for (const auto &spriteAnimatorNode : node["nodes"].as<std::list<SpriteAnimatorNode>>())
    {
        rhs.nodes.push_back(spriteAnimatorNode);
    }

    rhs.transitions = node["transitions"].as<std::list<SpriteAnimatorTransition>>();

    rhs.parameters = node["parameters"].as<std::list<SpriteAnimatorParameter>>();
    return true;
}

}; // namespace YAML
