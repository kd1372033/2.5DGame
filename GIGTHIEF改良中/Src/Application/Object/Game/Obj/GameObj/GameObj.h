#pragma once

class Furniture : public KdGameObject
{
public:
	Furniture() { Init(); }
	~Furniture() {}

	void Init() override;
	void Update()override;
	void DrawLit()override;

private:
	std::shared_ptr<KdModelData> m_model;
};

