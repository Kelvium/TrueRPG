#include "RenderSystem.h"

#include <entt.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "../../client/window/Window.h"
#include "../../client/graphics/Text.h"
#include "../../client/graphics/Light.h"
#include "../components/render/CameraComponent.h"
#include "../components/render/SpriteRendererComponent.h"
#include "../components/render/TextRendererComponent.h"
#include "../components/basic/HierarchyComponent.h"
#include "../utils/Hierarchy.h"
#include "../components/world/WorldMapComponent.h"
#include "../components/render/AutoOrderComponent.h"
#include "../components/render/GlobalLightComponent.h"
#include "../components/render/LightSourceComponent.h"

#if defined(M_PI) || defined(M_PI_2)
#undef M_PI
#undef M_PI_2
#define M_PI 3.14159265358979323846f	/* pi */
#define M_PI_2 1.57079632679489661923f	/* pi/2 */
#endif

RenderSystem::RenderSystem(entt::registry &registry)
        : m_registry(registry),
          m_shader(Shader::createShader("../res/shaders/shader.vs", "../res/shaders/shader.fs")),
          m_batch(m_shader, 30000)
{
    Window::getInstance().onResize += createEventHandler(&RenderSystem::resize);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RenderSystem::fixedUpdate()
{
	{
        auto view = m_registry.view<GlobalLightComponent>();
        for (auto entity : view)
        {
            auto &lightComponent = view.get<GlobalLightComponent>(entity);

            if (lightComponent.dayNightCycleEnable)
            {
                ++lightComponent.time;
            }
        }
    }

}

void RenderSystem::draw(float deltaTime)
{
    // Find the camera
    CameraComponent *cameraComponent = nullptr;
    TransformComponent cameraTransform;
    {
        auto view = m_registry.view<CameraComponent>();
        for (auto entity : view)
        {
            cameraComponent = &view.get<CameraComponent>(entity);
            cameraTransform = Hierarchy::computeTransform({entity, &m_registry});
        }
    }
    if (cameraComponent != nullptr)
    {
        glm::vec4 back = cameraComponent->background;
        glClearColor(back.r, back.g, back.b, back.a);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 viewMatrix = glm::translate(glm::mat4(1), glm::vec3(-cameraTransform.position, 0));
        m_batch.setViewMatrix(viewMatrix);
        m_batch.setProjectionMatrix(cameraComponent->getProjectionMatrix());
        m_batch.begin();

        // World map rendering
        {
            auto view = m_registry.view<WorldMapComponent>();
            for (auto entity : view)
            {
                auto &worldMapComponent = view.get<WorldMapComponent>(entity);
                auto transformComponent = Hierarchy::computeTransform({entity, &m_registry});

                int currentX = (int) std::floor(cameraTransform.position.x / ((float) worldMapComponent.tileSize * transformComponent.scale.x));
                int currentY = (int) std::floor(cameraTransform.position.y / ((float) worldMapComponent.tileSize * transformComponent.scale.y));

                for (int y = currentY + worldMapComponent.renderRadius - 1; y >= currentY - worldMapComponent.renderRadius + 1; y--)
                {
                    for (int x = currentX - worldMapComponent.renderRadius + 1; x < currentX + worldMapComponent.renderRadius; x++)
                    {
                        // Generate tiles
                        std::vector<Tile> tiles = worldMapComponent.generator->generateTiles(x, y);
                        for (const auto &tile : tiles)
                        {
                            Sprite tileSprite(*tile.texture);
                            tileSprite.setTextureRect(tile.textureRect);
                            tileSprite.setPosition(glm::vec2(x, y) * (float) worldMapComponent.tileSize * transformComponent.scale);
                            tileSprite.setScale(transformComponent.scale);

                            m_batch.draw(tileSprite, worldMapComponent.tileLayer);
                        }
                        // Generate objects
                        std::vector<Object> objects = worldMapComponent.generator->generateObjects(x, y, tiles);
                        for (const auto &object : objects)
                        {
                            Sprite objectSprite(*object.texture);
                            objectSprite.setTextureRect(object.textureRect);
                            objectSprite.setPosition(glm::vec2(x, y) * (float) worldMapComponent.tileSize * transformComponent.scale);
                            objectSprite.setOrigin(object.origin);
                            objectSprite.setScale(transformComponent.scale);

                            m_batch.draw(objectSprite, worldMapComponent.objectLayer,-(int) objectSprite.getPosition().y - object.orderPivot);
                        }
                    }
                }
            }
        }

        // Sprites rendering
        {
            auto view = m_registry.view<SpriteRendererComponent>();
            for (auto entity : view)
            {
                auto &spriteComponent = view.get<SpriteRendererComponent>(entity);
                Sprite sprite(spriteComponent.texture);
                sprite.setTextureRect(spriteComponent.textureRect);
                sprite.setColor(spriteComponent.color);

                auto transformComponent = Hierarchy::computeTransform({entity, &m_registry});

                sprite.setPosition(transformComponent.position);
                sprite.setOrigin(transformComponent.origin);
                sprite.setScale(transformComponent.scale);

                int order = spriteComponent.order;
                if (m_registry.all_of<AutoOrderComponent>(entity))
                {
                    auto &orderComponent = m_registry.get<AutoOrderComponent>(entity);
                    order = -(int) transformComponent.position.y - orderComponent.orderPivot;
                }

                m_batch.draw(sprite, spriteComponent.layer, order);
            }
        }

        Light light(m_shader);
        light.clean();
		i32 time = 0;
        // global light
        {
            auto view = m_registry.view<GlobalLightComponent>();
            for (auto entity : view)
            {
                auto &wnd = Window::getInstance();
                auto &lightComponent = view.get<GlobalLightComponent>(entity);
				
                m_shader.setUniform("resolution", glm::vec2(wnd.getWidth(), wnd.getHeight()));

				time = lightComponent.time % 24000;
				float ambient = (glm::tanh(4 * glm::sin((time / 12000.f) * M_PI - M_PI_2)) * 0.5f + 0.5f) * lightComponent.intensity;

                light.setColor(lightComponent.color);
                light.setPosition(glm::vec2(wnd.getWidth() / 2.f, wnd.getHeight() / 2.f));
                light.setIntensity(lightComponent.intensity);
                light.setRadius(wnd.getHeight());
                m_shader.setUniform("dayTime", lightComponent.time);
				m_shader.setUniform("ambient", ambient);

                light.draw();
            }
        }

		// light source
		{
			auto view = m_registry.view<LightSourceComponent>();

			for (auto entity : view)
			{
				auto &wnd = Window::getInstance();
                float w = static_cast<float>(wnd.getWidth());
                float h = static_cast<float>(wnd.getHeight());
				LightSourceComponent &lightSource = view.get<LightSourceComponent>(entity);

             	auto transform = Hierarchy::computeTransform({entity, &m_registry});

                glm::vec4 spacePos = cameraComponent->getProjectionMatrix() * (viewMatrix * glm::vec4(transform.position, 0.0f, 1.0f));
                glm::vec3 ndcSpacePos = spacePos / spacePos.w;
                glm::vec2 windowSpacePos = (((glm::vec2(ndcSpacePos) + 1.0f) / 2.0f) * glm::vec2(w, h));

                if (windowSpacePos.x + lightSource.radius < 0 ||
                    windowSpacePos.y + lightSource.radius < 0 ||
                    windowSpacePos.x - lightSource.radius > w ||
                    windowSpacePos.y - lightSource.radius > h)
                {
                    continue;
                }

				float intensity = (-glm::tanh(4 * glm::sin((time / 12000.f) * M_PI - M_PI / 2)) * 0.5f + 0.5f) * lightSource.intensity;

                light.setColor(lightSource.color);
                light.setPosition(windowSpacePos);
                light.setIntensity(intensity);
                light.setRadius(lightSource.radius);
                light.draw();
			}
		}

        // Text rendering
        {
            auto view = m_registry.view<TextRendererComponent>();
            for (auto entity : view)
            {
                auto &textComponent = view.get<TextRendererComponent>(entity);
                Text text(*textComponent.font, textComponent.text);
                text.setColor(textComponent.color);

                auto transformComponent = Hierarchy::computeTransform({entity, &m_registry});

                text.setPosition(transformComponent.position);
                FloatRect localBound = text.getLocalBounds();
                glm::vec2 textOrigin = transformComponent.origin;
                if (textComponent.horizontalAlign == HorizontalAlign::Center)
                {
                    textOrigin += glm::vec2(localBound.getWidth() / 2, 0.f);
                }
                if (textComponent.horizontalAlign == HorizontalAlign::Right)
                {
                    textOrigin += glm::vec2(localBound.getWidth(), 0.f);
                }
                if (textComponent.verticalAlign == VerticalAlign::Center)
                {
                    textOrigin += glm::vec2(0.f, localBound.getHeight() / 2);
                }
                if (textComponent.verticalAlign == VerticalAlign::Top)
                {
                    textOrigin += glm::vec2(0.f, localBound.getHeight());
                }
                text.setOrigin(textOrigin);
                text.setScale(transformComponent.scale);

                text.draw(m_batch, textComponent.layer, textComponent.order);
            }
        }
        m_batch.end();
    }
}

void RenderSystem::destroy()
{
    m_batch.destroy();
    m_shader.destroy();
}

// Every time when the window size is changed (by user or OS), this callback function is invoked
void RenderSystem::resize(int width, int height)
{
    glViewport(0, 0, width, height);
}
