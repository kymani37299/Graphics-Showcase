#pragma once

#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMesh/DirectXMesh.h>

#include "Common.h"
#include "Render/D3D12MemAlloc.h"

#ifdef DEBUG
#define API_CALL(X) {HRESULT hr = X; ASSERT(SUCCEEDED(hr), "DX ERROR " __FILE__);}
#else
#define API_CALL(X) X
#endif // DEBUG

using Microsoft::WRL::ComPtr;

#define USE_PIX
