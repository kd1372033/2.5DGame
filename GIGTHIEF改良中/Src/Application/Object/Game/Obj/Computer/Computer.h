#pragma once

class Jewelry;
class Player; // ★ Player クラスの前方宣言を追加

class Computer : public KdGameObject
{
public:
	Computer() { Init(); }
	virtual ~Computer() override = default;

	void Init() override;
	void Update() override;
	void PostUpdate() override;
	void DrawLit() override;

	// 回転角度設定
	void SetRotY(float angleY) { m_rotY = angleY; }

	// バリアの判定半径を設定（デフォルト: 1.5f）
	void SetBarrierRadius(float radius) { m_barrierRadius = radius; }

	// 紐付ける宝石（Jewelry）を設定
	void SetJewelry(const std::shared_ptr<Jewelry>& spJewelry) { m_wpJewelry = spJewelry; }

	// ★ プレイヤーを設定（UI表示距離判定用）
	void SetPlayer(const std::shared_ptr<Player>& spPlayer) { m_wpPlayer = spPlayer; }

	// バリア・破壊状態のゲッター
	bool IsBroken() const { return m_isBroken; }

private:
	std::shared_ptr<KdModelData> m_pcNormalModel; // 壊れていないPC
	std::shared_ptr<KdModelData> m_pcBrokenModel; // 壊れたPC

	std::weak_ptr<Jewelry> m_wpJewelry;
	std::weak_ptr<Player>  m_wpPlayer; // ★ プレイヤーの参照を保持

	float m_rotY = 270.0f;
	float m_barrierRadius = 1.5f; // 円形バリアの半径
	bool  m_isBroken = false;     // PCが壊れたか

	std::shared_ptr<KdSquarePolygon> m_spChoicePoly; // Choice.png用の四角形ポリゴン
	bool  m_isNearPlayer = false;                    // プレイヤー接近フラグ
	float m_interactDistance = 0.5f;                 // 反応距離
};