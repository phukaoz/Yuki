#include "ParticleSystem.h"

#include "Random.h"

#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

ParticleSystem::ParticleSystem(uint32_t maxParticle)
	: m_PoolIndex(maxParticle - 1)
{
	m_ParticlePool.resize(maxParticle);
}

void ParticleSystem::OnUpdate(Yuki::Timestep ts)
{
	for (auto& particle : m_ParticlePool)
	{
		if (!particle.Active)
			continue;

		if (particle.LifeRemaining <= 0.0f)
		{
			particle.Active = false;
			continue;
		}

		particle.LifeRemaining -= ts;
		particle.Position += particle.Velocity * (float)ts;
		particle.Rotation += 0.01f * ts;
	}
}

void ParticleSystem::OnRender(Yuki::OrthographicCamera& camera)
{
	Yuki::Renderer2D::BeginScene(camera);
	for (auto& particle : m_ParticlePool)
	{
		if (!particle.Active)
			continue;

		float life = particle.LifeRemaining / particle.LifeTime;
		glm::vec4 color = glm::lerp(particle.ColorEnd, particle.ColorBegin, life);

		float size = glm::lerp(particle.SizeEnd, particle.SizeBegin, life);
		Yuki::Renderer2D::DrawQuad(particle.Position, { size, size }, color, particle.Rotation);
	}
	Yuki::Renderer2D::EndScene();
}

void ParticleSystem::Emit(const ParticleProps& particleProps)
{
	Particle& particle = m_ParticlePool[m_PoolIndex];
	particle.Active = true;
	particle.Position = particleProps.Position;
	particle.Rotation = Random::Float() * 2.0f * glm::pi<float>();

	particle.Velocity = particleProps.Velocity;
	particle.Velocity.x += particleProps.VelocityVariation.x * (Random::Float() - 0.5f);
	particle.Velocity.y += particleProps.VelocityVariation.y * (Random::Float() - 0.5f);

	particle.ColorBegin = particleProps.ColorBegin;
	particle.ColorEnd = particleProps.ColorEnd;

	particle.LifeTime = particleProps.LifeTime;
	particle.LifeRemaining = particleProps.LifeTime;
	particle.SizeBegin = particleProps.SizeBegin + particleProps.SizeVariation * (Random::Float() - 0.5f);
	particle.SizeEnd = particleProps.SizeEnd;

	m_PoolIndex = --m_PoolIndex % m_ParticlePool.size();
}