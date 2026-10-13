#include "KdAmbientController.h"
#include "KdShaderManager.h"

KdAmbientController::KdAmbientController()
{
	m_spConeLight = std::make_unique<ConeLight>();
}

KdAmbientController::~KdAmbientController()
{
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// シェーダーマネージャで設定したシェーダーの初期値を取得してくる
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::Init()
{
	// 光源のパラメータ
	{
		const KdShaderManager::cbLight& cbLight = KdShaderManager::Instance().GetLightCB();

		m_parameter.m_ambientLightColor = cbLight.AmbientLight;

		m_parameter.m_directionalLightDir = cbLight.DirLight_Dir;
		m_parameter.m_directionalLightDir.Normalize();

		m_parameter.m_directionalLightColor = cbLight.DirLight_Color;
	}

	// フォグのパラメータ
	{
		const KdShaderManager::cbFog& cbFog = KdShaderManager::Instance().GetFogCB();

		m_parameter.m_distanceFogColor = cbFog.DistanceFogColor;
		m_parameter.m_distanceFogDensity = cbFog.DistanceFogDensity;

		m_parameter.m_heightFogColor = cbFog.HeightFogColor;
		m_parameter.m_heightFogTopValue = cbFog.HeightFogTopValue;
		m_parameter.m_heightFogBottomValue = cbFog.HeightFogBottomValue;
		m_parameter.m_heightFogBeginDistance = cbFog.HeightFogBeginDistance;
	}

	// 影用深度バッファ生成情報の初期化
	SetDirLightShadowArea({ 25.0f, 25.0f }, 30.0f);

	// 光のパラメータの反映
	WriteLightParams();

	// フォグのパラメータの反映
	WriteFogParams();
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 点光源・コーンライトは毎フレーム登録する形なので0クリアする
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::Update()
{
	m_pointLights.clear();

	// 毎フレームリセット
	m_coneLights.clear();
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 描画の直前の更新が終わったタイミングで実施
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::Draw()
{
	// 光のパラメータの反映
	WriteLightParams();

	// フォグのパラメータの反映
	WriteFogParams();
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// ポイントライトの追加
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::AddPointLight(const Math::Vector3& Color, float Radius, const Math::Vector3& Pos, bool IsBright)
{
	m_pointLights.push_back(PointLight(Color, Radius, Pos, IsBright));
}

void KdAmbientController::AddPointLight(const PointLight& pointLight)
{
	m_pointLights.push_back(pointLight);
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// コーンライトの設定
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::SetConeLight(
	const Math::Vector3& pos,
	const Math::Vector3& dir,
	float angle,
	float range,
	const Math::Vector3& col,
	bool isHit,
	const Math::Vector3& hitPos,
	const Math::Vector3& hitNormal)
{
	if (!m_spConeLight) { m_spConeLight = std::make_unique<ConeLight>(); }

	m_spConeLight->Pos = pos;
	m_spConeLight->Dir = dir;
	m_spConeLight->Angle = angle;
	m_spConeLight->Range = range;
	m_spConeLight->Color = col;
	m_spConeLight->Intensity = 1.0f;
	m_spConeLight->EdgeSmoothness = 0.5f; // ★ 削除またはコメントアウト

	m_spConeLight->IsHit = isHit ? 1 : 0;
	m_spConeLight->HitPos = hitPos;
	m_spConeLight->HitNormal = hitNormal;
}

void KdAmbientController::SetConeLight(const ConeLight& coneLight)
{
	if (!m_spConeLight) { m_spConeLight = std::make_unique<ConeLight>(); }
	*m_spConeLight = coneLight;
}

void KdAmbientController::AddConeLight(const ConeLight& coneLight)
{
	if (m_coneLights.size() < kMaxConeLightCount)
	{
		m_coneLights.push_back(coneLight);
	}
}

void KdAmbientController::SetConeLightEnable(bool enable)
{
	KdShaderManager::Instance().WriteCBConeLightEnable(enable);
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 平行光影の生成エリアの設定
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::SetDirLightShadowArea(const Math::Vector2& lightingArea, float dirLightHeight)
{
	m_shadowProj = DirectX::XMMatrixOrthographicLH(lightingArea.x, lightingArea.y, 0, dirLightHeight * 2.0f);
	m_dirLightHeight = dirLightHeight;
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 平行光の設定
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::SetDirLight(const Math::Vector3& dir, const Math::Vector3& col)
{
	m_parameter.m_directionalLightDir = dir;
	m_parameter.m_directionalLightDir.Normalize();

	m_parameter.m_directionalLightColor = col;

	m_dirtyLightDir = true;
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 環境光の設定
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::SetAmbientLight(const Math::Vector4& col)
{
	m_parameter.m_ambientLightColor = col;

	m_dirtyLightAmb = true;
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// フォグの有効/無効をそれぞれ切り替える
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::SetFogEnable(bool distance, bool height)
{
	KdShaderManager::Instance().WriteCBFogEnable(distance, height);
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 距離フォグの設定
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::SetDistanceFog(const Math::Vector3& col, float density)
{
	m_parameter.m_distanceFogColor = col;
	m_parameter.m_distanceFogDensity = density;

	m_dirtyFogDist = true;
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 高さフォグの設定
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::SetheightFog(const Math::Vector3& col, float topValue, float bottomValue, float distance)
{
	m_parameter.m_heightFogColor = col;
	m_parameter.m_heightFogTopValue = topValue;
	m_parameter.m_heightFogBottomValue = bottomValue;
	m_parameter.m_heightFogBeginDistance = distance;

	m_dirtyFogHeight = true;
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// 光関連パラメータの書き込み（毎フレーム描画直前に呼ばれる）
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::WriteLightParams()
{
	// 環境光
	if (m_dirtyLightAmb)
	{
		KdShaderManager::Instance().WriteCBAmbientLight(m_parameter.m_ambientLightColor);
		m_dirtyLightAmb = false;
	}

	// 平行光
	if (m_dirtyLightDir)
	{
		KdShaderManager::Instance().WriteCBDirectionalLight(
			m_parameter.m_directionalLightDir, m_parameter.m_directionalLightColor);
		m_dirtyLightDir = false;
	}

	// 点光源
	KdShaderManager::Instance().WriteCBPointLight(m_pointLights);

	// ★ 複数コーンライトの一括転送 (m_coneLights の配列を転送)
	KdShaderManager::Instance().WriteCBConeLight(m_coneLights);

	// 影描画エリアの更新
	KdShaderManager::Instance().WriteCBShadowArea(m_shadowProj, m_dirLightHeight);
}

// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
// フォグ関連パラメータの書き込み
// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////
void KdAmbientController::WriteFogParams()
{
	// 距離フォグ
	if (m_dirtyFogDist)
	{
		KdShaderManager::Instance().WriteCBDistanceFog(m_parameter.m_distanceFogColor,
			m_parameter.m_distanceFogDensity);

		m_dirtyFogDist = false;
	}

	// 高さフォグ
	if (m_dirtyFogHeight)
	{
		KdShaderManager::Instance().WriteCBHeightFog(m_parameter.m_heightFogColor,
			m_parameter.m_heightFogTopValue, m_parameter.m_heightFogBottomValue,
			m_parameter.m_heightFogBeginDistance);

		m_dirtyFogHeight = false;
	}
}