#include "imgui.h"
#include "imgui-SFML.h"
#include "Processor_Vectors.h"
#include "Profiler.hpp"
#include "Tools.h"

const char* Processor_Vectors::m_shaders[] = { "Popsicle", "Blue", "Red", "Terrain", "Animating Water", "None" };

void Processor_Vectors::init()
{
    m_shader.loadFromFile("shaders/shader_vector_fields.frag", sf::Shader::Fragment);
}

void Processor_Vectors::imgui()
{
    PROFILE_FUNCTION();

    auto selectedAlgorithm = (ParticleManager::Algorithm)m_selectedAlgorithmIndex;
    auto& selectedParameters = m_particleManager.parameters[m_selectedAlgorithmIndex];

    ImGui::Combo("Algorithm", &m_selectedAlgorithmIndex, ParticleManager::AlgorithmNames, (size_t)ParticleManager::Algorithm::Count);


    ImGui::Combo("Color Scheme", &m_selectedShaderIndex, m_shaders, IM_ARRAYSIZE(m_shaders));

    ImGui::Checkbox("##Contours", &m_drawContours);
    ImGui::SameLine();
    ImGui::SliderInt("Contour Lines", &m_numberOfContourLines, 0, 19);

    ImGui::InputInt("Particles", &selectedParameters.particleCount);
    ImGui::SliderInt("Trail Length", &selectedParameters.trailLength, 1, 32);
    ImGui::SliderFloat("Particle Speed", &selectedParameters.particleSpeed, 0.0f, 1000.0f, "%.1f");
    ImGui::SliderFloat("Particle Alpha", &selectedParameters.particleAlpha, 0.f, 1.f);

    if (selectedAlgorithm == ParticleManager::Algorithm::BFS)
    {
        ImGui::SliderInt("Cell Size", &selectedParameters.cellSize, 1, 128);
        ImGui::SliderFloat("Terrain Weight", &selectedParameters.terrainWeight, 0.0f, 1.0f, "%.3f");
    }
    
    if (ImGui::Button("Reset Particles"))
    {
        m_particleManager.reset();
    }

    ImGui::Separator();

    if (ImGui::Button("Reload Shader"))
    {
        m_shader.loadFromFile("shaders/shader_contour_color.frag", sf::Shader::Fragment);
    }
    m_projector.imgui();
}

void Processor_Vectors::render(sf::RenderWindow& window)
{
    PROFILE_FUNCTION();

    auto& selectedParameters = m_particleManager.parameters[m_selectedAlgorithmIndex];

    {
        PROFILE_SCOPE("Draw Transformed Image");

        m_sfTransformedDepthSprite.setPosition(m_projector.getTransformedPosition());
        float scale = m_projector.getTransformedScale();
        m_sfTransformedDepthSprite.setScale(scale, scale);

        //Use static so that it does not get initilialized every time this function is called
        static sf::Clock time;

        //Change color scheme
        m_shader.setUniform("shaderIndex", m_selectedShaderIndex);
        m_shader.setUniform("contour", m_drawContours);
        m_shader.setUniform("numberOfContourLines", m_numberOfContourLines);
        m_shader.setUniform("u_time", time.getElapsedTime().asSeconds());
        m_shader.setUniform("particleAlpha", selectedParameters.particleAlpha);

        window.draw(m_sfTransformedDepthSprite, &m_shader);
    }

    m_projector.render(window);
}

void Processor_Vectors::processEvent(const sf::Event& event, const sf::Vector2f& mouse)
{
    PROFILE_FUNCTION();
    m_projector.processEvent(event, mouse);
}

void Processor_Vectors::save(Save& save) const
{
    save.selectedShaderIndex = m_selectedShaderIndex;
    save.drawContours = m_drawContours;
    save.numberOfContourLines = m_numberOfContourLines;
    m_projector.save(save);
}

void Processor_Vectors::load(const Save& save)
{
    m_selectedShaderIndex = save.selectedShaderIndex;
    m_drawContours = save.drawContours;
    m_numberOfContourLines = save.numberOfContourLines;
    m_projector.load(save);
}

void Processor_Vectors::processTopography(const IntermediateData& data)
{
    PROFILE_FUNCTION();
    const cv::Mat& top = data.topography;

    auto selectedAlgorithm = (ParticleManager::Algorithm)m_selectedAlgorithmIndex;
    auto& selectedParameters = m_particleManager.parameters[m_selectedAlgorithmIndex];

    // Reset particles if data dimensions change
    static int dataSize[2] = { top.rows, top.cols };
    if (dataSize[0] != top.rows || dataSize[1] != top.cols)
    {
        m_particleManager.reset();
        dataSize[0] = top.rows;
        dataSize[1] = top.cols;
    }

    cv::Mat particleGrid = cv::Mat(top.rows, top.cols, CV_8U, 0.0);
    cv::Mat m_cvTransformedParticleGrid32f;

    {
        PROFILE_SCOPE("Update Particles");

        m_particleManager.update(selectedAlgorithm, top, data.deltaTime);

        for (auto& particle : m_particleManager.getParticles())
        {
            for (int i = 0; i < particle.trail.size(); ++i)
            {
                auto& [x, y] = particle.trail[i];
                particleGrid.at<uint8_t>((int)round(y), (int)round(x)) =
                    255 - (selectedParameters.trailLength - i - 1) * 255 / (selectedParameters.trailLength + 1);
            }

            particleGrid.at<uint8_t>((int)round(particle.pos.y), (int)round(particle.pos.x)) = 255;
        }
    }
    
    {
        PROFILE_SCOPE("Calibration TransformProjection");
        m_projector.project(top, m_cvTransformedDepthImage32f);
        m_projector.project(particleGrid, m_cvTransformedParticleGrid32f);
    }

    // Draw warped depth image
    int dw = m_cvTransformedDepthImage32f.cols;
    int dh = m_cvTransformedDepthImage32f.rows;

    // if something went wrong above, quit the function
    if (dw == 0 || dh == 0) { return; }

    {
        PROFILE_SCOPE("Transformed Image SFML Image");

        {
            // Ensure the input image is in the correct format (CV_32F)
            cv::Mat normalized;
            m_cvTransformedDepthImage32f.convertTo(normalized, CV_8U, 255.0); // Scale float [0, 1] to [0, 255]

            // Convert to RGB (SFML requires RGB format)
            cv::Mat rgb;
            cv::cvtColor(normalized, rgb, cv::COLOR_GRAY2RGBA);

            // Pass particle layer as image data
            for (int i = 0; i < rgb.rows; ++i)
            {
                for (int j = 0; j < rgb.cols; ++j)
                {
                    rgb.at<cv::Vec4b>(i, j)[1] = m_cvTransformedParticleGrid32f.at<uint8_t>(i, j);
                }
            }

            // Create SFML image
            sf::Image image;
            image.create(rgb.cols, rgb.rows, rgb.ptr());
            m_sfTransformedDepthImage = image;
        }

        {
            PROFILE_SCOPE("SFML Texture From Image");
            m_sfTransformedDepthTexture.loadFromImage(m_sfTransformedDepthImage);
            m_sfTransformedDepthSprite.setTexture(m_sfTransformedDepthTexture, true);
        }
    }
}
