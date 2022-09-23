#include "ConstantBuffer.h"

#include <Engine/Render/Commands.h>
#include <Engine/Render/Buffer.h>
#include <Engine/Render/Memory.h>

Buffer* ConstantBuffer::GetBuffer()
{
	uint32_t bufferSize = (m_DataSize + 255) & ~255;
	Buffer* buffer = GFX::CreateBuffer(bufferSize, bufferSize, RCF_Bind_CBV | RCF_CPU_Access);
	DeferredTrash::Put(buffer);
	GFX::SetDebugName(buffer, "ConstantManager::Buffer");
	GFX::Cmd::UploadToBufferCPU(buffer, 0, m_Data.data(), 0, m_DataSize);
	return buffer;
}

void ConstantBuffer::AddInternal(const uint8_t* data, uint32_t stride)
{
	if (m_DataSize + stride > m_Data.size()) m_Data.resize(m_DataSize + stride);
	uint8_t* dst = &m_Data[m_DataSize];
	memcpy(dst, data, stride);
	m_DataSize += stride;
}