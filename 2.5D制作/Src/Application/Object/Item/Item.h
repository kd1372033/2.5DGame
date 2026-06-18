#pragma once
#include "../../Scene/SceneManager.h"

class Item : public KdGameObject // 基盤のゲームオブジェクトクラスを継承
{
public:

	void Init() override;
	void Update() override;
	void PostUpdate() override;
	void DrawLit() override;

	// =========================================================
	// 初期設定用セッター
	// =========================================================
	void SetPos(const Math::Vector3& pos) { m_pos = pos; } // ★プレイヤー位置をセットするために追加
	void SetDir(Math::Vector3 dir) { m_dir = dir; }

	void StartThrow();

	// 足止め後に消失
	void OnHit() override
	{
		m_isExpired = true; // 自分自身を消滅フラグONにする
	}

private:
	std::shared_ptr<KdSquarePolygon> m_polygon;

	bool          m_isThrown = false; // 投げられている状態か
	Math::Vector3 m_pos = { 0,0,0 };
	Math::Vector3 m_vec = { 0,0,0 };  // 速度（cpp側もこちらに統一します）
	Math::Vector3 m_dir = { 0,0,0 };  // 向きを保持するメンバ変数
	float         m_hitRadius;

	const float gravity = -0.005f;    // 重力
	float m_throwSafetyTimer = 0.0f;
};