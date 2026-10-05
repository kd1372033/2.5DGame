#include "Enemy.h"

#include "../../../../Scene/SceneManager.h"
#include "../Player/Player.h"
#include "../../Item/Item/Item.h"
#include "../../Terrains/Ground/Ground .h"

bool Enemy::s_showDebugWire = false;

void Enemy::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	m_polygon = std::make_shared<KdSquarePolygon>();
	m_polygon->SetMaterial("Asset/Textures/Enemy.png");
	m_polygon->SetSplit(8, 6);
	m_polygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);

	// コーンライトの有効化
	KdShaderManager::Instance().WriteCBConeLightEnable(true);

	m_pos = {};
	m_dir = { 1.0f, 0.0f, 0.0f }; // 初期状態: 右向き
	m_speed = 0.01f;
	m_anime = 0.0f;
	m_gravity = 0.0f;

	m_state = State::Walk;
	m_timer = 0.0f;

	m_chaseFlg = false;
	m_searchArea = 0.45f;
	m_itemSearchArea = 1.0f;
	m_dynamicangle = 0;
}

void Enemy::Update()
{
	// 先頭のエネミー（リーダー）のみがデバッグキー入力を受け取る
	bool isLeader = false;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (auto firstEnemy = std::dynamic_pointer_cast<Enemy>(obj))
		{
			if (firstEnemy.get() == this) isLeader = true;
			break;
		}
	}

	if (isLeader && (GetAsyncKeyState('A') & 0x0001))
	{
		s_showDebugWire = !s_showDebugWire;
	}

	bool isMoving = false;

	// タイマー更新（パトロール中のみ）
	if (!m_chaseFlg && !m_itemFlg)
	{
		m_timer += 1.0f;
	}

	// 1. 各状態での移動計算
	if (m_itemFlg)
	{
		UpdateItemAttract(isMoving);
	}
	else if (m_chaseFlg)
	{
		UpdatePlayerChase(isMoving);
	}
	else
	{
		UpdatePatrol(isMoving);
	}

	// 2. UVアニメーション更新
	UpdateAnimation(isMoving);

	// アイテム注視が終わった後など、移動していない（または向きが上書きされた）場合のために
	// m_dirID（アニメーションの向き）から視界の方向（m_dir）を正しく復元する
	if (m_state != State::Approach) // アイテムへ移動中以外なら向きを復元
	{
		switch (m_dirID)
		{
		case 0: m_dir = { 0.0f, 0.0f, -1.0f }; break; // 下
		case 1: m_dir = { -1.0f, 0.0f,  0.0f }; break; // 左
		case 2: m_dir = { 1.0f, 0.0f,  0.0f }; break; // 右
		case 3: m_dir = { 0.0f, 0.0f,  1.0f }; break; // 上
		}
	}

	// 重力計算
	m_pos.y -= m_gravity;
	m_gravity += 0.005f;

	// 行列の更新
	Math::Matrix scalemat = Math::Matrix::CreateScale(0.5f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;
}

void Enemy::PostUpdate()
{
	// 1. 地面・壁の当たり判定
	CheckCollision();

	// 2. プレイヤー索敵
	CheckPlayerSearch();

	// 3. アイテム索敵
	CheckItemSearch();

	// 4. サーチライト壁判定（Ray計算）
	CheckAttackWall();


	m_searchlightcolor = { 10.0f, 0.0f, 0.0f }; // 発見時: 赤



	// 3. 発生源と照射方向の設定
	Math::Vector3 lightPos = m_pos + Math::Vector3(0.0f, 0.1f, 0.0f);
	Math::Vector3 lightDir = m_dir;
	lightDir.y = 0.0f;
	if (lightDir.LengthSquared() > 0.0001f)
	{
		lightDir.Normalize();
	}
	else
	{
		lightDir = { 1.0f, 0.0f, 0.0f };
	}

	// 4. 定数バッファへ書き込み（壁までの距離 m_hitDistance を渡して固定）
	KdShaderManager::Instance().WriteCBConeLightEnable(true);
	KdShaderManager::Instance().WriteCBConeLight2D(
		m_raypos,               // 発振源（敵の中心）
		m_dir,               // 照射方向
		m_viewAngle,            // 照射角度
		m_hitDistance,          // ★ 壁までの地点の距離（貫通防止）
		m_searchlightcolor,     // カラー
		1.0f,                   // 輝度
		0.2f,                   // ぼかし
		m_isHitWall,            // 壁ヒットフラグ
		m_hitCenterPos,         // 壁の着弾座標
		m_hitNormal             // 壁の法線
	);
}

// =============================================================
// Update 分割関数
// =============================================================

void Enemy::UpdatePatrol(bool& isMoving)
{
	if (m_isHorizontalPatrol)
	{
		if (m_dirID == 1)
		{
			m_dir = { -1.0f, 0.0f, 0.0f }; // 左
		}
		else
		{
			m_dir = { 1.0f, 0.0f, 0.0f };  // 右
		}
	}
	else
	{
		if (m_dirID == 0)
		{
			m_dir = { 0.0f, 0.0f, -1.0f }; // 下
		}
		else
		{
			m_dir = { 0.0f, 0.0f, 1.0f };  // 上
		}
	}

	switch (m_state)
	{
	case State::Walk:
		isMoving = true;
		m_pos.x += m_speed * m_dir.x;

		if (m_timer >= 120.0f) // 約2秒移動
		{
			m_state = State::Wait;
			m_timer = 0.0f;
		}
		break;

	case State::Wait:
		isMoving = false;

		if (m_timer >= 60.0f) // 約1秒停止
		{
			m_dir.x *= -1.0f;
			m_state = State::Walk;
			m_timer = 0.0f;
		}
		break;
	}
}

void Enemy::UpdateItemAttract(bool& isMoving)
{
	auto pItem = m_wpItem.lock();
	if (!pItem)
	{
		m_itemFlg = false;
		m_itemAttractTimeout = 0.0f;
		return;
	}

	Math::Vector3 itemPos = pItem->GetPos();
	Math::Vector3 vItem = itemPos - m_pos;
	vItem.y = 0.0f; // XZ平面上の距離

	float dist = vItem.Length();
	m_itemAttractTimeout += 1.0f;

	bool isReached = (dist <= 0.2f);
	bool isTimeout = (m_itemAttractTimeout >= 90.0f);

	if (isReached || isTimeout || m_itemWaitTimer > 0.0f)
	{
		isMoving = false;

		if (m_itemWaitTimer <= 0.0f)
		{
			m_itemWaitTimer = 180.0f;
		}

		m_itemWaitTimer -= 1.0f;

		if (m_itemWaitTimer <= 0.0f)
		{
			pItem->OnHit();

			m_itemFlg = false;
			m_wpItem.reset();
			m_itemAttractTimeout = 0.0f;
			m_state = State::Wait;
			m_timer = 0.0f;
		}
	}
	else
	{
		isMoving = true;
		vItem.Normalize();
		m_dir = vItem;
		m_pos += m_dir * m_speed;
	}
}

void Enemy::UpdatePlayerChase(bool& isMoving)
{
	isMoving = true;
	float dashSpeed = m_speed * 1.75f;
	m_pos += m_dir * dashSpeed;
}

void Enemy::UpdateAnimation(bool isMoving)
{
	// 向きの同期（左右固定）
	if (m_dir.x > 0.0f)      m_dirID = 2; // 右
	else if (m_dir.x < 0.0f) m_dirID = 1; // 左

	// アニメーションフレーム更新
	if (isMoving)
	{
		m_anime += 0.1f;
		if (m_anime >= 8.0f) m_anime = 0.0f;
		m_polygon->SetUVRect(Run[m_dirID][static_cast<int>(m_anime) % 8]);
	}
	else
	{
		m_anime += 0.05f;
		if (m_anime >= 4.0f) m_anime = 0.0f;
		m_polygon->SetUVRect(Wait[m_dirID][static_cast<int>(m_anime) % 4]);
	}
}

// =============================================================
// PostUpdate 分割関数（当たり判定）
// =============================================================

void Enemy::CheckCollision()
{
	// 地面レイ判定
	KdCollider::RayInfo ray;
	float enableStepHigh = 0.2f;

	ray.m_pos = m_pos;
	ray.m_pos.y += enableStepHigh;
	ray.m_dir = { 0.0f, -1.0f, 0.0f };
	ray.m_range = m_gravity + enableStepHigh;
	ray.m_type = KdCollider::TypeGround;

	std::list<KdCollider::CollisionResult> retRayList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this) continue;
		obj->Intersects(ray, &retRayList);
	}

	float maxOverlap = 0.0f;
	Math::Vector3 hitPos;
	bool hitRay = false;

	for (auto& ret : retRayList)
	{
		if (maxOverlap < ret.m_overlapDistance)
		{
			maxOverlap = ret.m_overlapDistance;
			hitPos = ret.m_hitPos;
			hitRay = true;
		}
	}

	if (hitRay)
	{
		m_pos.y = hitPos.y;
		m_gravity = 0.0f;
	}

	// 壁スフィア判定
	KdCollider::SphereInfo sphere;
	sphere.m_sphere.Center = m_pos;
	sphere.m_sphere.Center.y += 0.3f;
	sphere.m_sphere.Radius = m_collisionArea;
	sphere.m_type = KdCollider::Type::TypeGround;

	std::list<KdCollider::CollisionResult> retSphereList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this) continue;
		obj->Intersects(sphere, &retSphereList);
	}

	maxOverlap = 0.0f;
	bool hitSphere = false;
	Math::Vector3 hitDir = Math::Vector3::Zero;

	for (auto& ret : retSphereList)
	{
		Math::Vector3 dir = ret.m_hitDir;
		dir.y = 0.0f;

		if (dir.LengthSquared() > 0.0001f)
		{
			if (maxOverlap < ret.m_overlapDistance)
			{
				maxOverlap = ret.m_overlapDistance;
				dir.Normalize();
				hitDir = dir;
				hitSphere = true;
			}
		}
	}

	if (hitSphere)
	{
		m_pos.x += hitDir.x * maxOverlap;
		m_pos.z += hitDir.z * maxOverlap;
	}

	if (s_showDebugWire)
	{
		m_pDebugWire->AddDebugSphere(m_pos, m_collisionArea, kNormalColor);
	}
}

bool Enemy::IsPlayerInFieldOfView(const std::shared_ptr<Player>& player)
{
	if (!player) return false;

	Math::Vector3 eyePos = m_pos;
	eyePos.y += 0.5f;

	Math::Vector3 targetPos = player->GetPos();
	targetPos.y += 0.5f;

	Math::Vector3 vToPlayer = targetPos - eyePos;
	vToPlayer.y = 0.0f; // 水平判定

	float dist = vToPlayer.Length();

	// 1. 視界距離チェック
	if (dist > m_viewDistance || dist < 0.001f) return false;

	vToPlayer.Normalize();

	// 2. 扇形角度チェック
	Math::Vector3 forward = m_dir;
	forward.y = 0.0f;
	forward.Normalize();

	float dot = forward.Dot(vToPlayer);
	dot = std::clamp(dot, -1.0f, 1.0f);

	float angleDeg = DirectX::XMConvertToDegrees(std::acos(dot));
	if (angleDeg > (m_viewAngle * 0.5f)) return false;

	// 3. 壁遮蔽判定
	Math::Vector3 rayDir = targetPos - eyePos;
	float rayRange = rayDir.Length();
	rayDir.Normalize();

	KdCollider::RayInfo ray;
	ray.m_pos = eyePos;
	ray.m_dir = rayDir;
	ray.m_range = rayRange;
	ray.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this || obj == player) continue;

		std::list<KdCollider::CollisionResult> retRayList;
		if (obj->Intersects(ray, &retRayList))
		{
			for (auto& ret : retRayList)
			{
				if (std::abs(ret.m_hitNDir.y) < 0.5f)
				{
					return false; // 壁に遮られている
				}
			}
		}
	}

	return true;
}

void Enemy::CheckPlayerSearch()
{
	std::shared_ptr<Player> targetPlayer = nullptr;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (auto player = std::dynamic_pointer_cast<Player>(obj))
		{
			targetPlayer = player;
			break;
		}
	}

	if (!targetPlayer) return;

	float dist = (targetPlayer->GetPos() - m_pos).Length();

	// ゲームオーバー接触判定
	if (dist < 0.15f)
	{
		SceneManager::Instance().SetClearFlag(false);
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Result);
		return;
	}

	// -------------------------------------------------------------
	// 追跡＆見失い処理
	// -------------------------------------------------------------
	if (!m_chaseFlg)
	{
		// 【未追跡】扇形視界に入り、かつ壁がなければ追跡開始
		if (IsPlayerInFieldOfView(targetPlayer))
		{
			m_chaseFlg = true;
		}
	}
	else
	{
		// 【追跡中】距離離脱 または 壁遮蔽で見失う
		bool isLost = false;

		if (dist > m_viewDistance)
		{
			isLost = true; // 距離離脱
		}
		else
		{
			// 壁遮蔽判定
			Math::Vector3 eyePos = m_pos;
			eyePos.y += 0.5f;

			Math::Vector3 targetPos = targetPlayer->GetPos();
			targetPos.y += 0.5f;

			Math::Vector3 rayDir = targetPos - eyePos;
			float rayRange = rayDir.Length();
			rayDir.Normalize();

			KdCollider::RayInfo ray;
			ray.m_pos = eyePos;
			ray.m_dir = rayDir;
			ray.m_range = rayRange;
			ray.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

			for (auto& obj : SceneManager::Instance().GetObjList())
			{
				if (obj.get() == this || obj == targetPlayer) continue;

				std::list<KdCollider::CollisionResult> retRayList;
				if (obj->Intersects(ray, &retRayList))
				{
					for (auto& ret : retRayList)
					{
						if (std::abs(ret.m_hitNDir.y) < 0.5f)
						{
							isLost = true;
							break;
						}
					}
				}
				if (isLost) break;
			}
		}

		if (isLost)
		{
			// 追跡を解除して停止状態へ
			m_chaseFlg = false;
			m_state = State::Wait;
			m_timer = 0.0f;
		}
		else
		{
			// 追尾向きの更新
			Math::Vector3 chaseVec = targetPlayer->GetPos() - m_pos;
			chaseVec.y = 0.0f;
			chaseVec.Normalize();
			if (chaseVec.LengthSquared() > 0.0f)
			{
				m_dir = chaseVec;
			}
		}
	}

	// 扇形デバッグ描画
	if (s_showDebugWire && m_pDebugWire)
	{
		float halfAngle = DirectX::XMConvertToRadians(m_viewAngle * 0.5f);

		Math::Vector3 fwd = m_dir;
		fwd.y = 0.0f;
		fwd.Normalize();

		Math::Matrix rotLeft = Math::Matrix::CreateRotationY(-halfAngle);
		Math::Matrix rotRight = Math::Matrix::CreateRotationY(halfAngle);

		Math::Vector3 dirLeft = Math::Vector3::TransformNormal(fwd, rotLeft);
		Math::Vector3 dirRight = Math::Vector3::TransformNormal(fwd, rotRight);

		Math::Vector3 eyePos = m_pos;
		eyePos.y += 0.5f;

		m_pDebugWire->AddDebugLine(eyePos, dirLeft, m_viewDistance, kGreenColor);
		m_pDebugWire->AddDebugLine(eyePos, dirRight, m_viewDistance, kGreenColor);
	}
}

void Enemy::CheckItemSearch()
{
	if (s_showDebugWire && m_pDebugWire)
	{
		m_pDebugWire->AddDebugSphere(m_pos, m_itemSearchArea, kRedColor);
	}

	std::shared_ptr<Item> pFoundItem = nullptr;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (auto itemObj = std::dynamic_pointer_cast<Item>(obj))
		{
			if (itemObj->IsHeld()) continue;
			if (!itemObj->HasBeenThrown()) continue;

			Math::Vector3 itemPos = itemObj->GetPos();
			Math::Vector3 vDist = itemPos - m_pos;
			vDist.y = 0.0f;

			float distXZ = vDist.Length();

			if (distXZ <= m_itemSearchArea)
			{
				pFoundItem = itemObj;
				break;
			}
		}
	}

	if (pFoundItem)
	{
		if (!m_itemFlg)
		{
			m_itemFlg = true;
			m_chaseFlg = false;
			m_wpItem = pFoundItem;
			m_itemWaitTimer = 0.0f;
		}
	}
	else
	{
		if (m_itemFlg && m_itemWaitTimer <= 0.0f)
		{
			m_itemFlg = false;
			m_wpItem.reset();
		}
	}
}

void Enemy::CheckAttackWall()
{
	Math::Vector3 rayDir = m_dir;
	rayDir.y = 0.0f; // 水平方向

	if (rayDir.LengthSquared() > 0.001f)
	{
		rayDir.Normalize();
	}
	else
	{
		rayDir = { 1.0f, 0.0f, 0.0f };
	}

	// ★ 修正: レイの発射位置を、敵の中心より「少し後方(-0.2f)」からスタートさせる
	// これにより、敵が壁に密着してもレイの始点が壁の内部に埋まるのを防ぎます
	Math::Vector3 basePos = m_pos + Math::Vector3(0.0f, 0.25f, 0.0f);
	Math::Vector3 rayPos = basePos - (rayDir * 0.2f);

	float maxRayDist = m_viewRenderDistance + 0.2f; // 後ろに引いた分、最大照射距離も加算

	// 初期設定（壁に当たっていない時）
	m_isHitWall = false;
	m_hitCenterPos = basePos + rayDir * m_viewRenderDistance;
	m_hitNormal = -rayDir;
	m_hitDistance = m_viewRenderDistance;

	// まっすぐ前方にレイを発射（TypeBump のみ）
	KdCollider::RayInfo rayInfo(
		KdCollider::TypeBump,
		rayPos,
		rayDir,
		maxRayDist
	);

	std::list<KdCollider::CollisionResult> retHitList;

	for (const auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this || std::dynamic_pointer_cast<Player>(obj)) continue;

		obj->Intersects(rayInfo, &retHitList);
	}

	float minOverlap = maxRayDist;
	for (const auto& result : retHitList)
	{
		// 後ろに引いた位置から発射しているため、判定の距離も後方に引いた分を加算調整
		if (result.m_overlapDistance < minOverlap)
		{
			minOverlap = result.m_overlapDistance;
			m_isHitWall = true;
			m_hitCenterPos = result.m_hitPos;    // 壁の着弾座標
			m_hitNormal = result.m_hitNDir;      // 壁の法線

			// ★ 敵の位置(basePos)から壁までの「実際の距離」を計算
			m_hitDistance = (m_hitCenterPos - basePos).Length();
		}
	}

	m_hitNormal.Normalize();

	m_raypos = basePos;
	m_tohitvector = rayDir;

	// デバッグ描画
	if (s_showDebugWire && m_pDebugWire)
	{
		if (m_isHitWall)
		{
			m_pDebugWire->AddDebugLine(basePos, m_hitCenterPos, kGreenColor);
		}
		else
		{
			m_pDebugWire->AddDebugLine(basePos, basePos + (rayDir * m_viewRenderDistance), kRedColor);
		}
	}
}

// =============================================================
// 描画処理
// =============================================================

void Enemy::GenerateDepthMapFromLight()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}

void Enemy::DrawLit()
{
	// エネミー本体の描画のみ（ライト情報は PostUpdate で書き込み済み）
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}

void Enemy::DrawBright()
{
	// 板ポリゴン（m_viewPolygon）の描画は撤去し、シェーダーコーンライトのみで描画
}