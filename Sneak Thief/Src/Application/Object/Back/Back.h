#pragma once

class Back : public KdGameObject
{
public:
	Back() { Init(); }
	~Back() {}

	void Init() override;
	void DrawLit() override;

private:

	std::shared_ptr<KdSquarePolygon> m_polygon;
};