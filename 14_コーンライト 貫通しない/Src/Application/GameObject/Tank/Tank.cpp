#include "Tank.h"
#include "../Camera/TPSCamera/TPSCamera.h"
#include "../Ground/Ground.h"
#include "../../Scene/SceneManager.h"

void Tank::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	m_model = std::make_shared<KdModelData>();
	m_model->Load("Asset/Models/tank/tank.gltf");

	KdShaderManager::Instance().WorkAmbientController().SetConeLightEnable(true);

	m_pos = { 0, 5, 0 };
}

void Tank::Update()
{
	Math::Matrix camRotYMat;
	if (m_camera.expired() == false)
	{
		camRotYMat = m_camera.lock()->GetRotationYMatrix();
	}

	Math::Vector3 dir;
	bool moveFlg = false;

	if (GetAsyncKeyState('W') & 0x8000) {
		dir += Math::Vector3::TransformNormal({ 0,0,1 }, camRotYMat);
		moveFlg = true;
	}
	if (GetAsyncKeyState('D') & 0x8000) {
		dir += Math::Vector3::TransformNormal({ 1,0,0 }, camRotYMat);
		moveFlg = true;
	}
	if (GetAsyncKeyState('S') & 0x8000) {
		dir += Math::Vector3::TransformNormal({ 0,0,-1 }, camRotYMat);
		moveFlg = true;
	}
	if (GetAsyncKeyState('A') & 0x8000) {
		dir += Math::Vector3::TransformNormal({ -1,0,0 }, camRotYMat);
		moveFlg = true;
	}

	if (moveFlg == true)
	{
		dir.Normalize();

		Math::Matrix nowRotMat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
		Math::Vector3 nowDir = Math::Vector3::TransformNormal(Math::Vector3(0, 0, 1), nowRotMat);

		Math::Vector3 toDir = dir;

		float dot = nowDir.Dot(toDir);
		float angle = DirectX::XMConvertToDegrees(acos(dot));

		if (angle >= 0.1f)
		{
			if (angle > 5)
			{
				angle = 5;
			}

			Math::Vector3 cross = nowDir.Cross(toDir);
			if (cross.y >= 0.0f) {
				m_angle += angle;
				if (m_angle > 360) { m_angle -= 360; }
			}
			else {
				m_angle -= angle;
				if (m_angle < 0) { m_angle += 360; }
			}
		}

		KdDebugGUI::Instance().AddLog("%f\n", m_angle);
		m_pos += dir * 0.3f;
	}

	if (!m_isGrounded)
	{
		m_gravity += m_gravityAccel;
		if (m_gravity > m_maxGravity)
		{
			m_gravity = m_maxGravity;
		}
	}
	m_pos.y -= m_gravity;

	Math::Matrix rotMat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
	Math::Matrix transMat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = rotMat * transMat;

	CheckGroundCollision();

	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		m_isRayFixed = false;
	}

	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		m_maxLightRange += 0.5f;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		m_maxLightRange -= 0.5f;
	}
	m_maxLightRange = std::clamp(m_maxLightRange, 1.0f, 100.0f);

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		m_coneLightAngle += 0.5f;
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		m_coneLightAngle -= 0.5f;
	}
	m_coneLightAngle = std::clamp(m_coneLightAngle, m_minConeAngle, m_maxConeAngle);
}

void Tank::PostUpdate()
{
	Math::Vector3 rayPos = m_pos + Math::Vector3(0, 1.2f, 0) + (m_mWorld.Backward() * 0.8f);
	Math::Vector3 rayDir = m_mWorld.Backward();
	float maxRayDist = m_maxLightRange;

	float halfAngleRad = DirectX::XMConvertToRadians(m_coneLightAngle * 0.5f);

	Math::Matrix leftRot = Math::Matrix::CreateRotationY(-halfAngleRad);
	Math::Matrix rightRot = Math::Matrix::CreateRotationY(halfAngleRad);

	m_rayDirs[0] = rayDir;                                           // 中央
	m_rayDirs[1] = Math::Vector3::TransformNormal(rayDir, leftRot);  // 左
	m_rayDirs[2] = Math::Vector3::TransformNormal(rayDir, rightRot); // 右

	m_isHitWall = false;

	Math::Vector3 hitPositions[3];
	Math::Vector3 hitNormals[3];
	Math::Vector3 reflectDirs[3];
	float remainDistances[3] = { 0.0f, 0.0f, 0.0f };

	float maxHitDist = 0.0f;

	if (auto ground = m_wpStage.lock())
	{
		for (int i = 0; i < 3; ++i)
		{
			m_rayDistances[i] = maxRayDist;
			m_rayHitFlags[i] = false;

			KdCollider::RayInfo rayInfo(
				KdCollider::TypeGround | KdCollider::TypeBump,
				rayPos,
				m_rayDirs[i],
				maxRayDist
			);

			std::list<KdCollider::CollisionResult> retHitList;
			ground->Intersects(rayInfo, &retHitList);

			for (const auto& result : retHitList)
			{
				if (result.m_overlapDistance < 0.3f) { continue; } // 自機判定除外

				if (result.m_overlapDistance < m_rayDistances[i])
				{
					m_rayDistances[i] = result.m_overlapDistance;
					m_rayHitFlags[i] = true;
					hitPositions[i] = result.m_hitPos;
					hitNormals[i] = result.m_hitDir;

					remainDistances[i] = maxRayDist - result.m_overlapDistance;
				}
			}

			if (m_rayHitFlags[i])
			{
				m_isHitWall = true;
				if (m_rayDistances[i] > maxHitDist)
				{
					maxHitDist = m_rayDistances[i];
				}
			}
		}
	}

	// --- レイの着弾点に基づく中心位置（m_hitCenterPos）の決定 ---
	if (m_rayHitFlags[0])
	{
		// 1. 中央レイが壁に当たっている場合
		Math::Vector3 baseHitPos = hitPositions[0];

		float leftDist = m_rayHitFlags[1] ? m_rayDistances[1] : maxRayDist;
		float rightDist = m_rayHitFlags[2] ? m_rayDistances[2] : maxRayDist;
		float distDiff = rightDist - leftDist;

		// 壁面に沿った横方向ベクトルを計算
		Math::Vector3 wallTangent = hitNormals[0].Cross(Math::Vector3::Up);
		if (wallTangent.LengthSquared() < 0.001f)
		{
			wallTangent = hitNormals[0].Cross(Math::Vector3::Forward);
		}
		wallTangent.Normalize();

		if (rayDir.Dot(wallTangent) < 0.0f)
		{
			wallTangent = -wallTangent;
		}

		// 中央ヒット時の微調整
		m_hitCenterPos = baseHitPos - wallTangent * (distDiff * 0.15f);
		m_hitNormal = hitNormals[0];
	}
	else if (m_rayHitFlags[2])
	{
		// 2. 中央は外し、右レイのみが壁に当たっている場合（画像黒塗りのケース）
		Math::Vector3 wallTangent = hitNormals[2].Cross(Math::Vector3::Up);
		if (wallTangent.LengthSquared() < 0.001f)
		{
			wallTangent = hitNormals[2].Cross(Math::Vector3::Forward);
		}
		wallTangent.Normalize();

		// 壁の奥（照射進行方向）に揃える
		if (rayDir.Dot(wallTangent) < 0.0f)
		{
			wallTangent = -wallTangent;
		}

		float spotRadius = m_rayDistances[2] * tanf(halfAngleRad);

		// 右レイの着弾点から「壁の奥（左側・内側）」へ向かって半径分引き戻す
		m_hitCenterPos = hitPositions[2] - wallTangent * spotRadius;
		m_hitNormal = hitNormals[2];
	}
	else if (m_rayHitFlags[1])
	{
		// 3. 中央は外し、左レイのみが壁に当たっている場合
		Math::Vector3 wallTangent = hitNormals[1].Cross(Math::Vector3::Up);
		if (wallTangent.LengthSquared() < 0.001f)
		{
			wallTangent = hitNormals[1].Cross(Math::Vector3::Forward);
		}
		wallTangent.Normalize();

		if (rayDir.Dot(wallTangent) < 0.0f)
		{
			wallTangent = -wallTangent;
		}

		float spotRadius = m_rayDistances[1] * tanf(halfAngleRad);

		// 左レイの着弾点から「壁の奥（右側・内側）」へ向かって半径分プラスする
		m_hitCenterPos = hitPositions[1] + wallTangent * spotRadius;
		m_hitNormal = hitNormals[1];
	}
	else
	{
		// 4. 一本も当たっていない場合
		m_hitCenterPos = rayPos + rayDir * maxRayDist;
		m_hitNormal = -rayDir;
	}

	m_hitNormal.Normalize();

	for (int i = 0; i < 3; ++i)
	{
		if (m_rayHitFlags[i])
		{
			reflectDirs[i] = Math::Vector3::Reflect(m_rayDirs[i], hitNormals[i]);
			reflectDirs[i].Normalize();
		}
	}

	float renderLightDist = maxRayDist;

	// --- デバッグ描画 ---
	if (m_pDebugWire)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (m_rayHitFlags[i])
			{
				m_pDebugWire->AddDebugLine(rayPos, hitPositions[i], kGreenColor);
				Math::Vector3 reflectEndPos = hitPositions[i] + (reflectDirs[i] * remainDistances[i]);
				m_pDebugWire->AddDebugLine(hitPositions[i], reflectEndPos, kBlueColor);
			}
			else
			{
				m_pDebugWire->AddDebugLine(rayPos, rayPos + (m_rayDirs[i] * maxRayDist), kRedColor);
			}
		}
	}

	// 描画データセット
	m_raypos = rayPos;
	m_tohitvector = rayDir;
	m_dynamicangle = m_coneLightAngle;
	m_renderlightdist = renderLightDist;
	m_searchlightcolor = { 10.0f, 0.0f, 0.0f };
}

void Tank::DrawLit()
{
	// 2026/09/14 修正: 遮蔽パラメータ（m_isHitWall, 着弾座標, 着弾法線）をシェーダーへ渡す
	KdShaderManager::Instance().WriteCBConeLight(
		m_raypos,
		m_tohitvector,
		DirectX::XMConvertToRadians(m_dynamicangle),
		m_renderlightdist,
		m_searchlightcolor,
		m_isHitWall,
		m_hitCenterPos,
		m_hitNormal
	);
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
}

void Tank::DrawBright()
{

}

void Tank::CheckGroundCollision()
{
	Math::Vector3 rayPos = m_pos + Math::Vector3(0.0f, 2.0f, 0.0f);
	Math::Vector3 rayDir = -Math::Vector3::Up;
	float rayDist = 5.0f;

	KdCollider::RayInfo rayInfo(
		KdCollider::TypeGround,
		rayPos,
		rayDir,
		rayDist
	);

	std::list<KdCollider::CollisionResult> retHitList;

	if (auto ground = m_wpStage.lock())
	{
		ground->Intersects(rayInfo, &retHitList);
	}

	float minDist = rayDist;
	m_isGrounded = false;

	for (const auto& result : retHitList)
	{
		if (result.m_overlapDistance < minDist)
		{
			minDist = result.m_overlapDistance;

			m_pos.y = result.m_hitPos.y;
			m_isGrounded = true;
		}
	}

	if (m_isGrounded)
	{
		m_gravity = 0.0f;

		Math::Matrix rotMat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
		Math::Matrix transMat = Math::Matrix::CreateTranslation(m_pos);
		m_mWorld = rotMat * transMat;
	}
}