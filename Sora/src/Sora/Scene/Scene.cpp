#include "sorapch.h"
#include "Scene.h"

#include <glm/glm.hpp>

#include "Sora/Scene/Entity.h"
#include "Sora/Scene/Component.h"
#include "Sora/Scene/ScriptableEntity.h"
#include "Sora/Renderer/Renderer2D.h"

namespace Sora {

	namespace Utils{
		
		static b2BodyType TypeConvert(Rigidbody2DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
			case Rigidbody2DComponent::BodyType::Static:	return b2_staticBody;
			case Rigidbody2DComponent::BodyType::Dynamic:	return b2_dynamicBody;
			case Rigidbody2DComponent::BodyType::Kinematic:	return b2_kinematicBody;
			}

			SORA_CORE_ASSERT(false, "Unknown body type!");
			return b2_staticBody;
		}

		template<typename Component>
		static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
		{
			auto view = src.view<Component>();
			for (auto e : view)
			{
				UUID uuid = src.get<IDComponent>(e).ID;
				entt::entity dstHandle = enttMap.at(uuid);

				auto& srcComponent = src.get<Component>(e);
				dst.emplace_or_replace<Component>(dstHandle, srcComponent);
			}
		}

		template<typename Component>
		static void CopyComponentIfExist(Entity dst, Entity src)
		{
			if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
		}
	}

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = CreateRef<Scene>();

		newScene->mViewportWidth = other->mViewportWidth;
		newScene->mViewportHeight = other->mViewportHeight;
		
		std::unordered_map<UUID, entt::entity> enttMap;	
		auto& srcRegistry = other->mRegistry;
		auto& dstRegistry = newScene->mRegistry;
		auto view = srcRegistry.view<IDComponent>();
		for (auto e : view)
		{
			UUID uuid = srcRegistry.get<IDComponent>(e).ID;
			const std::string& name = srcRegistry.get<TagComponent>(e).Tag;
			Entity entity = newScene->CreateEntity(name, uuid);
			enttMap[uuid] = entity;
		}
		Utils::CopyComponent<TransformComponent>(dstRegistry, srcRegistry, enttMap);
		Utils::CopyComponent<SpriteComponent>(dstRegistry, srcRegistry, enttMap);
		Utils::CopyComponent<CameraComponent>(dstRegistry, srcRegistry, enttMap);
		Utils::CopyComponent<NativeScriptComponent>(dstRegistry, srcRegistry, enttMap);
		Utils::CopyComponent<Rigidbody2DComponent>(dstRegistry, srcRegistry, enttMap);
		Utils::CopyComponent<BoxCollider2DComponent>(dstRegistry, srcRegistry, enttMap);

		//newScene->mWorldID = other->mWorldID;
		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name, const std::optional<UUID>& uuid)
	{
		Entity entity = { mRegistry.create(), this };
		entity.AddComponent<IDComponent>(uuid.value_or(UUID()));
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Untitled Entity" : name;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		mRegistry.destroy(entity);
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		std::string name = entity.GetName();
		Entity newEntity = CreateEntity(name);

		Utils::CopyComponentIfExist<TransformComponent>(newEntity, entity);
		Utils::CopyComponentIfExist<SpriteComponent>(newEntity, entity);
		Utils::CopyComponentIfExist<CameraComponent>(newEntity, entity);
		Utils::CopyComponentIfExist<NativeScriptComponent>(newEntity, entity);
		Utils::CopyComponentIfExist<Rigidbody2DComponent>(newEntity, entity);
		Utils::CopyComponentIfExist<BoxCollider2DComponent>(newEntity, entity);

		return newEntity;
	}

	void Scene::OnRuntimeStart()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { 0.0f, -10.0f };
		mWorldID = b2CreateWorld(&worldDef);

		auto viewRigidbody2D = mRegistry.view<Rigidbody2DComponent>();
		for (auto e : viewRigidbody2D)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			b2BodyDef bodyDef = b2DefaultBodyDef();
			bodyDef.type = Utils::TypeConvert(rb2d.Type);
			bodyDef.position.x = transform.Translation.x;
			bodyDef.position.y = transform.Translation.y;
			bodyDef.rotation = b2MakeRot(transform.Rotation.z);
			bodyDef.fixedRotation = rb2d.FixedRotation;
			
			b2BodyId bodyID = b2CreateBody(mWorldID, &bodyDef);
			// TODO: find the better way to store body id.
			memcpy(&rb2d.RuntimeBody, &bodyID, sizeof(b2BodyId));
		
			if (entity.HasComponent<BoxCollider2DComponent>())
			{
				auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

				b2ShapeDef shapeDef = b2DefaultShapeDef();
				shapeDef.density = bc2d.Density;
				shapeDef.friction = bc2d.Friction;
				shapeDef.restitution = bc2d.Restitution;

				b2Polygon polygon = b2MakeBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y);
				b2ShapeId shapeID = b2CreatePolygonShape(bodyID, &shapeDef, &polygon);
				// TODO: find the better way to store shape id.
				memcpy(&bc2d.RuntimeFixture, &shapeID, sizeof(b2ShapeId));
			}
		}
	}

	void Scene::OnRuntimeStop()
	{
		b2DestroyWorld(mWorldID);
	}

	void Scene::OnUpdatePhysics(Timestep ts)
	{
		const int32_t subStepCount = 4;
		b2World_Step(mWorldID, ts, subStepCount);

		auto viewRigidbody2D = mRegistry.view<Rigidbody2DComponent>();
		for (auto e : viewRigidbody2D)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			// TODO: find the better way to get body.
			b2BodyId bodyID;
			memcpy(&bodyID, &rb2d.RuntimeBody, sizeof(b2BodyId));

			b2Vec2 position = b2Body_GetPosition(bodyID);
			transform.Translation.x = position.x;
			transform.Translation.y = position.y;

			b2Rot rotation = b2Body_GetRotation(bodyID);
			transform.Rotation.z = b2Rot_GetAngle(rotation);
		}
	}

	void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera)
	{
		Renderer2D::BeginScene(camera);

		auto groupTransformSprite = mRegistry.group<TransformComponent, SpriteComponent>();
		for (auto entity : groupTransformSprite)
		{
			auto [transform, sprite] = groupTransformSprite.get<TransformComponent, SpriteComponent>(entity);
			Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
		}
		Renderer2D::EndScene();
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		auto viewNativeScript = mRegistry.view<NativeScriptComponent>();
		for (auto entity : viewNativeScript)
		{
			auto& script = viewNativeScript.get<NativeScriptComponent>(entity);
			if (!script.Instance)
			{
				script.Instance = script.InstantiateScript();
				if(script.Instance) script.Instance->mEntity = Entity(entity, this);
				script.Instance->OnCreate();
			}

			script.Instance->OnUpdate(ts);
		}

		OnUpdatePhysics(ts);

		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		auto viewTransformCamera = mRegistry.view<TransformComponent, CameraComponent>();
		for (auto entity : viewTransformCamera)
		{
			auto [transform, camera] = viewTransformCamera.get<TransformComponent, CameraComponent>(entity);
			if (camera.Primary)
			{
				mainCamera = &camera.Camera;
				cameraTransform = transform.GetTransform();
				break;
			}
		}

		if (mainCamera)
		{
			Renderer2D::BeginScene(mainCamera->GetProjection(), cameraTransform);

			auto groupTransformSprite = mRegistry.group<TransformComponent, SpriteComponent>();
			for (auto entity : groupTransformSprite)
			{
				auto [transform, sprite] = groupTransformSprite.get<TransformComponent, SpriteComponent>(entity);
				Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
			}
			Renderer2D::EndScene();
		}
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		mViewportWidth = width;
		mViewportHeight = height;

		auto viewCamera = mRegistry.view<CameraComponent>();
		for (auto entity : viewCamera)
		{
			auto& camera_component = viewCamera.get<CameraComponent>(entity);
			if (!camera_component.FixedAspectRatio)
				camera_component.Camera.SetViewportSize(width, height);
		}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto viewCamera = mRegistry.view<CameraComponent>();
		for (auto entity : viewCamera)
		{
			const auto& cameraComponent = viewCamera.get<CameraComponent>(entity);
			if (cameraComponent.Primary)
				return Entity(entity, this);
		}

		return {};
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(false);
	}

	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SpriteComponent>(Entity entity, SpriteComponent& component)
	{
	}
	
	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if(mViewportWidth > 0 && mViewportHeight > 0)
			component.Camera.SetViewportSize(mViewportWidth, mViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
	}

}