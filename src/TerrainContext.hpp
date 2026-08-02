#pragma once

class SandBoxProjector;

class TerrainContext
{
    SandBoxProjector & m_projector;

public:
    explicit TerrainContext(SandBoxProjector & projector)
        : m_projector(projector)
    {
    }

    SandBoxProjector & projector() const { return m_projector; }
};
