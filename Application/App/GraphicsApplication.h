#pragma once

#include <Engine/Common.h>
#include <Engine/Core/Application.h>

class GraphicsApplication : public Application
{
public:

	void OnInit(GraphicsContext& context) override
	{
		OnInit_Internal(context);
	}

	void OnDestroy(GraphicsContext& context) override 
	{
		OnDestroy_Internal(context);
		m_ActiveSample->OnDestroy(context);
		delete m_ActiveSample;
	}

	Texture* OnDraw(GraphicsContext& context) override 
	{
		return m_ActiveSample->OnDraw(context);
	}

	void OnUpdate(GraphicsContext& context, float dt) override 
	{
		OnUpdate_Internal(context, dt);
		m_ActiveSample->OnUpdate(context, dt);
	}

	void OnShaderReload(GraphicsContext& context) override 
	{
		m_ActiveSample->OnShaderReload(context);
	}

	void OnWindowResize(GraphicsContext& context) override 
	{
		m_ActiveSample->OnWindowResize(context);
	}

	void NextSample();
	void PreviousSample();

	const char* GetActiveSampleName() const;

private:
	void SwitchSample(GraphicsContext& context, uint32_t sampleIndex);

	void OnInit_Internal(GraphicsContext& context);
	void OnUpdate_Internal(GraphicsContext& context, float dt);
	void OnDestroy_Internal(GraphicsContext& context);

private:
	uint32_t m_PendingSampleIndex = 0;
	uint32_t m_ActiveSampleIndex = 0;

	Application* m_ActiveSample = 0;
};