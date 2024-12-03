#include "SpriteSheet.h"

SpriteSheet::SpriteSheet()
{
	sheet.clear();
	width = 0;
	height = 0;
}

SpriteSheet::~SpriteSheet()
{
	sheet.clear();
	width = 0;
	height = 0;
}

SpriteSheet::SpriteSheet(const SpriteSheet& that)
{
	*this = that;
}

SpriteSheet& SpriteSheet::operator=(const SpriteSheet& that)
{
	if (this != &that)
	{
		sheet = that.sheet;
		width = that.width;
		height = that.height;
	}
	return *this;
}

SpriteSheet::SpriteSheet(const std::vector<Sprite>& s, unsigned int w, unsigned int h, unsigned int id)
{
	sheet = s;
	width = w;
	height = h;
}

std::vector<Sprite> SpriteSheet::GetSheet() const
{
	return sheet;
}

unsigned int SpriteSheet::GetWidth() const
{
	return width;
}

unsigned int SpriteSheet::GetHeight() const
{
	return height;
}

void SpriteSheet::SetSheet(std::vector<Sprite> s)
{
	sheet = s;
}

void SpriteSheet::SetWidth(unsigned int w)
{
	width = w;
}

void SpriteSheet::SetHeight(unsigned int h)
{
	height = h;
}

void SpriteSheet::SetDimensions(unsigned int w, unsigned int h)
{
	width = w; height = h;
}
