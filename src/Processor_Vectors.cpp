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

SandBoxProjector & Processor_Vectors::activeProjector()
{
    return m_overlayProcessor ? m_overlayProcessor->projector() : m_projector;
}

void Processor_Vectors::imgui()
{
    imguiControls(false);
}

void Processor_Vectors::imguiControls(bool overlayOnly)
{
    PROFILE_FUNCTION();

    ImGui::TextUnformatted("Mode: Steering");
    auto& selectedParameters = m_particleManager.parameters[
        (size_t)ParticleManager::Algorithm::Steering];

    if (!overlayOnly)
    {
        ImGui::Combo("Color Scheme", &m_selectedShaderIndex, m_shaders, IM_ARRAYSIZE(m_shaders));
        ImGui::Checkbox("##Contours", &m_drawContours);
        ImGui::SameLine();
        ImGui::SliderInt("Contour Lines", &m_numberOfContourLines, 0, 19);
    }

    ImGui::SliderFloat("Spawn Rate", &selectedParameters.spawnRate, 0.0f, 20000.0f, "%.0f / sec");
    ImGui::Text("Active Particles: %zu", m_particleManager.getParticleCount());
    ImGui::SliderInt("Trail Length", &selectedParameters.trailLength, 1, 32);
    ImGui::SliderFloat("Base Particle Speed", &selectedParameters.particleSpeed, 0.0f, 1000.0f, "%.1f");
    ImGui::SliderFloat("Particle Alpha", &selectedParameters.particleAlpha, 0.f, 1.f);

    ImGui::SliderFloat("Steering Distance", &selectedParameters.steeringDistance, 4.0f, 128.0f, "%.0f px");
    bool heightRangeChanged = ImGui::SliderFloat(
        "Minimum Wind Height",
        &selectedParameters.minimumWindHeight,
        0.0f,
        1.0f);
    heightRangeChanged |= ImGui::SliderFloat(
        "Maximum Wind Height",
        &selectedParameters.maximumWindHeight,
        0.0f,
        1.0f);
    selectedParameters.maximumWindHeight = std::max(
        selectedParameters.maximumWindHeight,
        selectedParameters.minimumWindHeight);
    if (heightRangeChanged)
    {
        m_particleManager.reset();
    }
    
    if (ImGui::Button("Reset Particles"))
    {
        m_particleManager.reset();
    }

    ImGui::Separator();

    if (ImGui::Button("Reload Shader"))
    {
        m_shader.loadFromFile("shaders/shader_vector_fields.frag", sf::Shader::Fragment);
    }
    if (!overlayOnly)
    {
        m_projector.imgui();
    }
}

void Processor_Vectors::render(sf::RenderWindow& window)
{
    PROFILE_FUNCTION();
    renderVectors(window, false);
}

void Processor_Vectors::renderVectors(sf::RenderWindow & window, bool overlayOnly)
{
    auto& selectedParameters = m_particleManager.parameters[
        (size_t)ParticleManager::Algorithm::Steering];

    {
        PROFILE_SCOPE("Draw Transformed Image");

        SandBoxProjector & projector = activeProjector();
        m_sfTransformedDepthSprite.setPosition(projector.getTransformedPosition());
        float scale = projector.getTransformedScale();
        m_sfTransformedDepthSprite.setScale(scale, scale);

        //Use static so that it does not get initilialized every time this function is called
        static sf::Clock time;

        //Change color scheme
        m_shader.setUniform("shaderIndex", m_selectedShaderIndex);
        m_shader.setUniform("contour", m_drawContours);
        m_shader.setUniform("numberOfContourLines", m_numberOfContourLines);
        m_shader.setUniform("u_time", time.getElapsedTime().asSeconds());
        m_shader.setUniform("particleAlpha", selectedParameters.particleAlpha);
        m_shader.setUniform("overlayOnly", overlayOnly);
        m_shader.setUniform("reverseDepthAlpha", true);

        window.draw(m_sfTransformedDepthSprite, &m_shader);
    }

}

void Processor_Vectors::processEvent(const sf::Event& event, const sf::Vector2f& mouse)
{
    PROFILE_FUNCTION();
    activeProjector().processEvent(event, mouse);
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

    auto& selectedParameters = m_particleManager.parameters[
        (size_t)ParticleManager::Algorithm::Steering];

    // Reset particles if data dimensions change
    static int dataSize[2] = { top.rows, top.cols };
    if (dataSize[0] != top.rows || dataSize[1] != top.cols)
    {
        m_particleManager.reset();
        dataSize[0] = top.rows;
        dataSize[1] = top.cols;
    }

    cv::Mat particleGrid = cv::Mat(top.rows, top.cols, CV_8U, 0.0);
    cv::Mat particleHeightGrid = cv::Mat(top.rows, top.cols, CV_8U, 0.0);
    cv::Mat m_cvTransformedParticleGrid32f;
    cv::Mat transformedParticleHeightGrid;

    {
        PROFILE_SCOPE("Update Particles");

        m_particleManager.update(top, data.deltaTime);

        const auto drawParticle = [&]
        (
            int centerX,
            int centerY,
            int intensity,
            double height,
            double halfLength,
            double halfWidth,
            const sf::Vector2<double> & direction
        )
        {
            const uint8_t heightValue = (uint8_t)std::round(
                std::clamp(height, 0.0, 1.0) * 255.0);
            const double directionLength = std::sqrt(
                direction.x * direction.x + direction.y * direction.y);
            const double directionX = directionLength > 0.000001
                ? direction.x / directionLength
                : 1.0;
            const double directionY = directionLength > 0.000001
                ? direction.y / directionLength
                : 0.0;
            const int extent = (int)std::ceil(halfLength + halfWidth);
            for (int offsetY = -extent; offsetY <= extent; offsetY++)
            {
                for (int offsetX = -extent; offsetX <= extent; offsetX++)
                {
                    const double along = offsetX * directionX + offsetY * directionY;
                    const double across = -offsetX * directionY + offsetY * directionX;
                    if (std::abs(along) > halfLength || std::abs(across) > halfWidth)
                    {
                        continue;
                    }
                    const int x = centerX + offsetX;
                    const int y = centerY + offsetY;
                    if (x < 0 || y < 0 || x >= particleGrid.cols || y >= particleGrid.rows)
                    {
                        continue;
                    }

                    const uint8_t value = (uint8_t)std::clamp(
                        intensity,
                        0,
                        255);
                    uint8_t & existingValue = particleGrid.at<uint8_t>(y, x);
                    uint8_t & existingHeight = particleHeightGrid.at<uint8_t>(y, x);
                    if (value > existingValue
                        || (value == existingValue && heightValue > existingHeight))
                    {
                        existingValue = value;
                        existingHeight = heightValue;
                    }
                }
            }
        };

        for (auto& particle : m_particleManager.getParticles())
        {
            const double particleHeight = particle.height;
            for (int i = 0; i < particle.trail.size(); ++i)
            {
                auto& [x, y] = particle.trail[i];
                drawParticle(
                    (int)std::round(x),
                    (int)std::round(y),
                    255 - (selectedParameters.trailLength - i - 1) * 255
                        / (selectedParameters.trailLength + 1),
                    particleHeight,
                    0.45,
                    0.45,
                    particle.direction);
            }

            drawParticle(
                (int)std::round(particle.pos.x),
                (int)std::round(particle.pos.y),
                255,
                particleHeight,
                1.5 + particleHeight * 1.5,
                0.55 + particleHeight * 0.50,
                particle.direction);
        }
    }
    
    {
        PROFILE_SCOPE("Calibration TransformProjection");
        SandBoxProjector & projector = activeProjector();
        projector.project(top, m_cvTransformedDepthImage32f);
        projector.project(particleGrid, m_cvTransformedParticleGrid32f);
        projector.project(particleHeightGrid, transformedParticleHeightGrid);
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
                    rgb.at<cv::Vec4b>(i, j)[2] = transformedParticleHeightGrid.at<uint8_t>(i, j);
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

void Processor_Vectors::initOverlay()
{
    m_particleManager.reset();
    m_shader.loadFromFile("shaders/shader_vector_fields.frag", sf::Shader::Fragment);
}

void Processor_Vectors::imguiOverlay()
{
    imguiControls(true);
}

void Processor_Vectors::processTopographyOverlay(
    const IntermediateData & data,
    TopographyProcessor & processor)
{
    m_overlayProcessor = &processor;
    processTopography(data);
}

void Processor_Vectors::renderOverlay(
    sf::RenderWindow & window,
    TopographyProcessor & processor)
{
    m_overlayProcessor = &processor;
    renderVectors(window, true);
}

void Processor_Vectors::processOverlayEvent(
    const sf::Event &,
    const sf::Vector2f &,
    TopographyProcessor &)
{
}

void Processor_Vectors::saveOverlay(Save &) const
{
}

void Processor_Vectors::loadOverlay(const Save &)
{
}
