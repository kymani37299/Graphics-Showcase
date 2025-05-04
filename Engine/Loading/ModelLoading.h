#pragma once

#include <unordered_map>
#include <vector>

#include "Common.h"

struct Buffer;
struct Texture;
struct GraphicsContext;

struct cgltf_data;
struct cgltf_node;
struct cgltf_primitive;
struct cgltf_material;
struct cgltf_texture;
struct cgltf_animation_channel;
struct cgltf_skin;
struct cgltf_scene;
struct cgltf_light;
struct cgltf_mesh;
struct cgltf_camera;

#undef OPAQUE

namespace ModelLoading
{
	enum class AnimTarget
	{
		Invalid,
		Translation,
		Rotation,
		Scale,
		Weights
	};

	enum class AnimInterpolation
	{
		Invalid,
		Step,
		Lerp,
		Cubic,
	};

	struct AnimKeyFrame
	{
		float Time;
		union
		{
			Quaternion Rotation = Quaternion{0.0f, 0.0f, 0.0f, 0.0f};
			Float3 Translation;
			Float3 Scale;
			float Weight;
		};

	};

	struct AnimationEntry
	{
		uint32_t WeightTargetIndex = 0;
		AnimTarget Target = AnimTarget::Invalid;
		AnimInterpolation Interpolation = AnimInterpolation::Invalid;
		std::vector<AnimKeyFrame> KeyFrames = {};
		float Duration = 1.0f;
	};

	struct MorphVertex
	{
		Float3 Position;
		Float2 Texcoord;
		Float3 Normal;
		Float4 Tangent;
	};

	struct MorphTarget
	{
		float Weight;
		Buffer* Data; // MorphVertex
	};

	struct SkeletonJoint
	{
		DirectX::XMFLOAT4X4 Transform;
		DirectX::XMFLOAT4X4 ModelToJoint;
		std::vector<AnimationEntry> Animations;
	};

	struct BoundingSphere
	{
		Float3 Center{ 0.0f, 0.0f, 0.0f };
		float Radius = 1.0f;
	};

	struct MeshData
	{
		uint32_t PrimitiveCount = 0;

		std::vector<Float3> PositionsData;
		std::vector<uint32_t> IndicesData;

		Buffer* Positions = nullptr;	// float3
		Buffer* Texcoords = nullptr;	// float2
		Buffer* Normals = nullptr;		// float3 
		Buffer* Tangents = nullptr;		// float4
		Buffer* Weights = nullptr;		// float4
		Buffer* Joints = nullptr;		// uint4
		Buffer* Indices = nullptr;		// uint32_t
	};

	struct MaterialData
	{
		enum class MaterialType
		{
			OPAQUE,
			ALPHA_DISCARD,
			ALPHA_BLEND
		};

		MaterialType MatType = MaterialType::OPAQUE;

		Float3 AlbedoFactor{ 1.0f, 1.0f, 1.0f };
		float MetallicFactor = 1.0f;
		float RoughnessFactor = 1.0f;

		Texture* Albedo = nullptr;
		Texture* Normal = nullptr;
		Texture* MetallicRoughness = nullptr;
	};

	struct SceneObject
	{
		BoundingSphere BoundingVolume;
		DirectX::XMFLOAT4X4 ModelToWorld;

		MeshData Mesh;
		MaterialData Material;

		std::vector<AnimationEntry> AnimcationData;
		std::vector<MorphTarget> MorphTargets;
		std::vector<SkeletonJoint> Skeleton;

	};

	struct SceneCamera
	{
		Float3 Position{};
		Quaternion Rotation{};
		bool IsOrtho = false;

		float ZFar;
		float ZNear;

		float FOV;
	};

	struct SceneLight
	{
		enum class LightType
		{
			Directional,
			Point,
			Spot
		};

		Float3 Position{};
		Float3 Direction{};
		Float3 Color{};
		float Strength = 0.0f;
		LightType Type = LightType::Point;
		float Range = 0.0f;
		float InnerConeAngle = 0.0f;
		float OuterConeAngle = 0.0f;
	};

	struct Scene
	{
		std::vector<SceneCamera> Cameras;
		std::vector<SceneObject> Objects;
		std::vector<SceneLight> Lights;
	};

	class Loader
	{
	public:
		Loader(GraphicsContext& context): 
			m_Context(context)
		{}

		Scene Load(const std::string& path);

	private:
		void FillNodeAnimationMap(cgltf_data* sceneData);

		Scene LoadScene(cgltf_scene* scene);
		void LoadNode(cgltf_node* nodeData);
		SceneCamera LoadCamera(cgltf_camera* cameraNode);
		SceneLight LoadLight(cgltf_light* lightNode);

		std::vector<AnimationEntry> LoadAnimations(cgltf_node* nodeData);
		std::vector<MorphTarget> LoadMorph(cgltf_primitive* meshData, const std::vector<float>& weights);
		std::vector<SkeletonJoint> LoadSkin(cgltf_skin* skinData);
		MeshData LoadMesh(cgltf_primitive* meshData);
		MaterialData LoadMaterial(cgltf_material* materialData);
		Texture* LoadTexture(cgltf_texture* textureData, ColorUNORM defaultColor = {0.0f, 0.0f, 0.0f, 0.0f});

	private:
		// Configurations
		GraphicsContext& m_Context;
		uint32_t m_TextureNumMips = 1;

		// Intermediate variables
		Scene m_Scene;
		DirectX::XMFLOAT4X4 m_CurrentTransform;
		std::string m_DirectoryPath;
		std::unordered_map<cgltf_node*, std::vector<cgltf_animation_channel*>> m_NodeAnimationMap;
	};

	void Free(GraphicsContext& context, SceneObject& sceneObject);
	void Free(Scene& scene);
}