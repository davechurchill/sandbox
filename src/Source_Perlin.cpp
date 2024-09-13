#include "Source_Perlin.h"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

void Source_Perlin::init()
{
    calculateNoise();
}

void Source_Perlin::calculateNoise()
{
    m_perlin = Perlin2DNew((int)(1 << m_seedSize), (int)(1 << m_seedSize), m_seed);
    m_grid = m_perlin.GeneratePerlinNoise(m_octaves, m_persistance);
    m_topography = cv::Mat(cv::Size((int)m_grid.width(), (int)m_grid.height()), CV_32F, (void *)m_grid.data(), cv::Mat::AUTO_STEP);
    m_image = Tools::matToSfImage(m_topography);
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

    m_texture.loadFromImage(m_image);
    m_sprite.setTexture(m_texture, true);


    window.draw(m_sprite);
}

void Source_Perlin::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    if (event.type == sf::Event::KeyPressed)
    {
        switch (event.key.code)
        {
        case sf::Keyboard::R: { m_seed += 1; calculateNoise(); break; }
        case sf::Keyboard::W: { m_octaves = std::min(m_octaves + 1, m_seedSize); calculateNoise();  break; }
        case sf::Keyboard::S: { m_octaves--; calculateNoise();  break; }
        case sf::Keyboard::A: { m_persistance -= 0.1f; if (m_persistance < 0.1f) { m_persistance = 0.1f; } calculateNoise();  break; }
        case sf::Keyboard::D: { m_persistance += 0.1f; calculateNoise();  break; }
        }
    }
}

void Source_Perlin::save(Save & save) const
{
    save.octaves = m_octaves;
    save.seed = m_seed;
    save.seedSize = m_seedSize;
    save.persistance = m_persistance;
    save.drawGrid = m_drawGrid;
}

void Source_Perlin::load(const Save & save)
{
    m_octaves = save.octaves;
    m_seed = save.seed;
    m_seedSize = save.seedSize;
    m_persistance = save.persistance;
    m_drawGrid = save.drawGrid;
    calculateNoise();
}

cv::Mat Source_Perlin::getTopography()
{
    return m_topography;
}
