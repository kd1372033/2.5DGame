#pragma once
class TPSCamera;
class Ground;

class Tank : public KdGameObject
{
public:
	Tank() {}
	~Tank()				override {}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void DrawLit()		override;
	void DrawBright()	override;

	void SetCamera(const std::weak_ptr<TPSCamera> camera)
	{
		m_camera = camera;
	}

	bool IsHitWall() const { return m_isHitWall; }

	void SetStage(const std::shared_ptr<Ground>& stage) { m_wpStage = stage; }

private:
	// カメラ情報 ポインタ 
	//参照だけなら weak_ptr で十分
	std::weak_ptr<TPSCamera> m_camera;
	std::weak_ptr<Ground> m_wpStage;

	//モデル
	std::shared_ptr<KdModelData> m_model;

	float m_angle = 0.0f;
	Math::Vector3 m_pos;


private:

	bool          m_isRayFixed = false; // レイが固定されているかどうかのフラグ
	Math::Vector3 m_fixedHitPos;        // 固定された壁の着弾座標

	float m_rayLength = 20.0f; // 初期長さを 20m に設定
	const float m_minRayLength = 1.0f;  // 最小長
	const float m_maxRayLength = 100.0f; // 最大長

	// コーンライト調整用変数
	float m_coneLightRange = 20.0f; // 照射距離（初期値: 20）
	float m_coneLightAngle = 30.0f; // 照射角度（初期値: 30度）

	// 制限値
	const float m_minConeRange = 5.0f;
	const float m_maxConeRange = 200.0f;
	const float m_minConeAngle = 5.0f;
	const float m_maxConeAngle = 120.0f;

	float m_maxLightRange = 20.0f; // ライトの最大距離（キー操作で変化）
	float m_coneAngle = 30.0f; // ライトの開き角度
	bool m_isHitWall = false; // 壁に当たっているかどうか（Hitフラグ）

	// 地面判定用のレイの長さ（自機の中心から下方向へ飛ばす長さ）
	float m_groundRayCheckDist = 2.0f;

	// 重力・接地関連の変数
	float m_gravity = 0.0f;           // 現在の落下速度
	const float m_gravityAccel = 0.001f; // 重力加速度（下方向の加速力）
	const float m_maxGravity = 1.0f;  // 最大落下速度（落下速度の上限）

	bool m_isGrounded = false;        // 接地しているかどうか


	Math::Vector3 m_raypos;
	Math::Vector3 m_tohitvector;
	float m_dynamicangle;
	float m_renderlightdist;
	Math::Vector3 m_searchlightcolor;

	// 3本のレイそれぞれの判定用
	float m_rayDistances[3] = { 0.0f, 0.0f, 0.0f };
	Math::Vector3 m_rayDirs[3];
	bool m_rayHitFlags[3] = { false, false, false };

	Math::Vector3 m_hitCenterPos = Math::Vector3::Zero;	// 着弾点	
	Math::Vector3 m_hitNormal = Math::Vector3::Zero;	// 法線


	// 地面判定処理
	void CheckGroundCollision();
};