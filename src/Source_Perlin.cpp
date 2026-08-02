#include "Source_Perlin.h"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include <algorithm>
#include <iostream>

void Source_Perlin::init()
{
    calculateNoise();
}

void Source_Perlin::calculateNoise()
{
    constexpr int minSeedSize = 1;
    constexpr int maxSeedSize = 10;
    m_seedSize = std::clamp(m_seedSize, minSeedSize, maxSeedSize);
    m_octaves = std::clamp(m_octaves, 1, m_seedSize);
    m_persistance = std::max(m_persistance, 0.01f);

    m_perlin = Perlin2DNew((int)(1 << m_seedSize), (int)(1 << m_seedSize), m_seed);
    m_grid = m_perlin.GeneratePerlinNoise(m_octaves, m_persistance);
    m_topography = cv::Mat(cv::Size((int)m_grid.width(), (int)m_grid.height()), CV_32F, (void *)m_grid.data(), cv::Mat::AUTO_STEP);
    m_image = Tools::matToSfImage(m_topography);
    markTerrainChanged();
}


void Source_Perlin::imgui()
{
    if (ImGui::InputInt("Seed", &m_seed, 1, 1000))
    {
        calculateNoise();
    }

    if (ImGui::InputInt("SeedSize", &m_seedSize, 1, 100))
    {
        calculateNoise();
    }

    if (ImGui::InputInt("Octaves", &m_octaves, 1, 20))
    {
        calculateNoise();
    }

    if (ImGui::SliderFloat("Persistance", &m_persistance, 0, 2))
    {
        calculateNoise();
    }

    ImGui::Checkbox("Draw Grid", &m_drawGrid);
}



void Source_Perlin::render(sf::RenderWindow & window)
{
    const sf::Color gridColor(64, 64, 64);

    if (!m_texture.loadFromImage(m_image))
    {
        std::cerr << "Failed to load the Perlin terrain texture.\n";
        return;
    }
    m_sprite.setTexture(m_texture, true);


    window.draw(m_sprite);
}

void Source_Perlin::processEvent(const sf::Event & event, const sf::Vector2f &)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        switch (keyPressed->code)
        {
        case sf::Keyboard::Key::R: { m_seed += 1; calculateNoise(); break; }
        case sf::Keyboard::Key::W: { m_octaves = std::min(m_octaves + 1, m_seedSize); calculateNoise();  break; }
        case sf::Keyboard::Key::S: { m_octaves--; calculateNoise();  break; }
        case sf::Keyboard::Key::A: { m_persistance -= 0.1f; if (m_persistance < 0.1f) { m_persistance = 0.1f; } calculateNoise();  break; }
        case sf::Keyboard::Key::D: { m_persistance += 0.1f; calculateNoise();  break; }
        }
    }
}

void Source_Perlin::save(Settings & save) const
{
    Settings::json & settings = save.section("Source_Perlin");
    settings["m_octaves"] = m_octaves;
    settings["m_seed"] = m_seed;
    settings["m_seedSize"] = m_seedSize;
    settings["m_persistance"] = m_persistance;
    settings["m_drawGrid"] = m_drawGrid;
}

void Source_Perlin::load(const Settings & save)
{
    const Settings::json & settings = save.section("Source_Perlin");
    Settings::read(settings, "m_octaves", m_octaves);
    Settings::read(settings, "m_seed", m_seed);
    Settings::read(settings, "m_seedSize", m_seedSize);
    Settings::read(settings, "m_persistance", m_persistance);
    Settings::read(settings, "m_drawGrid", m_drawGrid);
    calculateNoise();
}

cv::Mat Source_Perlin::getTopography()
{
    return m_topography;
}
