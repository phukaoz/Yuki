#pragma once

#include "Sora.h"

class Sandbox2D : public Sora::Layer
{
public:
	Sandbox2D();
	virtual ~Sandbox2D() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(Sora::Timestep ts) override;
	virtual void OnImGuiRender() override;
	virtual void OnEvent(Sora::Event& e) override;
private:
};