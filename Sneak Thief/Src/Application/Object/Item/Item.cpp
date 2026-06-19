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
	m_isThrown = false;
	m_hitRadius = 1.5f;
	m_throwSafetyTimer = 0.0f; // タイマー初期化
		
	// 当たり判定登録
	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape("Item", { 0,0,0 }, m_hitRadius, KdCollider::Type::TypeSight);
}

void Item::StartThrow()
{
	m_isThrown = true;
	m_throwSafetyTimer = 0.15f; // 投げてから0.15秒間（約9フレーム）は地面判定を絶対にしない無敵時間

	// 速度(m_vec)の初速を決める
	m_vec.x = 0.06f * m_dir.x;
	m_vec.y = 0.06f;           // 縦への打ち上げ力
	m_vec.z = 0.06f * m_dir.z;

	// 行列の先行確定
	Math::Matrix scalemat = Math::Matrix::CreateScale(0.125f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;

}

void Item::Update()
{
	// =========================================================
	// Updateでは純粋な移動計算（放物線）のみを行う！
	// =========================================================
	if (m_isThrown)
	{
		// 1. 速度のY成分に重力を足す
		m_vec.y += gravity;

		// 2. 速度を現在の座標に加算する
		m_pos += m_vec;
	}

	// =========================================================
	// ワールド行列の更新
	// =========================================================
	Math::Matrix scalemat = Math::Matrix::CreateScale(0.125f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;
}

void Item::PostUpdate()
{
	// 安全タイマーのカウントダウン（1フレームずつ減らす）
	if (m_throwSafetyTimer > 0.0f)
	{
		m_throwSafetyTimer -= 1.0f / 60.0f;
	}

	// =========================================================
	// レイ判定による自動着地システム
	// =========================================================
	// 条件：投げられていて、落下中で、安全タイマーが終了している時のみ
	if (m_isThrown && m_vec.y < 0.0f && m_throwSafetyTimer <= 0.0f)
	{
		KdCollider::RayInfo ray;
		ray.m_pos = m_pos;
		ray.m_pos.y += 0.1f;
		ray.m_dir = { 0.0f, -1.0f, 0.0f };
		ray.m_range = 0.25f;
		ray.m_type = KdCollider::TypeGround;

		// デバッグ用に下向きのレイを描画（赤色）
		m_pDebugWire->AddDebugLine(ray.m_pos, ray.m_dir, ray.m_range, kRedColor);

		std::list<KdCollider::CollisionResult> retRayList;

		// 全オブジェクトと当たり判定を行う
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

		// 地面にヒットしていたら着地処理
		if (hit)
		{
			m_pos = hitPos;               // 正確な地面の高さ（-1.6f付近）に座標を固定
			m_vec = { 0.0f, 0.0f, 0.0f }; // 速度を完全にゼロにする
			m_isThrown = false;           // 投げ状態を終了してその場に留まらせる

			// 行列も即時反映
			Math::Matrix scalemat = Math::Matrix::CreateScale(0.125f);
			Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
			m_mWorld = scalemat * transmat;
		}
	}

	// =========================================================
	// 敵の索敵判定
	// =========================================================
	KdCollider::SphereInfo searchSphere;
	searchSphere.m_sphere.Center = m_pos;
	searchSphere.m_sphere.Radius = 0.25f; // 半径 2.5 メートル（固定値に変更）
	searchSphere.m_type = KdCollider::Type::TypeGround;

	// デバッグ用：索敵範囲を「緑色」または「黄色」の球で描画
	m_pDebugWire->AddDebugSphere(searchSphere.m_sphere.Center, searchSphere.m_sphere.Radius, kGreenColor);

	// 全オブジェクトの中から「敵」を探す
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (enemy)
		{
			std::list<KdCollider::CollisionResult> retList;
			if (enemy->Intersects(searchSphere, &retList))
			{
				enemy->SetTargetItem(shared_from_this());
				enemy->AttractTo(m_pos); // 敵が近づく
			}
		}
	}
}

void Item::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}