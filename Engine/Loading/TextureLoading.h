#pragma once

#include "Common.h"

struct Texture;
struct GraphicsContext;

enum class RCF : uint64_t;

namespace TextureLoading
{
	Texture* LoadTextureHDR(const std::string& path, RCF creationFlags);
	Texture* LoadTexture(GraphicsContext& context, const std::string& path, RCF creationFlags, uint32_t numMips = 1);
	Texture* LoadCubemap(const std::string& path, RCF creationFlags);
}