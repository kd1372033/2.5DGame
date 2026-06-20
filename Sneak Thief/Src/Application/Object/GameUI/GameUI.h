#pragma once

class GameUI : public KdGameObject
{
public:
	GameUI() { Init(); }
	~GameUI() {}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void DrawSprite()		override;

	// ★追加：外部（GameScene）からフラグを切り替えるための関数
	void SetVisibleJewelry(bool isVisible) { m_isDrawJewelry = isVisible; }


private:
	KdTexture m_arrow;
	KdTexture m_space;
	KdTexture m_jewelryIcon; // ★追加：宝石の画像データ

	// 各キーのアルファ値
	float m_alphaLeft = 0.3f;
	float m_alphaRight = 0.3f;
	float m_alphaUp = 0.3f;
	float m_alphaDown = 0.3f;
	float m_alphaSpace = 0.3f;

	Math::Vector2 m_arrowbasepos;
	Math::Vector2 m_spacebasepos;
	Math::Vector2 m_jewelryPos; // ★追加：宝石UIの配置座標

	Math::Matrix arrowbasemat;
	Math::Matrix spacebasemat;
	Math::Matrix m_jewelryMat;  // ★追加：宝石UI用の行列

	bool m_isDrawJewelry = false; // ★追加：表示フラグ（初期値はfalse）

};