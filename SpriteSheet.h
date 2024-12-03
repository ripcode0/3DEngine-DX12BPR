#pragma once

#include <vector>
#include "Sprite.h"

class SpriteSheet
{
private:
	std::vector<Sprite> sheet;
	unsigned int width;
	unsigned int height;

public:
	SpriteSheet();
	~SpriteSheet();
	SpriteSheet(const SpriteSheet& that);
	SpriteSheet& operator=(const SpriteSheet& that);

	SpriteSheet(const std::vector<Sprite>& s, unsigned int w, unsigned int h, unsigned int id);

	std::vector<Sprite> GetSheet() const;
	unsigned int GetWidth() const;
	unsigned int GetHeight() const;

	void SetSheet(std::vector<Sprite> s);
	void SetWidth(unsigned int w);
	void SetHeight(unsigned int h);
	void SetDimensions(unsigned int w, unsigned int h);
};