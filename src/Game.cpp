#include "Game.h"

#include "scene/Entity.h"
#include "scene/components/CameraComponent.h"
#include "scene/components/NativeScriptComponent.h"
#include "scene/components/SpriteRendererComponent.h"
#include "scene/components/TextRendererComponent.h"
#include "scene/components/WorldMapComponent.h"

#include "scene/utils/Hierarchy.h"
#include "scripts/PlayerScript.h"
#include "scripts/TextScript.h"
#include "scripts/DebugInfoScript.h"
#include "scripts/WorldMapScript.h"
#include "scene/components/AudioListenerComponent.h"
#include "scripts/PumpkinScript.h"

Game::Game()
        : m_font("../res/fonts/vt323.ttf", 32),
          m_heroTexture(Texture::create("../res/textures/hero.png")),
          m_baseTexture(Texture::create("../res/textures/base.png")),
          m_steps("../res/audio/steps.mp3"),
          m_music("../res/audio/music.mp3")
{
    Entity worldMapEntity = m_scene.createEntity("worldMap");

    auto &worldTransform = worldMapEntity.getComponent<TransformComponent>();
    worldTransform.scale = glm::vec2(2.f, 2.f);
    worldTransform.origin = glm::vec2(0.5f, 0.5f);

    auto &worldMap = worldMapEntity.addComponent<WorldMapComponent>();
    worldMap.addTile(0, {&m_baseTexture, IntRect(192, 4256 - 32, 32, 32)});
    worldMap.addTile(1, {&m_baseTexture, IntRect(96, 4256 - 32, 32, 32)});
    worldMap.addTile(2, {&m_baseTexture, IntRect(160, 4256 - 32, 32, 32)});
    worldMap.addTile(3, {&m_baseTexture, IntRect(64, 4256 - 64, 32, 32)});
    worldMapEntity.addComponent<NativeScriptComponent>().bind<WorldMapScript>();


    m_cameraEntity = m_scene.createEntity("camera");
    m_cameraEntity.addComponent<CameraComponent>();


    // Создание текста
    Entity textEntity = m_scene.createEntity("text");
    auto &textRenderer = textEntity.addComponent<TextRendererComponent>(&m_font, "True RPG!\n Welcome!");

    // Некоторые настройки текста для примера
    textRenderer.horizontalAlign = HorizontalAlign::Center;
    textRenderer.verticalAlign = VerticalAlign::Top;
    textRenderer.layer = 10;

    // Биндим скрипт к энтити и передаем туда камеру
    textEntity.addComponent<NativeScriptComponent>().bind<TextScript>(m_cameraEntity);


    // Создание fps счетчика
    Entity debugInfoEntity = m_scene.createEntity("debugInfo");
    auto &debugText = debugInfoEntity.addComponent<TextRendererComponent>(&m_font, "");
    debugText.layer = 10;
    auto &fpsTransform = debugInfoEntity.getComponent<TransformComponent>();
    fpsTransform.scale = glm::vec2(0.8f, 0.8f);
    debugInfoEntity.addComponent<NativeScriptComponent>().bind<DebugInfoScript>(m_cameraEntity);


    // Создание игрока
    m_playerEntity = m_scene.createEntity("player");
    auto &playerTransform = m_playerEntity.getComponent<TransformComponent>();
    playerTransform.position = glm::vec2(0.0f, 0.0f);
    m_playerEntity.addComponent<AudioListenerComponent>();

    Entity spriteEntity = m_scene.createEntity("sprite");
    auto &heroRenderer = spriteEntity.addComponent<SpriteRendererComponent>(m_heroTexture);
    heroRenderer.textureRect = IntRect(32, 96, 32, 32);
    heroRenderer.layer = 1;

    auto &heroTransform = spriteEntity.getComponent<TransformComponent>();
    heroTransform.scale = glm::vec2(2.f, 2.f);
    heroTransform.origin = glm::vec2(16, 16);

    auto stepsSoundEntity = m_scene.createEntity("stepsSound");
    auto &stepsComponent = stepsSoundEntity.addComponent<AudioSourceComponent>(m_steps);
    stepsComponent.volume = 0.25f;
    stepsComponent.loop = true;

    // Крепим к игроку спрайт, звук, текст и камеру
    Hierarchy::addChild(m_playerEntity, spriteEntity);
    Hierarchy::addChild(m_playerEntity, stepsSoundEntity);
    Hierarchy::addChild(m_playerEntity, textEntity);
    Hierarchy::addChild(m_playerEntity, debugInfoEntity);
    Hierarchy::addChild(m_playerEntity, m_cameraEntity);

    // Биндим скрипт к игроку
    m_playerEntity.addComponent<NativeScriptComponent>().bind<PlayerScript>();


    // Музыкальная тыква
    Entity pumpkinEntity = m_scene.createEntity("pumpkin");
    auto &pumpkinRenderer = pumpkinEntity.addComponent<SpriteRendererComponent>(m_baseTexture);
    pumpkinRenderer.textureRect = IntRect(192, 3584, 32, 32);
    pumpkinRenderer.layer = 1;

    auto &pumpkinTransform = pumpkinEntity.getComponent<TransformComponent>();
    pumpkinTransform.position = glm::vec2(384.f, 256.f);
    pumpkinTransform.scale = glm::vec2(2.f, 2.f);
    pumpkinTransform.origin = glm::vec2(16, 16);

    auto &musicComponent = pumpkinEntity.addComponent<AudioSourceComponent>(m_music);
    musicComponent.volume = 1.0f;

    // Настройка текста тыквы
    Entity pumpkinTextEntity = m_scene.createEntity("text");
    auto &pumpkinTextRenderer = pumpkinTextEntity.addComponent<TextRendererComponent>(&m_font);
    pumpkinTextRenderer.horizontalAlign = HorizontalAlign::Center;
    pumpkinTextRenderer.layer = 10;

    auto &pumpkinTextTransform = pumpkinTextEntity.getComponent<TransformComponent>();
    pumpkinTextTransform.scale = glm::vec2(0.5f);

    Hierarchy::addChild(pumpkinEntity, pumpkinTextEntity);

    pumpkinEntity.addComponent<NativeScriptComponent>().bind<PumpkinScript>(m_playerEntity);
}

void Game::update(float deltaTime)
{
    m_scene.update(deltaTime);
}

void Game::destroy()
{
    m_scene.destroy();
    m_font.destroy();
    m_heroTexture.destroy();
    m_baseTexture.destroy();
}
