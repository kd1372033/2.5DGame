#include "Enemy.h"
#include "../../Scene/SceneManager.h"
#include "../Player/Player.h"
#include "../Item/Item.h"

bool Enemy::s_showDebugWire = false;

void Enemy::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	// 視界ポリゴンの生成
	m_viewPolygon = std::make_shared<VisionPolygon>();

	m_polygon = std::make_shared<KdSquarePolygon>();
	m_polygon->SetMaterial("Asset/Textures/Enemy.png");
	m_polygon->SetSplit(8, 6);
	m_polygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);

	// 8/25 追加
	// サーチライト用デカールポリゴンの生成
	m_decalPolygon = std::make_shared<KdSquarePolygon>();
	m_decalPolygon->SetMaterial("Asset/Textures/Decal.png");
	// 画像の下端中央を基点にする（ライトの発射位置に合わせやすくするため）
	m_decalPolygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);
	// ========================================================================

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

	// 視界ポリゴンの形状を最新の向き・角度に合わせて更新
	UpdateViewPolygon();

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

	// 視界ポリゴンの形状を最新の向き・角度に合わせて更新
	//UpdateViewPolygon();
}

// =============================================================
// Update 分割関数
// =============================================================

void Enemy::UpdatePatrol(bool& isMoving)
{
	// 例: パトロール中の向きを確実に m_dir にセットする
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
		m_pos.x += m_speed * m_dir.x; // ★X軸方向のみ移動（左右巡回）

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
			m_dir.x *= -1.0f; // ★左右反転
			m_state = State::Walk;
			m_timer = 0.0f;
		}
		break;
	}
}

void Enemy::UpdateItemAttract(bool& isMoving)
{
	// safe lock: ローカル変数に保持して一貫性を保つ
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

	bool isReached = (dist <= 0.3f);
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
			pItem->OnHit(); // 安全に呼び出し

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
	sphere.m_sphere.Radius = 0.3f;
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

	// 3. 壁遮蔽判定（法線ベクトル m_hitNDir で壁だけを判定）
	Math::Vector3 rayDir = targetPos - eyePos;
	float rayRange = rayDir.Length();
	rayDir.Normalize();

	KdCollider::RayInfo ray;
	ray.m_pos = eyePos;
	ray.m_dir = rayDir;
	ray.m_range = rayRange;
	// 地面・障害物フラグを対象にする
	ray.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this || obj == player) continue;

		std::list<KdCollider::CollisionResult> retRayList;
		if (obj->Intersects(ray, &retRayList))
		{
			for (auto& ret : retRayList)
			{
				// ヒットした面の法線 m_hitNDir のY成分をチェック
				// 垂直な面（壁）に当たった場合のみ「見えない」と判定
				//if (std::abs(ret.m_hitNDir.y) < 0.5f)
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
						// ヒットした面が「壁（垂直面）」であれば遮蔽
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
	if (s_showDebugWire)
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

// -------------------------------------------------------------
// 扇形視界ポリゴンの生成・更新処理
// -------------------------------------------------------------

void Enemy::UpdateViewPolygon() {

	if (!m_viewPolygon) return;

	std::vector<KdPolygon::Vertex> vertices;

	unsigned int centerColor = 0xFFFFFFFF;
	unsigned int outerColor = 0x00FFFFFF;

	// 1. 向きベクトルの正規化
	Math::Vector3 dir = m_dir;
	dir.y = 0.0f;
	if (dir.LengthSquared() == 0.0f) dir = { 0.0f, 0.0f, 1.0f };
	dir.Normalize();

	float halfAngle = DirectX::XMConvertToRadians(m_viewAngle * 0.5f);
	float baseAngle = std::atan2(dir.x, dir.z);

	// 2. 正面レイキャスト（壁判定）
	Math::Vector3 eyePosW = m_pos + Math::Vector3(0.0f, 0.5f, 0.0f);

	KdCollider::RayInfo frontRay;
	frontRay.m_pos = eyePosW;
	frontRay.m_dir = dir;
	frontRay.m_range = m_viewRenderDistance;
	frontRay.m_type = KdCollider::TypeGround | KdCollider::TypeBump;

	float minHitDist = m_viewRenderDistance;
	bool isHitWall = false;
	Math::Vector3 wallNormal = Math::Vector3::Zero;

	for (const auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this || std::dynamic_pointer_cast<Player>(obj)) continue;

		std::list<KdCollider::CollisionResult> retRayList;
		if (obj->Intersects(frontRay, &retRayList))
		{
			for (const auto& ret : retRayList)
			{
				if (std::abs(ret.m_hitNDir.y) < 0.3f)
				{
					if (ret.m_overlapDistance < minHitDist)
					{
						minHitDist = ret.m_overlapDistance;
						wallNormal = ret.m_hitNDir;
						isHitWall = true;
					}
				}
			}
		}
	}

	// 3. 左右の斜め方向の距離を正しく計算（コサイン補正）
	float angleLeft = baseAngle - halfAngle;
	float angleRight = baseAngle + halfAngle;

	Math::Vector3 dirLeft = { std::sin(angleLeft),  0.0f, std::cos(angleLeft) };
	Math::Vector3 dirRight = { std::sin(angleRight), 0.0f, std::cos(angleRight) };

	// 正面距離(minHitDist)をベースに、左右の斜め辺の到達距離を補正
	// (斜め方向の直線距離 = 正面距離 / cos(halfAngle))
	float cosVal = std::cos(halfAngle);
	float frontFloorDist = isHitWall ? std::max(0.0f, minHitDist - 0.02f) : m_viewRenderDistance;

	// 床を伸びる距離（左右）
	float sideFloorDist = frontFloorDist / cosVal;

	float polyY = 0.05f;

	// 4. 頂点座標の構築
	Math::Vector3 v0_pos = m_pos + Math::Vector3(0.0f, polyY, 0.0f);

	Math::Vector3 v1_floor = m_pos + dirLeft * sideFloorDist + Math::Vector3(0.0f, polyY, 0.0f);
	Math::Vector3 v2_floor = m_pos + dirRight * sideFloorDist + Math::Vector3(0.0f, polyY, 0.0f);

	KdPolygon::Vertex v0, v1_f, v2_f;

	v0.pos = v0_pos;
	v0.color = centerColor;
	v0.UV = { 0.5f, 0.5f };
	v0.normal = { 0.0f, 1.0f, 0.0f };

	v1_f.pos = v1_floor;
	v1_f.color = isHitWall ? centerColor : outerColor;
	v1_f.UV = { 0.0f, 0.0f };
	v1_f.normal = { 0.0f, 1.0f, 0.0f };

	v2_f.pos = v2_floor;
	v2_f.color = isHitWall ? centerColor : outerColor;
	v2_f.UV = { 1.0f, 0.0f };
	v2_f.normal = { 0.0f, 1.0f, 0.0f };

	// 床面
	vertices.push_back(v0);
	vertices.push_back(v1_f);
	vertices.push_back(v2_f);

	// 5. 壁面の立ち上げ（壁衝突時のみ）
	if (isHitWall)
	{
		// 余り距離（立ち上げる高さ）＝ 設定最大距離 - 壁までの正面距離
		float excess = m_viewRenderDistance - frontFloorDist;

		Math::Vector3 v1_wall = v1_floor + (wallNormal * 0.02f) + Math::Vector3(0.0f, excess, 0.0f);
		Math::Vector3 v2_wall = v2_floor + (wallNormal * 0.02f) + Math::Vector3(0.0f, excess, 0.0f);

		KdPolygon::Vertex v1_w, v2_w;

		v1_w.pos = v1_wall;
		v1_w.color = outerColor;
		v1_w.UV = { 0.0f, 0.0f };
		v1_w.normal = wallNormal;

		v2_w.pos = v2_wall;
		v2_w.color = outerColor;
		v2_w.UV = { 1.0f, 0.0f };
		v2_w.normal = wallNormal;

		vertices.push_back(v1_f);
		vertices.push_back(v1_w);
		vertices.push_back(v2_w);

		vertices.push_back(v1_f);
		vertices.push_back(v2_w);
		vertices.push_back(v2_f);
	}

	m_viewPolygon->SetVertices(vertices);
}

// 8/25前
//void Enemy::UpdateViewPolygon() {
//
//	if (!m_viewPolygon) return;
//
//	std::vector<KdPolygon::Vertex> vertices;
//	const int slice = 5;
//	float halfAngle = DirectX::XMConvertToRadians(m_viewAngle * 0.5f);
//
//	unsigned int centerColor = 0xFFFFFFFF;
//	unsigned int outerColor = 0x00FFFFFF;
//
//	float baseAngle = std::atan2(m_dir.x, m_dir.z);
//
//	// 1. レイの発射位置
//
//	// 腰〜胸の高さ（0.5f）から飛ばすことで床ポリゴンとの誤判定を防止
//
//	Math::Vector3 eyePosW = m_pos;
//	eyePosW.y += 0.5f;
//
//	std::vector<float> rayDistances(slice + 1, m_viewRenderDistance);
//
//	// 2. レイキャストで壁までの距離を計測
//
//	for (int i = 0; i <= slice; ++i) {
//		float t = static_cast<float>(i) / slice;
//		float angle = baseAngle - halfAngle + (halfAngle * 2.0f) * t;
//
//		Math::Vector3 rayDirW = { std::sin(angle), 0.0f, std::cos(angle) };
//		KdCollider::RayInfo ray;
//
//		ray.m_pos = eyePosW;
//		ray.m_dir = rayDirW;
//		ray.m_range = m_viewRenderDistance;
//
//		// TypeGround と TypeBump の両方を検出
//		ray.m_type = KdCollider::TypeGround | KdCollider::TypeBump;
//
//		float minHitDist = m_viewRenderDistance;
//		bool hit = false;
//
//		for (const auto& obj : SceneManager::Instance().GetObjList())
//		{
//			if (obj.get() == this) continue;
//			if (std::dynamic_pointer_cast<Player>(obj)) continue;
//
//			std::list<KdCollider::CollisionResult> retRayList;
//			if (obj->Intersects(ray, &retRayList))
//			{
//				for (const auto& ret : retRayList)
//				{
//					// ★一体型モデル用の法線判定：
//
//					// 法線のY成分の絶対値が 0.5 未満 ＝ 垂直に近い面（壁）
//
//					// ※もしこれでも突き抜ける場合は std::abs(ret.m_hitNDir.y) < 0.8f に緩めてみてください
//
//					if (std::abs(ret.m_hitNDir.y) < 0.5f)
//					{
//						if (ret.m_overlapDistance < minHitDist)
//						{
//							minHitDist = ret.m_overlapDistance;
//							hit = true;
//						}
//					}
//				}
//			}
//		}
//		// 壁にヒットした場合は、壁の厚み＋めり込み防止として 0.2f 手前でカットする
//
//		if (hit)
//		{
//			rayDistances[i] = std::max(0.0f, minHitDist - 0.2f);
//		}
//		else
//		{
//			rayDistances[i] = m_viewRenderDistance;
//		}
//	}
//
//	// 3. ポリゴンの構築
//	// 床よりわずかに浮かす（Zファイティング防止）
//	float polyY = 0.05f;
//	for (int i = 0; i < slice; ++i)
//	{
//
//		float t1 = static_cast<float>(i) / slice;
//		float t2 = static_cast<float>(i + 1) / slice;
//
//		float angle1 = baseAngle - halfAngle + (halfAngle * 2.0f) * t1;
//		float angle2 = baseAngle - halfAngle + (halfAngle * 2.0f) * t2;
//
//		float dist1 = rayDistances[i];
//		float dist2 = rayDistances[i + 1];
//
//		Math::Vector3 v0W = eyePosW;
//		Math::Vector3 v1W = eyePosW + Math::Vector3(std::sin(angle1), 0.0f, std::cos(angle1)) * dist1;
//		Math::Vector3 v2W = eyePosW + Math::Vector3(std::sin(angle2), 0.0f, std::cos(angle2)) * dist2;
//
//		KdPolygon::Vertex v0;
//
//		v0.pos = v0W - m_pos;
//		v0.pos.y = polyY;
//		v0.color = centerColor;
//		v0.UV = { 0.5f, 0.5f };
//		v0.normal = { 0.0f, 1.0f, 0.0f };
//
//		KdPolygon::Vertex v1;
//		v1.pos = v1W - m_pos;
//		v1.pos.y = polyY;
//		v1.color = outerColor;
//		v1.UV = { 0.0f, 0.0f };
//		v1.normal = { 0.0f, 1.0f, 0.0f };
//
//		KdPolygon::Vertex v2;
//		v2.pos = v2W - m_pos;
//		v2.pos.y = polyY;
//		v2.color = outerColor;
//		v2.UV = { 1.0f, 0.0f };
//		v2.normal = { 0.0f, 1.0f, 0.0f };
//
//		vertices.push_back(v0);
//		vertices.push_back(v1);
//		vertices.push_back(v2);
//
//	}
//
//	m_viewPolygon->SetVertices(vertices);
//}

void Enemy::CheckItemSearch()
{
	if (s_showDebugWire)
	{
		m_pDebugWire->AddDebugSphere(m_pos, m_itemSearchArea, kRedColor);
	}

	std::shared_ptr<Item> pFoundItem = nullptr;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (auto itemObj = std::dynamic_pointer_cast<Item>(obj))
		{
			// 1. 手に持たれている場合は無視
			if (itemObj->IsHeld())
			{
				continue;
			}

			// 2. まだ一度も投げられていないアイテム（机から落ちただけなど）は無視
			if (!itemObj->HasBeenThrown())
			{
				continue;
			}

			// ★ 修正：Y軸（高さ）の差を無視して、XZ平面上の距離で索敵する
			Math::Vector3 itemPos = itemObj->GetPos();
			Math::Vector3 vDist = itemPos - m_pos;
			vDist.y = 0.0f; // 高さをゼロにする

			float distXZ = vDist.Length(); // 平面上の距離

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
			m_chaseFlg = false; // アイテム優先
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

// =============================================================
// 描画処理
// =============================================================

void Enemy::GenerateDepthMapFromLight()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}

void Enemy::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);


}

// -------------------------------------------------------------
// 半透明パスでの視界描画
// -------------------------------------------------------------
void Enemy::DrawUnLit()
{

	//if (!m_viewPolygon || m_viewPolygon->GetVertices().empty()) return;

	//KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	//KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	//// ★ m_mWorld ではなく Math::Matrix::Identity を使用
	//KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_viewPolygon, Math::Matrix::Identity);

	//KdShaderManager::Instance().UndoDepthStencilState();
	//KdShaderManager::Instance().UndoBlendState();
}

void Enemy::DrawBright()
{
	if (!m_viewPolygon || m_viewPolygon->GetVertices().empty()) return;

	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	m_viewPolygon->SetColor(Math::Color(1.0f, 0.0f, 0.0f, 0.6f));

	// ★ 単位行列（Identity）を指定して、計算したワールド座標をそのまま描画する
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_viewPolygon, Math::Matrix::Identity);

	KdShaderManager::Instance().UndoDepthStencilState();
	KdShaderManager::Instance().UndoBlendState();


	// 8/25前
	//if (!m_viewPolygon || m_viewPolygon->GetVertices().empty()) return;

	//// ★修正ポイント3：デプステストを「有効」、Z書き込みを「無効」にする
	//// これにより「壁より奥にあるライト」を描画しないようにする
	//// （通常、半透明はZWriteDisableにするが、これはOK。問題はDepthEnableになっているか）
	//// KdDirect3D::Instance().GetDevContext()->OMSetDepthStencilState(...) を直接呼ぶか、
	//// ShaderManagerにそのようなStateがあればそれを使う。
	//// ここでは、一般的な半透明設定（ZWriteのみDisable、テストはEnable）を指定するStateがあると仮定、
	//// または Undo で戻せるように明示的に設定する。

	//// 一般的な3Dエンジンにおける半透明描画の正しいステート：
	//// RasterizerState: CullBack (背面カリング)
	//// BlendState: AlphaBlend
	//// DepthStencilState: DepthEnable=TRUE, DepthWriteMask=ZERO (Z書き込みOFF、テストON)
	//KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
	//KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	//m_viewPolygon->SetColor(Math::Color(1.0f, 0.0f, 0.0f, 0.6f));

	//// ★ 0.5f のスケールをかけて位置を移動させる行列を作成
	//Math::Matrix scaleMat = Math::Matrix::CreateScale(0.5f);
	//Math::Matrix transMat = Math::Matrix::CreateTranslation(m_pos);
	//Math::Matrix polyWorld = scaleMat * transMat;

	//KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_viewPolygon, polyWorld);

	//KdShaderManager::Instance().UndoDepthStencilState();
	//KdShaderManager::Instance().UndoBlendState();
}