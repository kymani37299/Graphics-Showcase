#include "GraphicsApplication.h"

#include <Engine/Render/Commands.h>
#include <Engine/Render/Context.h>
#include <Engine/Render/Shader.h>
#include <Engine/System/Window.h>
#include <Engine/System/Input.h>

#include "App/GraphicsApplicationGUI.h"
#include "Common/DebugRender.h"
#include "SampleList.h"

void GraphicsApplication::NextSample()
{
	const uint32_t sampleCount = static_cast<uint32_t>(s_SampleList.size());
	m_PendingSampleIndex = (m_ActiveSampleIndex + 1) % sampleCount;
}

void GraphicsApplication::PreviousSample()
{
	const uint32_t sampleCount = static_cast<uint32_t>(s_SampleList.size());
	m_PendingSampleIndex = m_ActiveSampleIndex == 0 ? sampleCount - 1 : m_ActiveSampleIndex - 1;
}

const char* GraphicsApplication::GetActiveSampleName() const
{
	return s_SampleList[m_ActiveSampleIndex].Name;
}

void GraphicsApplication::SwitchSample(GraphicsContext& context, uint32_t sampleIndex)
{
	const uint32_t sampleCount = static_cast<uint32_t>(s_SampleList.size());
	if (sampleIndex >= sampleCount)
	{
		ASSERT(0, "[GraphicsApplication] Trying to switch to nonexistent sample!");
		return;
	}

	if (m_ActiveSample != nullptr)
	{
		m_ActiveSample->OnDestroy(context);
		delete m_ActiveSample;
	}
	
	m_ActiveSampleIndex = sampleIndex;
	m_ActiveSample = s_SampleList[m_ActiveSampleIndex].CreateFunction();
	m_ActiveSample->OnInit(context);
}

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

	SwitchSample(context, m_ActiveSampleIndex);
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
		SwitchSample(context, m_PendingSampleIndex);
	}
}

void GraphicsApplication::OnDestroy_Internal(GraphicsContext& context)
{
	DebugRender::Deinit();
}