#pragma once

class Ground : public KdGameObject
{
public:
	Ground() {}
	~Ground()override {}

	void Init()override;
	void DrawLit()override;

	const std::unique_ptr<KdCollider>& GetCollider() const { return m_pCollider; }
public:
	// 当たり判定実行用の関数を追加
	bool Intersects(const KdCollider::RayInfo& rayInfo, std::list<KdCollider::CollisionResult>* pResults);

private:
	//モデル
	std::shared_ptr<KdModelData> m_model;

	Math::Matrix m_wallWorld; // ★追加：壁用のワールド行列

	// 地面用と壁用でコライダーを分ける
	std::unique_ptr<KdCollider> m_pCollider;     // 地面用
	std::unique_ptr<KdCollider> m_pWallCollider; // 壁用 ★追加
};