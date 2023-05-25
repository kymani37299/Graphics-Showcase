#include "GraphicsApplication.h"

#include <Engine/Render/Commands.h>
#include <Engine/Render/Context.h>
#include <Engine/Render/Shader.h>
#include <Engine/System/Window.h>
#include <Engine/System/Input.h>

#include "App/GraphicsApplicationGUI.h"
#include "Common/DebugRender.h"
#include "Animation/AnimationApp.h"
#include "Clouds/CloudsApp.h"
#include "Grass/GrassApp.h"

#define ADD_SAMPLE(Index, Class, Name) m_NumSamples = max(m_NumSamples, Index+1); if(m_SampleNames.size() < Index+1) m_SampleNames.resize(Index+1); m_SampleNames[Index] = Name;
void GraphicsApplication::RegisterSamples()
{
#include "App/SampleList.h"
}
#undef ADD_SAMPLE

#define ADD_SAMPLE(Index, Class, Name) case Index: m_ActiveSample = new Class{}; break;
void GraphicsApplication::SwitchSample(uint32_t sampleIndex)
{
	switch (sampleIndex)
	{
#include "App/SampleList.h"
	default:
		ASSERT(0, "[GraphicsApplication] Trying to switch to nonexistent sample!");
	}
}
#undef ADD_SAMPLE

void GraphicsApplication::OnInit_Internal(GraphicsContext& context)
{
	Window::Get()->ShowCursor(true);
	DebugRender::Init(context);

	GUI* gui = GUI::Get();
	gui->PushMenu("General");
	gui->AddElement(new RenderStatsGUI());
	gui->AddElement(new GraphicsApplicationGUI(this));
	gui->AddElement(new ControlsGUI());
	gui->PopMenu();

	RegisterSamples();
	SwitchSample(m_ActiveSampleIndex);
	m_ActiveSample->OnInit(context);
}

void GraphicsApplication::OnUpdate_Internal(GraphicsContext& context, float dt)
{
	if (Input::IsKeyJustPressed('F'))
	{
		static bool showCursorToggle = true;
		showCursorToggle = !showCursorToggle;
		Window::Get()->ShowCursor(showCursorToggle);
	}

	if (Input::IsKeyJustPressed('R'))
	{
		GFX::ReloadAllShaders();
	}

	if (Input::IsKeyJustPressed('G'))
	{
		GUI::Get()->ToggleVisible();
	}

	if (Input::IsKeyJustPressed(VK_ESCAPE))
	{
		Window::Get()->Shutdown();
	}

	if (Input::IsKeyJustPressed('Z'))
	{
		PreviousSample();
	}

	if (Input::IsKeyJustPressed('X'))
	{
		NextSample();
	}

	if (m_PendingSampleIndex != m_ActiveSampleIndex)
	{
		ContextManager::Get().Flush();

		m_ActiveSample->OnDestroy(context);
		delete m_ActiveSample;

		m_ActiveSampleIndex = m_PendingSampleIndex;
		SwitchSample(m_ActiveSampleIndex);
		m_ActiveSample->OnInit(context);
	}
}

void GraphicsApplication::OnDestroy_Internal(GraphicsContext& context)
{
	DebugRender::Deinit();
}
