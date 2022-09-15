#pragma once

#include <Engine/Common.h>
#include <Engine/Core/Application.h>

class GraphicsApplicationGUI;

// TODO: Keep initialized only the active application
class GraphicsApplication : public Application
{
	friend class GraphicsApplicationGUI;
public:
	GraphicsApplication(const std::vector<Application*>& applications):
		m_Applications(applications)
	{
		ASSERT(!applications.empty(), "[GraphicsApplication] No application implementations!");
	}

	void OnInit(GraphicsContext& context) override 
	{
		OnInit_Internal();
		for (Application* application : m_Applications)
		{
			application->OnInit(context);
		}
	}
	void OnDestroy(GraphicsContext& context) override 
	{
		for (Application* application : m_Applications)
		{
			application->OnDestroy(context);
		}
	}

	Texture* OnDraw(GraphicsContext& context) override 
	{
		return m_Applications[m_ActiveApplication]->OnDraw(context);
	}

	void OnUpdate(GraphicsContext& context, float dt) override 
	{
		m_Applications[m_ActiveApplication]->OnUpdate(context, dt);
	}

	void OnShaderReload(GraphicsContext& context) override 
	{
		for (Application* application : m_Applications)
		{
			application->OnShaderReload(context);
		}
	}

	void OnWindowResize(GraphicsContext& context) override 
	{
		for (Application* application : m_Applications)
		{
			application->OnWindowResize(context);
		}
	}

private:
	void OnInit_Internal();

private:
	uint32_t m_ActiveApplication = 0;
	std::vector<Application*> m_Applications;
};