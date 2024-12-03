#pragma once

#include <vector>
#include <fstream>
#include <string>
#include "libraries/gateware/gateware.h"
#include "libraries/tinyxml2/tinyxml2.h"

struct SPRITE_VERTEX
{
	POINTF position;
	POINTF uvs;

	SPRITE_VERTEX(FLOAT x, FLOAT y, FLOAT u, FLOAT v);
};

SPRITE_VERTEX::SPRITE_VERTEX(FLOAT x, FLOAT y, FLOAT u, FLOAT v) :
	position({ x,y }), uvs({ u,v })
{
}

struct SPRITE_RECT
{
	POINTF minimum, maximum;
};

struct SPRITE_DATA
{
	POINTF translation;
	POINTF scale;
	FLOAT rotation, depth;
};

class SPRITE
{
public:
	std::string					name;
	SPRITE_DATA					data;
	SPRITE_RECT					scissor;
	UINT						texture_id;

public:

	SPRITE();
	~SPRITE() = default;
	SPRITE(const SPRITE& c) = default;
	SPRITE& operator=(const SPRITE& c) = default;
	SPRITE(std::string name, SPRITE_DATA data, SPRITE_RECT scissor, UINT id);
};

SPRITE::SPRITE() :
	name(""), data({ { 0.0f, 0.0f }, { 1.0f, 1.0f }, 0.0f, 0.1f }), scissor({ 0, 0, 1024, 1024 }), texture_id(-1)
{
}

SPRITE::SPRITE(std::string name, SPRITE_DATA data, SPRITE_RECT scissor, UINT id) :
	name(name), data(data), scissor(scissor), texture_id(id)
{
}

struct SPRITE_CHARACTER
{
public:
	char				letter;
	UINT				x;
	UINT				y;
	UINT				width;
	UINT				height;
	UINT				origin_x;
	UINT				origin_y;
	UINT				advance;

};

struct SPRITE_FONT
{
public:
	std::string						name;
	UINT							size;
	BOOL							bold;
	BOOL							italic;
	UINT							width;
	UINT							height;
	std::vector<SPRITE_CHARACTER>	letters;

};

class SPRITE_TEXT : public SPRITE
{
public:
	std::string					text;
	std::vector<SPRITE_VERTEX>	vertices;
	SPRITE_FONT					font;

public:
	SPRITE_TEXT() = default;
	~SPRITE_TEXT() = default;
	SPRITE_TEXT(const SPRITE_TEXT& c) = default;
	SPRITE_TEXT& operator=(const SPRITE_TEXT& c) = default;

	void SetText(std::string text);
	BOOL LoadFontDataFromFile(std::string font_xml);

private:
	POINTF SCREENtoNDC(POINTF ndc, POINTF screen);
};

BOOL SPRITE_TEXT::LoadFontDataFromFile(std::string font_xml)
{
	tinyxml2::XMLDocument document;
	tinyxml2::XMLError error = document.LoadFile(font_xml.c_str());
	if (error != tinyxml2::XML_SUCCESS)
	{
		std::cout << "Error loading data from: " << font_xml.c_str() << std::endl;
		return FALSE;
	}

	font.name = document.FirstChildElement("font")->FindAttribute("name")->Value();
	font.size = atoi(document.FirstChildElement("font")->FindAttribute("size")->Value());
	font.bold = (strcmp(document.FirstChildElement("font")->FindAttribute("bold")->Value(), "false") == 0) ? FALSE : TRUE;
	font.italic = (strcmp(document.FirstChildElement("font")->FindAttribute("italic")->Value(), "false") == 0) ? FALSE : TRUE;
	font.width = atoi(document.FirstChildElement("font")->FindAttribute("width")->Value());
	font.height = atoi(document.FirstChildElement("font")->FindAttribute("height")->Value());

	tinyxml2::XMLElement* current = document.FirstChildElement("font")->FirstChildElement("character");

	while (current)
	{
		SPRITE_CHARACTER c;
		c.letter = *current->FindAttribute("text")->Value();
		c.x = atoi(current->FindAttribute("x")->Value());
		c.y = atoi(current->FindAttribute("y")->Value());
		c.width = atoi(current->FindAttribute("width")->Value());
		c.height = atoi(current->FindAttribute("height")->Value());
		c.origin_x = atoi(current->FindAttribute("origin-x")->Value());
		c.origin_y = atoi(current->FindAttribute("origin-y")->Value());
		c.advance = atoi(current->FindAttribute("advance")->Value());
		font.letters.push_back(c);
		current = current->NextSiblingElement();
	}

	document.Clear();

	return TRUE;
}

POINTF SPRITE_TEXT::SCREENtoNDC(POINTF ndc, POINTF screen)
{
	POINTF result = POINTF();
	result.x = (ndc.x / screen.x) * 2.0f - 1.0f;
	result.y = (1.0f - (ndc.y / screen.y)) * 2.0f - 1.0f;
	return result;
}

void SPRITE_TEXT::SetText(std::string text)
{
	this->text = text;
	vertices.clear();

	if (!font.letters.size())
	{
		return;
	}

	size_t kerning = 1;
	size_t text_length = text.size();
	size_t total_advance = 0;
	for (size_t i = 0; i < text_length; i++)
	{
		total_advance += font.letters[static_cast<size_t>(text[i] - ' ')].advance + kerning;
	}
	POINTF screen_size = { 1024, 768 };

	float x = (screen_size.x / 2.0f) + (-1.0f * static_cast<float>(total_advance) / 2.0f);
	float y = (screen_size.y / 2.0f) - (font.size / 2.0f);


	for (size_t i = 0; i < text_length; i++)
	{
		const SPRITE_CHARACTER& c = font.letters[static_cast<size_t>(text[i] - ' ')];

		SPRITE_VERTEX p0 = SPRITE_VERTEX(
			x - c.origin_x,
			y - c.origin_y,
			(FLOAT)c.x / (FLOAT)font.width,
			(FLOAT)c.y / (FLOAT)font.height
		);
		SPRITE_VERTEX p1 = SPRITE_VERTEX(
			x - c.origin_x + c.width,
			y - c.origin_y,
			((FLOAT)c.x + (FLOAT)c.width) / (FLOAT)font.width,
			(FLOAT)c.y / (FLOAT)font.height
		);
		SPRITE_VERTEX p2 = SPRITE_VERTEX(
			x - c.origin_x,
			y - c.origin_y + c.height,
			(FLOAT)c.x / (FLOAT)font.width,
			((FLOAT)c.y + (FLOAT)c.height) / (FLOAT)font.height
		);
		SPRITE_VERTEX p3 = SPRITE_VERTEX(
			x - c.origin_x + c.width,
			y - c.origin_y + c.height,
			((FLOAT)c.x + (FLOAT)c.width) / (FLOAT)font.width,
			((FLOAT)c.y + (FLOAT)c.height) / (FLOAT)font.height
		);

		p0.position = SCREENtoNDC(p0.position, screen_size);
		p1.position = SCREENtoNDC(p1.position, screen_size);
		p2.position = SCREENtoNDC(p2.position, screen_size);
		p3.position = SCREENtoNDC(p3.position, screen_size);

		vertices.push_back(p2);
		vertices.push_back(p0);
		vertices.push_back(p3);
		vertices.push_back(p1);

		x += c.advance + kerning;
	}
}

class HUD
{
public:
	std::vector<SPRITE_VERTEX>	vertices;
	std::vector<SPRITE>			sprites;
	std::vector<SPRITE_TEXT>	text;

public:

	HUD();
	~HUD() = default;
	HUD(const HUD& c) = default;
	HUD& operator=(const HUD& c) = default;

	void AddSprite(const SPRITE& sprite);
	void AddSpriteText(const SPRITE_TEXT& font);
	void Update();
};

HUD::HUD()
{
	SPRITE_VERTEX quad[] =
	{
		{ -1.0f,  1.0f, 0.0f, 0.0f },
		{  1.0f,  1.0f, 1.0f, 0.0f },
		{ -1.0f, -1.0f, 0.0f, 1.0f },
		{  1.0f, -1.0f, 1.0f, 1.0f }
	};
	vertices.insert(vertices.end(), quad, quad + ARRAYSIZE(quad));
}

void HUD::AddSprite(const SPRITE& sprite)
{
	sprites.push_back(sprite);
}

void HUD::AddSpriteText(const SPRITE_TEXT& font)
{
	text.push_back(font);
	vertices.insert(vertices.end(), font.vertices.begin(), font.vertices.end());
}

void HUD::Update()
{
	vertices.clear();
	SPRITE_VERTEX quad[] =
	{
		{ -1.0f,  1.0f, 0.0f, 0.0f },
		{  1.0f,  1.0f, 1.0f, 0.0f },
		{ -1.0f, -1.0f, 0.0f, 1.0f },
		{  1.0f, -1.0f, 1.0f, 1.0f }
	};
	vertices.insert(vertices.end(), quad, quad + ARRAYSIZE(quad));
	for (size_t i = 0; i < text.size(); i++)
	{
		vertices.insert(vertices.end(), text[i].vertices.begin(), text[i].vertices.end());
	}
}