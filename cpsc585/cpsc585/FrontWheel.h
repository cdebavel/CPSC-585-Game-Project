#pragma once

#include "Drawable.h"
#include "Physics.h"


class FrontWheel
{
public:
	FrontWheel(IDirect3DDevice9* device, int filter, float mass);
	~FrontWheel(void);
	void update();
	void setPosAndRot(float posX, float posY, float posZ,
		float rotX, float rotY, float rotZ);	// In Radians
private:


public:
	Drawable* drawable;
	hkpRigidBody* body;

private:


};

