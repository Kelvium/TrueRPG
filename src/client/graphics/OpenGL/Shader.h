#ifndef RPG_SHADER_H
#define RPG_SHADER_H

#include "../Graphics.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../IShader.h"
#include "../ShaderInfo.h"
#include "UniformBuffer.h"

#include <string>
#include <iostream>
#include <functional>
#include <any>
#include <unordered_map>

#define UNIFORMTAG(ph_1, ph_2) glUniform ## ph_1 ## ph_2

#define callFuncs(type, ID, separate) callIfCallable(UNIFORMTAG(1, type), glGetUniformLocation(ID, val_name.c_str()), std::forward<Params>(params)...) separate \
    callIfCallable(UNIFORMTAG(2, type), glGetUniformLocation(ID, val_name.c_str()), std::forward<Params>(params)...) separate \
    callIfCallable(UNIFORMTAG(3, type), glGetUniformLocation(ID, val_name.c_str()), std::forward<Params>(params)...) separate \
    callIfCallable(UNIFORMTAG(4, type), glGetUniformLocation(ID, val_name.c_str()), std::forward<Params>(params)...)

#define callFuncsForVector(type, ID, separate) callIfCallable(UNIFORMTAG(1, type), glGetUniformLocation(ID, val_name.c_str()), 1, glm::value_ptr(std::forward<Params>(params)...)) separate \
    callIfCallable(UNIFORMTAG(2, type), glGetUniformLocation(ID, val_name.c_str()), 1, glm::value_ptr(std::forward<Params>(params)...)) separate \
    callIfCallable(UNIFORMTAG(3, type), glGetUniformLocation(ID, val_name.c_str()), 1, glm::value_ptr(std::forward<Params>(params)...)) separate \
    callIfCallable(UNIFORMTAG(4, type), glGetUniformLocation(ID, val_name.c_str()), 1, glm::value_ptr(std::forward<Params>(params)...))

#define callFuncsForMatrix(type, ID, separate) callIfCallable(UNIFORMTAG(type, 2fv), glGetUniformLocation(ID, val_name.c_str()), 1, GL_FALSE, glm::value_ptr(std::forward<Params>(params)...)) separate \
    callIfCallable(UNIFORMTAG(type, 3fv), glGetUniformLocation(ID, val_name.c_str()), 1, GL_FALSE, glm::value_ptr(std::forward<Params>(params)...)) separate  \
    callIfCallable(UNIFORMTAG(type, 4fv), glGetUniformLocation(ID, val_name.c_str()), 1, GL_FALSE, glm::value_ptr(std::forward<Params>(params)...))

template <typename F, typename... S>
static inline constexpr void callIfCallable(F&& func, S&& ...args)
{
    if constexpr (std::is_invocable_v<F, decltype(std::forward<S>(args))...>)
    {
        std::invoke(std::forward<F>(func), std::forward<S>(args)...);
    }
    else return;
}

template<std::size_t N, typename T, typename... types>
struct get_Nth_type
{
    using type = typename get_Nth_type<N - 1, types...>::type;
};

template<typename T, typename... types>
struct get_Nth_type<0, T, types...>
{
    using type = T;
};

template<std::size_t N, typename... Args>
using get_ = typename get_Nth_type<N, Args...>::type;


class Shader : public IShader
{
private:
    unsigned int m_id{};
    unsigned int m_mvpUniformIndex;
    unsigned int m_lightUniformIndex;
    MVPUniformBuffer m_mvpBuffer;
    LightUniformBuffer m_lightBuffer;
    ShaderEnabledUniform m_enabledUniform;

    std::unordered_map<std::string, IUniformBuffer *> m_uniforms;
    std::unordered_map<std::string, std::any> m_uniformVals;
public:
    Shader() = default;
    explicit Shader(unsigned int id);

    template <typename ... Params>
    void setHotUniform(const std::string &val_name, Params... params)
    {
        if constexpr (std::is_floating_point_v<get_<0, Params...>>)
        {
            callFuncs(f, m_id, ;);
        }
        else if constexpr (std::is_integral_v<get_<0,Params...>>)
        {
            if constexpr (sizeof...(Params) > 1)
            {
                if constexpr (std::is_same_v<get_<1, Params...>, float *>)
                {
                    callFuncs(fv, m_id, ;);
                }
                else if constexpr (std::is_pointer_v<get_<1, Params...>>)
                {
                    callFuncs(iv, m_id, ;);
                }
                else { callFuncs(i, m_id, ;); }
            }
            else { callFuncs(i, m_id, ;); }
        }
        else if constexpr (std::is_same_v<get_<0, Params...>, glm::vec2> ||
                           std::is_same_v<get_<0, Params...>, glm::vec3> ||
                           std::is_same_v<get_<0, Params...>, glm::vec4>)
        {
            callFuncsForVector(fv, m_id, ;);
        }
        else if constexpr (std::is_same_v<get_<0, Params...>, glm::mat4>)
        {
            callFuncsForMatrix(Matrix, m_id, ;);
        }
    }

    void setUniform(const std::string &val_name, glm::mat4 mat) override
    {
        // setHotUniform(val_name, mat);
        m_uniformVals[val_name] = mat;
    }

    void setUniform(const std::string &val_name, glm::vec2 vec) override
    {
        setHotUniform(val_name, vec);
        m_uniformVals[val_name] = vec;
    }

    void setUniform(const std::string &val_name, glm::vec3 vec) override
    {
        setHotUniform(val_name, vec);
        m_uniformVals[val_name] = vec;
    }

    void setUniform(const std::string &val_name, glm::vec4 vec) override
    {
        setHotUniform(val_name, vec);
        m_uniformVals[val_name] = vec;
    }

    void setUniform(const std::string &val_name, float val) override
    {
        setHotUniform(val_name, val);
        m_uniformVals[val_name] = val;
    }

    void setUniform(const std::string &val_name, int val) override
    {
        setHotUniform(val_name, val);
        m_uniformVals[val_name] = val;
    }

    void setUniform(const std::string &val_name, size_t size, int *arr) override
    {
        setHotUniform(val_name, size, arr);
        m_uniformVals[val_name] = std::vector<int>(arr, arr + size);
    }

    // Activate the shader
    void use() const override;

    void updateUbo() override
    {
        if (m_enabledUniform.useMVP)
        {
            auto &ubo = *m_uniforms["MVP"];
            MVPUniform mvp{};
            mvp.model = std::any_cast<glm::mat4>(m_uniformVals["model"]);
            mvp.view = std::any_cast<glm::mat4>(m_uniformVals["view"]);
            mvp.proj = std::any_cast<glm::mat4>(m_uniformVals["projection"]);

            ubo.setSubData(&mvp);
            ubo.bindBase();
        }
        if (m_enabledUniform.useGlobalLight)
        {
            auto &ubo = *m_uniforms["GlobalLight"];
            GlobalLightUniform globalLight{};
            globalLight.brightness = std::any_cast<float>(m_uniformVals["brightness"]);

            ubo.setSubData(&globalLight);
            ubo.bindBase(2);
        }
        if (m_enabledUniform.useLight)
        {
            auto &ubo = *m_uniforms["Light"];
            LightUniform light{};
            light.pos = std::any_cast<glm::vec2>(m_uniformVals["light.pos"]);
            light.color = std::any_cast<glm::vec3>(m_uniformVals["light.color"]);
            light.radius = std::any_cast<float>(m_uniformVals["light.radius"]);
            light.intensity = std::any_cast<float>(m_uniformVals["light.intensity"]);
            
            ubo.setSubData(&light);
            ubo.bindBase(2);
        }
    }

    unsigned int getId() const noexcept;

    void destroy() override;

    static Shader createShader(const std::string& vertexPath, const std::string& fragmentPath, ShaderEnabledUniform enabled = {});

private:
    // Some useful functions to check shader compilation/binding errors
    static unsigned int compileShader(const std::string& path, unsigned int type);
};

#endif //RPG_SHADER_H