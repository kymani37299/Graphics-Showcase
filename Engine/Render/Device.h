#pragma once

#include <queue>

#include "Common.h"
#include "Render/RenderAPI.h"

class RenderTask;
class DescriptorHeap;
struct GraphicsContext;
struct Texture;
struct Shader;
struct Buffer;

struct DeviceMemory
{
	ScopedRef<DescriptorHeap> SRVHeap;
	ScopedRef<DescriptorHeap> RTVHeap;
	ScopedRef<DescriptorHeap> DSVHeap;
	ScopedRef<DescriptorHeap> SMPHeap;
};

class DeferredTaskExecutor
{
public:
	DeferredTaskExecutor(uint32_t maxTasksPerFrame) :
		m_MaxTasksPerFrame(maxTasksPerFrame) {}

	~DeferredTaskExecutor();

	void Submit(RenderTask* task) { m_Tasks.push(task); }
	void Run(GraphicsContext& context);

private:
	uint32_t m_MaxTasksPerFrame;
	std::queue<RenderTask*> m_Tasks;
};

class Device
{
private:
	static Device* s_Instance;

public:
	static constexpr uint8_t SWAPCHAIN_BUFFER_COUNT = 2;
	static constexpr DXGI_FORMAT SWAPCHAIN_DEFAULT_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr uint8_t IN_FLIGHT_FRAME_COUNT = 3;

	static void Init() { s_Instance = new Device(); s_Instance->InitDevice(); }
	static Device* Get() { return s_Instance; }
	static void Destroy() { if (s_Instance) { s_Instance->DeinitDevice(); delete s_Instance; } }

private:
	Device();
	~Device();
	void InitDevice();
	void DeinitDevice();

public:
	void RecreateSwapchain();
	void BindSwapchainToRenderTarget(GraphicsContext& context);
	void CopyToSwapchain(Texture* texture);
	void EndFrame();

	ID3D12Device* GetHandle() const { return m_Handle.Get(); }
	D3D12MA::Allocator* GetAllocator() const { return m_Allocator.Get(); }
	GraphicsContext& GetContext() { return *m_Context; }
	DeviceMemory& GetMemory() { return m_Memory; }
	DeferredTaskExecutor& GetTaskExecutor() { return m_TaskExecutor; }

	bool IsMainContext(GraphicsContext& context) { return true; } // TODO

private:
	ComPtr<IDXGIFactory4> m_DXGIFactory;
	ComPtr<ID3D12Device> m_Handle;
	ComPtr<D3D12MA::Allocator> m_Allocator;

	ComPtr<IDXGISwapChain> m_SwapchainHandle;
	ScopedRef<Texture> m_SwapchainBuffers[SWAPCHAIN_BUFFER_COUNT];
	uint8_t m_CurrentSwapchainBuffer = 0;

	DeviceMemory m_Memory;
	ScopedRef<GraphicsContext> m_Context;

	static constexpr uint32_t MAX_DEFERRED_TASKS = 32;
	DeferredTaskExecutor m_TaskExecutor{ MAX_DEFERRED_TASKS };
};