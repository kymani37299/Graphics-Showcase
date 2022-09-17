#include "GraphicsApplication.h"

#include <Engine/Render/Commands.h>
#include <Engine/Render/Context.h>
#include <Engine/Render/Shader.h>
#include <Engine/System/Window.h>
#include <Engine/System/Input.h>

#include "App/GraphicsApplicationGUI.h"
#include "Clouds/CloudsApp.h"

#define MAX(A,B) ((A>B) ? (A) : (B))
#define ADD_SAMPLE(Index, Class, Name) m_NumSamples = MAX(m_NumSamples, Index+1); if(m_SampleNames.size() < Index+1) m_SampleNames.resize(Index+1); m_SampleNames[Index] = Name;
void GraphicsApplication::RegisterSamples()
{
#include "App/SampleList.h"
}
#undef ADD_SAMPLE
#undef MAX

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

	if (m_PendingSampleIndex != m_ActiveSampleIndex)
	{
		GFX::Cmd::FlushContext(context);
		GFX::Cmd::ResetContext(context);
		
		m_ActiveSample->OnDestroy(context);
		delete m_ActiveSample;

		m_ActiveSampleIndex = m_PendingSampleIndex;
		SwitchSample(m_ActiveSampleIndex);
		m_ActiveSample->OnInit(context);
	}
}
