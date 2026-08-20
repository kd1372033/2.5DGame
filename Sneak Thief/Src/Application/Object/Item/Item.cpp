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
	m_hitRadius = 1.5f;
	m_throwSafetyTimer = 0.0f;
	m_pickUpCooldown = 0.2f; // 初期値として0.2秒をセット

	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape("Item", { 0,0,0 }, m_hitRadius, KdCollider::Type::TypeSight);
}

// 拾われた時の処理
void Item::PickUp(std::shared_ptr<Player> pOwner)
{
	m_isHeld = true;
	m_isThrown = false;
	m_wpOwner = pOwner;
	m_vec = { 0.0f, 0.0f, 0.0f };
}

// 方向を指定して投げる
void Item::StartThrow(const Math::Vector3& dir)
{
	m_dir = dir;
	StartThrow();
}

// 投擲開始処理
void Item::StartThrow()
{
	m_isHeld = false;
	m_isThrown = true;
	m_throwSafetyTimer = 0.15f; // 安全タイマー

	// 初速の設定
	m_vec.x = 0.08f * m_dir.x;
	m_vec.y = 0.03f; // 打ち上げ力
	m_vec.z = 0.06f * m_dir.z;

	// 行列計算
	Math::Matrix scalemat = Math::Matrix::CreateScale(0.125f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;
}

void Item::Update()
{
	// 拾いタイマーをカウントダウン
	if (m_pickUpCooldown > 0.0f)
	{
		m_pickUpCooldown -= 1.0f / 60.0f;
	}
	// 1. 手に持たれている時はプレイヤーの位置に追従
	if (m_isHeld)
	{
		auto owner = m_wpOwner.lock();
		if (owner)
		{
			m_pos = owner->GetPos();
			m_pos.y += 0.125f; // プレイヤーの頭上・手元の高さに調整
		}
	}
	// 2. 投げられている時は放物線移動
	else if (m_isThrown)
	{
		m_vec.y += gravity;
		m_pos += m_vec;
	}

	// ワールド行列の更新
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

	// =========================================================
	// 1. 一体型ステージモデルに対する「壁判定＆跳ね返り」
	// =========================================================
	if (m_isThrown)
	{
		KdCollider::SphereInfo bumpSphere;
		bumpSphere.m_sphere.Center = m_pos;
		bumpSphere.m_sphere.Center.y += 0.1f;
		bumpSphere.m_sphere.Radius = 0.15f;
		bumpSphere.m_type = KdCollider::TypeGround; // ステージモデルと同じ TypeGround を指定

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

			// 【重要】Y成分の絶対値が小さい（垂直に近い面）場合のみ「壁」とみなして跳ね返す
			// ※ Y成分が大きい面は「床」なので、ここでは跳ね返さず下の着地レイキャストに任せる
			if (fabsf(hitDir.y) < 0.6f)
			{
				hitDir.y = 0.0f; // 水平方向のみに補正
				hitDir.Normalize();

				// 壁へのめり込み解除
				m_pos += hitDir * ret.m_overlapDistance;

				float dot = m_vec.Dot(hitDir);
				if (dot < 0.0f)
				{
					// 壁に対して速度を反射させる
					m_vec = m_vec - 2.0f * dot * hitDir;

					// 跳ね返り時の減衰（60%に勢いを落とす）
					m_vec.x *= 0.8f;
					m_vec.z *= 0.8f;
				}
				break; // 1つの壁面と当たったら抜ける
			}
		}
	}

	// =========================================================
	// 2. 地面（床面）への着地判定
	// =========================================================
	if (m_isThrown && m_vec.y <= 0.0f && m_throwSafetyTimer <= 0.0f)
	{
		KdCollider::RayInfo ray;
		ray.m_pos = m_pos;
		ray.m_pos.y += 0.2f;
		ray.m_dir = { 0.0f, -1.0f, 0.0f };
		ray.m_range = 0.2f + fabsf(m_vec.y) + 0.1f; // 落下速度に応じた可変レイ長
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
			m_pos = hitPos;               // 地面の高さに固定
			m_vec = { 0.0f, 0.0f, 0.0f }; // 速度リセット
			m_isThrown = false;           // 着地（投げ状態終了）

			Math::Matrix scalemat = Math::Matrix::CreateScale(0.125f);
			Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
			m_mWorld = scalemat * transmat;
		}
	}

	// =========================================================
	// 3. 敵の索敵判定（既存処理）
	// =========================================================
	KdCollider::SphereInfo searchSphere;
	searchSphere.m_sphere.Center = m_pos;
	searchSphere.m_sphere.Radius = 0.25f;
	searchSphere.m_type = KdCollider::Type::TypeGround;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (enemy)
		{
			std::list<KdCollider::CollisionResult> retList;
			if (enemy->Intersects(searchSphere, &retList))
			{
				enemy->SetTargetItem(shared_from_this());
				enemy->AttractTo(m_pos);
			}
		}
	}
}

void Item::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}