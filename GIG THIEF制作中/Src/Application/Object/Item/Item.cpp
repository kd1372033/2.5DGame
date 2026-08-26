#include "Item.h"
#include "../Player/Player.h"
#include "../Enemy/Enemy.h"

void Item::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	m_polygon = std::make_shared<KdSquarePolygon>();

	m_polygon->SetMaterial("Asset/Textures/item.png");
	m_polygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);

	m_pos = { 0.0f, 0.0f, 0.0f };
	m_vec = { 0.0f, 0.0f, 0.0f };
	m_dir = { 0.0f, 0.0f, 0.0f };
	m_isThrown = false;
	m_isHeld = false;
	m_hasBeenThrown = false;
	m_isLandedAfterThrow = false; // ★ 初期化

	m_hitRadius = 1.5f;
	m_throwSafetyTimer = 0.0f;
	m_pickUpCooldown = 0.2f;

	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape("Item", { 0,0,0 }, m_hitRadius, KdCollider::Type::TypeSight);
}

// 拾われた時の処理
void Item::PickUp(std::shared_ptr<Player> pOwner)
{
	m_isHeld = true;
	m_isThrown = false;
	m_hasBeenThrown = false;
	m_isLandedAfterThrow = false; // ★ 手に持ったら着地フラグを解除
	m_wpOwner = pOwner;
	m_vec = { 0.0f, 0.0f, 0.0f };
}

// 方向を指定して投げる
void Item::StartThrow(const Math::Vector3& dir)
{
	m_isHeld = false;
	m_isThrown = true;
	m_hasBeenThrown = true;
	m_isLandedAfterThrow = false; // ★ 投げた直後は空中なので false

	m_vec = dir * 0.2f;

	if (m_pCollider)
	{
		m_pCollider->RegisterCollisionShape("Item", { 0,0,0 }, m_hitRadius, KdCollider::Type::TypeSight);
	}

	m_dir = dir;
	StartThrow();
}

// 投擲開始処理
void Item::StartThrow()
{
	m_isHeld = false;
	m_isThrown = true;
	m_hasBeenThrown = true;
	m_isLandedAfterThrow = false; // ★ 投げた直後は空中なので false
	m_throwSafetyTimer = 0.15f;

	// 初速の設定
	m_vec.x = 0.08f * m_dir.x;
	m_vec.y = 0.03f;
	m_vec.z = 0.06f * m_dir.z;

	// 行列計算
	Math::Matrix scalemat = Math::Matrix::CreateScale(0.125f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;
}

void Item::Update()
{
	if (m_pickUpCooldown > 0.0f)
	{
		m_pickUpCooldown -= 1.0f / 60.0f;
	}

	if (m_isHeld)
	{
		auto owner = m_wpOwner.lock();
		if (owner)
		{
			m_pos = owner->GetPos();
			m_pos.y += 0.125f;
		}
	}
	else if (m_isThrown)
	{
		m_vec.y += gravity;
		m_pos += m_vec;
	}

	Math::Matrix scalemat = Math::Matrix::CreateScale(0.125f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;
}

void Item::PostUpdate()
{
	if (m_isHeld) return;

	if (m_throwSafetyTimer > 0.0f)
	{
		m_throwSafetyTimer -= 1.0f / 60.0f;
	}

	// 1. 壁判定＆跳ね返り
	if (m_isThrown)
	{
		KdCollider::SphereInfo bumpSphere;
		bumpSphere.m_sphere.Center = m_pos;
		bumpSphere.m_sphere.Center.y += 0.1f;
		bumpSphere.m_sphere.Radius = 0.15f;
		bumpSphere.m_type = KdCollider::TypeGround;

		std::list<KdCollider::CollisionResult> retBumpList;

		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() == this) continue;
			if (std::dynamic_pointer_cast<Player>(obj)) continue;

			obj->Intersects(bumpSphere, &retBumpList);
		}

		for (auto& ret : retBumpList)
		{
			Math::Vector3 hitDir = ret.m_hitDir;

			if (fabsf(hitDir.y) < 0.6f)
			{
				hitDir.y = 0.0f;
				hitDir.Normalize();

				m_pos += hitDir * ret.m_overlapDistance;

				float dot = m_vec.Dot(hitDir);
				if (dot < 0.0f)
				{
					m_vec = m_vec - 2.0f * dot * hitDir;
					m_vec.x *= 0.8f;
					m_vec.z *= 0.8f;
				}
				break;
			}
		}
	}

	// 2. 地面（床面）への着地判定
	if (m_isThrown && m_vec.y <= 0.0f && m_throwSafetyTimer <= 0.0f)
	{
		KdCollider::RayInfo ray;
		ray.m_pos = m_pos;
		ray.m_pos.y += 0.2f;
		ray.m_dir = { 0.0f, -1.0f, 0.0f };
		ray.m_range = 0.2f + fabsf(m_vec.y) + 0.1f;
		ray.m_type = KdCollider::TypeGround;

		std::list<KdCollider::CollisionResult> retRayList;

		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() == this) continue;
			if (std::dynamic_pointer_cast<Player>(obj)) continue;

			obj->Intersects(ray, &retRayList);
		}

		float maxOverLap = 0.0f;
		Math::Vector3 hitPos;
		bool hit = false;

		for (auto& ret : retRayList)
		{
			if (maxOverLap < ret.m_overlapDistance)
			{
				maxOverLap = ret.m_overlapDistance;
				hitPos = ret.m_hitPos;
				hit = true;
			}
		}

		if (hit)
		{
			m_pos = hitPos;
			m_vec = { 0.0f, 0.0f, 0.0f };
			m_isThrown = false;

			// ★★★ 投げられた後、床に着地したタイミングでフラグをONにする ★★★
			m_isLandedAfterThrow = true;

			Math::Matrix scalemat = Math::Matrix::CreateScale(0.125f);
			Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
			m_mWorld = scalemat * transmat;
		}
	}
}

void Item::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}