//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Dear ImGui code headers for Director's Cut
//
// $NoKeywords: $
//=============================================================================//

#ifndef _IMGUI_PUBLIC_H_
#define _IMGUI_PUBLIC_H_

#include "imgui.h"

#undef RegSetValueEx
#undef RegSetValue
#undef RegQueryValueEx
#undef RegQueryValue
#undef RegOpenKeyEx
#undef RegOpenKey
#undef RegCreateKeyEx
#undef RegCreateKey
#undef ReadConsoleInput
#undef INVALID_HANDLE_VALUE
#undef GetCommandLine

#include "imguizmo/imguizmo.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <string>

void FPU_MatrixF_x_MatrixF(const float* a, const float* b, float* r);

namespace ImGui
{
	// ImGui::InputText() with std::string
	// Because text input needs dynamic resizing, we need to setup a callback to grow the capacity
	IMGUI_API bool  InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	IMGUI_API bool  InputTextMultiline(const char* label, std::string* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	IMGUI_API bool  InputTextWithHint(const char* label, const char* hint, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
}

struct matrix_t_dx;

struct vec_t_dx
{
public:
	float x, y, z, w;

	void Lerp(const vec_t_dx& v, float t);

	void Set(float v) { x = y = z = w = v; }
	void Set(float _x, float _y, float _z = 0.f, float _w = 0.f) { x = _x; y = _y; z = _z; w = _w; }

	vec_t_dx& operator -= (const vec_t_dx& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	vec_t_dx& operator += (const vec_t_dx& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	vec_t_dx& operator *= (const vec_t_dx& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
	vec_t_dx& operator *= (float v) { x *= v;    y *= v;    z *= v;    w *= v;    return *this; }

	vec_t_dx operator * (float f) const;
	vec_t_dx operator - () const;
	vec_t_dx operator - (const vec_t_dx& v) const;
	vec_t_dx operator + (const vec_t_dx& v) const;
	vec_t_dx operator * (const vec_t_dx& v) const;

	const vec_t_dx& operator + () const { return (*this); }
	float Length() const;
	float LengthSq() const;
	vec_t_dx Normalize();
	vec_t_dx Normalize(const vec_t_dx& v);
	vec_t_dx Abs() const;

	void Cross(const vec_t_dx& v);

	void Cross(const vec_t_dx& v1, const vec_t_dx& v2);

	float Dot(const vec_t_dx& v) const;

	float Dot3(const vec_t_dx& v) const;

	void Transform(const matrix_t_dx& matrix);
	void Transform(const vec_t_dx& s, const matrix_t_dx& matrix);

	void TransformVector(const matrix_t_dx& matrix);
	void TransformPoint(const matrix_t_dx& matrix);
	void TransformVector(const vec_t_dx& v, const matrix_t_dx& matrix);
	void TransformPoint(const vec_t_dx& v, const matrix_t_dx& matrix);

	float& operator [] (size_t index) { return ((float*)&x)[index]; }
	const float& operator [] (size_t index) const { return ((float*)&x)[index]; }
	bool operator!=(const vec_t_dx& other) const { return memcmp(this, &other, sizeof(vec_t_dx)) != 0; }
};

struct matrix_t_dx
{
public:

	union
	{
		float m[4][4];
		float m16[16];
		struct
		{
			vec_t_dx right, up, dir, position;
		} v;
		vec_t_dx component[4];
	};

	operator float* () { return m16; }
	operator const float* () const { return m16; }
	void Translation(float _x, float _y, float _z);

	void Translation(const vec_t_dx& vt);

	void Scale(float _x, float _y, float _z);
	void Scale(const vec_t_dx& s);

	matrix_t_dx& operator *= (const matrix_t_dx& mat)
	{
		matrix_t_dx tmpMat;
		tmpMat = *this;
		tmpMat.Multiply(mat);
		*this = tmpMat;
		return *this;
	}
	matrix_t_dx operator * (const matrix_t_dx& mat) const
	{
		matrix_t_dx matT;
		matT.Multiply(*this, mat);
		return matT;
	}
	
	void Multiply(const matrix_t_dx& matrix);

	void Multiply(const matrix_t_dx& m1, const matrix_t_dx& m2);

	float GetDeterminant() const;
	
	float Inverse(const matrix_t_dx& srcMatrix, bool affine = false);
	void SetToIdentity();
	void Transpose();

	void RotationAxis(const vec_t_dx& axis, float angle);

	void OrthoNormalize();
};


#endif
