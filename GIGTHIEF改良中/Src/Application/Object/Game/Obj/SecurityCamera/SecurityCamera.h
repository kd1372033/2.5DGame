//#pragma once
//
//class Player;
//
//class SecurityCamera : public KdGameObject
//{
//public:
//	SecurityCamera() { Init(); }
//	virtual ~SecurityCamera() override = default;
//
//	void Init() override;
//	void Update() override;
//	void PostUpdate() override;
//	void DrawUnLit() override; // 半透明カラースフィアの描画
//
//	// プレイヤーの参照をセット
//	void SetPlayer(const std::shared_ptr<Player>& spPlayer) { m_wpPlayer = spPlayer; }
//
//	// スフィアの位置・半径設定
//	void SetSphereArea(const Math::Vector3& pos, float radius)
//	{
//		m_pos = pos;
//		m_detectionRadius = radius;
//	}
//
//private:
//	void CheckPlayerDetection();
//
//	std::weak_ptr<Player> m_wpPlayer;
//
//	float m_detectionRadius = 1.5f; // 監視スフィアの半径
//	bool  m_isPlayerDetected = false;
//
//	// スフィアのカラー（通常時: 黄色半透明 / 発見時: 赤色半透明）
//	Math::Color m_normalColor = { 1.0f, 0.8f, 0.0f, 0.35f };
//	Math::Color m_alertColor = { 1.0f, 0.0f, 0.0f, 0.5f };
//};