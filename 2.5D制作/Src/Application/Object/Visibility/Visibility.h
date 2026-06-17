#pragma once

class Visibility : public KdGameObject
{
public:
	Visibility() { Init(); }
	~Visibility() {}

	void Init() override;
	void Update() override;
	void PostUpdate() override;
	void DrawLit() override;

	void SetDeg(float deg) { m_deg = deg; }

private:

	std::shared_ptr<KdModelData> m_model;
	
	float m_deg;
};