////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: 박창현 Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>


IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;
int score = 0;
int interSectedCount = 0;
const int NUM_OF_SPHERE = 8;
// There are four balls
// initialize the position (coordinate) of each ball (ball0 ~ ball3)
const float spherePos[NUM_OF_SPHERE][2] = { {-0.7f,-0.0f}  , {1.2f, 2.3f}, {1.2f, 0.0f},{ -0.8f,-0.9f },{1.2f, -2.3f}, {3.2f, 2.3f }, {3.2f, 0.0f}, {3.2f, -2.3f} };
// initialize the color of each ball (ball0 ~ ball3)
const D3DXCOLOR sphereColor[NUM_OF_SPHERE] = { d3d::MAGENTA,  d3d::YELLOW, d3d::YELLOW, d3d::WHITE, d3d::YELLOW, d3d::BLUE,d3d::BLUE,d3d::BLUE };

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

class CWall;
// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
	float					center_x, center_y, center_z;
	float                   m_radius;
	float					m_velocity_x;
	float					m_velocity_z;

public:
	CSphere(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_radius = 0;
		m_velocity_x = 0;
		m_velocity_z = 0;
		m_pSphereMesh = NULL;
	}
	~CSphere(void) {}

	bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
			return false;
		return true;
	}

	void destroy(void)
	{
		if (m_pSphereMesh != NULL) {
			m_pSphereMesh->Release();
			m_pSphereMesh = NULL;
		}
	}

	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
pDevice->SetMaterial(&m_mtrl);
m_pSphereMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball)
	{
		D3DXVECTOR3 ballCenter = ball.getCenter();
		double distance = (this->center_x - ballCenter.x)*(this->center_x - ballCenter.x) + (this->center_z - ballCenter.z)*(this->center_z - ballCenter.z);
		if (distance <= 4 * (this->getRadius())*(this->getRadius())) {
			return true;
		}
		return false;
	}

	float normalizeX(float axisX, float axisZ) {
		float num = sqrt(axisX * axisX + axisZ * axisZ);
		return axisX / num;
	}

	float normalizeZ(float axisX, float axisZ) {
		float num = sqrt(axisX * axisX + axisZ * axisZ);
		return axisZ / num;
	}

	void hitBy(CSphere& ball)
	{
		if (hasIntersected(ball)) {
			score += 10;
			D3DXVECTOR3 ballCenter = ball.getCenter();

			float collisionX = normalizeX(this->center_x - ballCenter.x, this->center_z - ballCenter.z);
			float collisionZ = normalizeZ(this->center_x - ballCenter.x, this->center_z - ballCenter.z);

			float multipleVectorsA = ball.getVelocity_X()*collisionX + ball.getVelocity_Z()*collisionZ;
			float multipleVectorsB = this->getVelocity_X()*collisionX + this->getVelocity_Z()*collisionZ;

			float transVAx = collisionX * multipleVectorsA;
			float transVAz = collisionZ * multipleVectorsA;

			float transVBx = collisionX * multipleVectorsB;
			float transVBz = collisionZ * multipleVectorsB;

			float v1x = this->m_velocity_x + transVAx - transVBx;
			float v1z = this->m_velocity_z + transVAz - transVBz;

			//float v2x = ball.getVelocity_X() - transVAx + transVBx;
			//float v2z = ball.getVelocity_Z() - transVAz + transVBz;

			setCenter(this->center_x + 0.01 * collisionX, this->center_y, this->center_z + 0.01 * collisionZ);

			setPower(1.5*v1x, 1.5*v1z);
			//ball.setPower(v2x, v2z);
		}
	}

	void ballUpdate(float timeDiff)
	{
		const float TIME_SCALE = 3.3;
		D3DXVECTOR3 cord = this->getCenter();
		double velocityX = abs(this->getVelocity_X());
		double velocityZ = abs(this->getVelocity_Z());

		if (velocityX > 0.01 || velocityZ > 0.01)
		{
			float timeX = cord.x + TIME_SCALE*timeDiff*m_velocity_x;
			float timeZ = cord.z + TIME_SCALE*timeDiff*m_velocity_z;

			//correction of position of ball
			/* Please uncomment this part because this correction of ball position is necessary when a ball collides with a wall
			if(tX >= (4.5 - M_RADIUS))
				tX = 4.5 - M_RADIUS;
			else if(tX <=(-4.5 + M_RADIUS))
				tX = -4.5 + M_RADIUS;
			else if(tZ <= (-3 + M_RADIUS))
				tZ = -3 + M_RADIUS;
			else if(tZ >= (3 - M_RADIUS))
				tZ = 3 - M_RADIUS;
			*/
			this->setCenter(timeX, cord.y, timeZ);
		}
		else { this->setPower(0, 0); }
		//this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
		double rate = 1 - (1 - DECREASE_RATE)*timeDiff * 400;
		if (rate < 0)
			rate = 0;
		this->setPower(getVelocity_X() * rate, getVelocity_Z() * rate);

		// 공이 보드 밖으로 벗어나지 못하게 제한
		if (this->center_z + this->getRadius() > 3.06f)
		{
			this->center_z = 3.06f - this->getRadius();
		}
		if (this->center_z - this->getRadius() < -3.06f)
		{
			this->center_z = -3.06f + this->getRadius();
		}
		if (this->center_x + this->getRadius() > 4.56f)
		{
			this->center_x = 4.56f - this->getRadius();
		}
		//if (this->center_x - this->getRadius() < -4.56f)
		//{
		//	this->center_x = -4.56f + this->getRadius();
		//}
	}

	double getVelocity_X() { return this->m_velocity_x; }
	double getVelocity_Z() { return this->m_velocity_z; }

	void setPower(double vx, double vz)
	{
		this->m_velocity_x = vx;
		this->m_velocity_z = vz;
		// 최대 속도 제한
		if (vx > 5) this->m_velocity_x = 5;
		if (vz > 5) this->m_velocity_z = 5;
	}

	void setCenter(float x, float y, float z)
	{
		D3DXMATRIX m;
		center_x = x;	center_y = y;	center_z = z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getRadius(void)  const { return (float)(M_RADIUS); }
	const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	D3DXVECTOR3 getCenter(void) const
	{
		D3DXVECTOR3 org(center_x, center_y, center_z);
		return org;
	}

private:
	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh*              m_pSphereMesh;

};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:
	float					m_x;
	float					m_z;
	float                   m_width;
	float                   m_depth;
	float					m_height;
	float					rotation;

	void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

	D3DXMATRIX              m_mLocal;
	D3DMATERIAL9            m_mtrl;
	ID3DXMesh*              m_pBoundMesh;

public:
	CWall(void)
	{
		D3DXMatrixIdentity(&m_mLocal);
		ZeroMemory(&m_mtrl, sizeof(m_mtrl));
		m_width = 0;
		m_depth = 0;
		m_pBoundMesh = NULL;
	}
	~CWall(void) {}
public:

	float getRotation() {
		return rotation;
	}

	bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
	{
		if (NULL == pDevice)
			return false;

		m_mtrl.Ambient = color;
		m_mtrl.Diffuse = color;
		m_mtrl.Specular = color;
		m_mtrl.Emissive = d3d::BLACK;
		m_mtrl.Power = 5.0f;

		m_width = iwidth;
		m_depth = idepth;

		if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
			return false;
		return true;
	}
	void destroy(void)
	{
		if (m_pBoundMesh != NULL) {
			m_pBoundMesh->Release();
			m_pBoundMesh = NULL;
		}
	}
	void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return;
		pDevice->SetTransform(D3DTS_WORLD, &mWorld);
		pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
		pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
	}

	bool hasIntersected(CSphere& ball) {
		D3DXVECTOR3 pos = ball.getCenter(); //공의 위치좌표
		if (this->m_x < -4) {
			if ((pos.x < -4.5 + ball.getRadius())) {
				return true;
			}
		}
		else if (4 < this->m_x) {
			if (4.5 - ball.getRadius() < pos.x) {
				return true;
			}
		}
		else {
			if (0 < this->m_z) {
				if (3 - ball.getRadius() < pos.z) {
					return true;
				}
			}
			else {
				if (pos.z < -3 + ball.getRadius()) {
					return true;
				}
			}
		}
		return false;
	}

	bool hasRodIntersected(CSphere& ball)
	{
		float rodRotation = this->getRotation();
		if (rodRotation < 0) rodRotation *= (-1);

		D3DXVECTOR3 ballCenter = ball.getCenter();

		// rod z = ax + b
		float a, b;
		b = abs(m_x) / tan(rodRotation) - abs(m_z);
		a = (b - m_z) / (0 - m_x);
		double distance = (a*ballCenter.x - ballCenter.z + b) / sqrt(a*a + 1);
		if (distance <= 4*ball.getRadius()*ball.getRadius()){
			interSectedCount++;
			return true;
		}
		return false;
	}


	void hitBy(CSphere& ball) {
		if (hasIntersected(ball)) {
			D3DXVECTOR3 b = ball.getCenter();
			float velocityX = ball.getVelocity_X();
			float velocityZ = ball.getVelocity_Z();
			if (-4 < this->m_x && this->m_x < 4) {
				velocityZ = -velocityZ;
				if (0 < this->m_z) {
					ball.setCenter(b.x, b.y, b.z - 0.01);
				}
				else {
					ball.setCenter(b.x, b.y, b.z + 0.01);
				}
			}
			else {
				velocityX = -velocityX;
				if (0 < this->m_x) {
					ball.setCenter(b.x - 0.01, b.y, b.z);
				}
				else {
					ball.setCenter(b.x + 0.01, b.y, b.z);
				}
			}
			ball.setPower(velocityX, velocityZ);
		}
	}

	void hitRodBy(CSphere& ball){
		if (hasRodIntersected(ball)){
			float rodRotation = this->getRotation();
			if (rodRotation < 0) rodRotation *= (-1);

			D3DXVECTOR3 rodLocate(this->m_x, 0.12f, this->m_z);

			// rod z = ax + b
			float a, b;
			b = abs(m_x) / tan(rodRotation) - abs(m_z);
			a = (b - m_z) / (0 - m_x);

			// 원과 rod의 충돌점
			float collidX, collidZ;
			// sphere 진행방향 z = ax + b
			float sphereA, sphereB;
			sphereA = ball.getVelocity_Z() / ball.getVelocity_X();
			sphereB = ball.getVelocity_Z();

			collidX = (b - sphereB) / (sphereA - a);
			collidZ = collidX * a + b;

			ball.setPower(-collidX - collidZ*a, collidX / a + collidZ);
		}
	}

	void setPosition(float x, float y, float z)
	{
		D3DXMATRIX m;
		this->m_x = x;
		this->m_z = z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	void rotate(float x, float y, float z, float angle) {
		rotation += angle;
		D3DXMATRIX worldPosition, localPosition, originalLocalPosition, rotationMatrix, result;
		D3DXMatrixTranslation(&worldPosition, this->m_x, y, this->m_z);
		D3DXMatrixTranslation(&localPosition, x, y, z);
		D3DXMatrixTranslation(&originalLocalPosition, -x, -y, -z);
		D3DXMatrixRotationY(&rotationMatrix, rotation);
		result = localPosition * rotationMatrix * worldPosition * originalLocalPosition;
		setLocalTransform(result);
	}

	float getHeight(void) const { return M_HEIGHT; }
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
	CLight(void)
	{
		static DWORD i = 0;
		m_index = i++;
		D3DXMatrixIdentity(&m_mLocal);
		::ZeroMemory(&m_lit, sizeof(m_lit));
		m_pMesh = NULL;
		m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		m_bound._radius = 0.0f;
	}
	~CLight(void) {}
public:
	bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
	{
		if (NULL == pDevice)
			return false;
		if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
			return false;

		m_bound._center = lit.Position;
		m_bound._radius = radius;

		m_lit.Type = lit.Type;
		m_lit.Diffuse = lit.Diffuse;
		m_lit.Specular = lit.Specular;
		m_lit.Ambient = lit.Ambient;
		m_lit.Position = lit.Position;
		m_lit.Direction = lit.Direction;
		m_lit.Range = lit.Range;
		m_lit.Falloff = lit.Falloff;
		m_lit.Attenuation0 = lit.Attenuation0;
		m_lit.Attenuation1 = lit.Attenuation1;
		m_lit.Attenuation2 = lit.Attenuation2;
		m_lit.Theta = lit.Theta;
		m_lit.Phi = lit.Phi;
		return true;
	}
	void destroy(void)
	{
		if (m_pMesh != NULL) {
			m_pMesh->Release();
			m_pMesh = NULL;
		}
	}
	bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
	{
		if (NULL == pDevice)
			return false;

		D3DXVECTOR3 pos(m_bound._center);
		D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
		D3DXVec3TransformCoord(&pos, &pos, &mWorld);
		m_lit.Position = pos;

		pDevice->SetLight(m_index, &m_lit);
		pDevice->LightEnable(m_index, TRUE);
		return true;
	}

	void draw(IDirect3DDevice9* pDevice)
	{
		if (NULL == pDevice)
			return;
		D3DXMATRIX m;
		D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
		pDevice->SetTransform(D3DTS_WORLD, &m);
		pDevice->SetMaterial(&d3d::WHITE_MTRL);
		m_pMesh->DrawSubset(0);
	}

	D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
	DWORD               m_index;
	D3DXMATRIX          m_mLocal;
	D3DLIGHT9           m_lit;
	ID3DXMesh*          m_pMesh;
	d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall	g_legoPlane;
CWall	g_legowall[5];
CWall   g_rod[2];
CSphere	g_sphere[NUM_OF_SPHERE];
//CSphere	g_target_blueball;
CLight	g_light;
LPD3DXFONT g_font;
LPD3DXFONT g_debug;

double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup()
{
	int i;

	D3DXMatrixIdentity(&g_mWorld);
	D3DXMatrixIdentity(&g_mView);
	D3DXMatrixIdentity(&g_mProj);

	// create plane and set the position
	if (false == g_legoPlane.create(Device, -1, -1, 9, 0.03f, 6, d3d::GREEN)) return false;
	g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

	// create walls and set the position. note that there are four walls
	if (false == g_legowall[0].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[0].setPosition(0.0f, 0.12f, 3.06f);
	if (false == g_legowall[1].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[1].setPosition(0.0f, 0.12f, -3.06f);
	if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[2].setPosition(4.56f, 0.12f, 0.0f);
	//if (false == g_legowall[3].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	//g_legowall[3].setPosition(-4.56f, 0.12f, 0.0f);

	if (g_rod[0].create(Device, -1, -1, 0.12f, 0.3f, 3.0f, d3d::DARKRED) == false) {
		return false;
	}
	g_rod[0].setPosition(-3.0f, 0.12f, -1.5f);
	g_rod[0].rotate(0.0f, 0.12f, 1.5f, -0.5f);
	if (g_rod[1].create(Device, -1, -1, 0.12f, 0.3f, 3.0f, d3d::DARKRED) == false) {
		return false;
	}
	g_rod[1].setPosition(-3.0f, 0.12f, 1.5f);
	g_rod[1].rotate(0.0f, 0.12f, -1.5f, 0.5f);
	// create four balls and set the position
	for (i = 0; i < NUM_OF_SPHERE; i++) {
		if (g_sphere[i].create(Device, sphereColor[i]) == false) return false;
		g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
		g_sphere[i].setPower(0, 0);
	}
	// 시작 시 하얀 공에 중력 적용
	g_sphere[3].setPower(-1.0, 0);

	// create blue ball for set direction
	//if (false == g_target_blueball.create(Device, d3d::BLUE)) return false;
	//g_target_blueball.setCenter(.0f, (float)M_RADIUS, .0f);

	// light setting 
	D3DLIGHT9 lit;
	::ZeroMemory(&lit, sizeof(lit));
	lit.Type = D3DLIGHT_POINT;
	lit.Diffuse = d3d::WHITE;
	lit.Specular = d3d::WHITE * 0.9f;
	lit.Ambient = d3d::WHITE * 0.9f;
	lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
	lit.Range = 100.0f;
	lit.Attenuation0 = 0.0f;
	lit.Attenuation1 = 0.9f;
	lit.Attenuation2 = 0.0f;
	if (false == g_light.create(Device, lit))
		return false;

	// Position and aim the camera.
	D3DXVECTOR3 pos(-5.0f, 7.5f, 0.0f);
	D3DXVECTOR3 target(0.0f, -2.5f, 0.0f);
	D3DXVECTOR3 up(1.0f, 0.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);

	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 3,
		(float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

	// Set render states.
	Device->SetRenderState(D3DRS_LIGHTING, TRUE);
	Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
	Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	g_light.setLight(Device, g_mWorld);
	D3DXCreateFont(Device, 60, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &g_font);
	D3DXCreateFont(Device, 20, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &g_debug);
	return true;
}

void Cleanup(void)
{
	g_legoPlane.destroy();
	for (int i = 0; i < 3; i++) {
		g_legowall[i].destroy();
	}
	destroyAllLegoBlock();
	g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
	int i = 0;
	int j = 0;


	if (Device)
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();

		// update the position of each ball. during update, check whether each ball hit by walls.
		for (i = 0; i < NUM_OF_SPHERE; i++) {
			g_sphere[i].ballUpdate(timeDelta);
			if(i < 3) g_legowall[i].hitBy(g_sphere[3]);
		}
		// 하얀 공에 중력 적용
		g_sphere[3].setPower(g_sphere[3].getVelocity_X() + timeDelta * (-9.8), g_sphere[3].getVelocity_Z());

		// check whether any two balls hit together and update the direction of balls
		// 하얀 공이 충돌하는 지 확인
		for (i = 0; i < NUM_OF_SPHERE; i++) { 
			if (i == 3) continue;
			g_sphere[3].hitBy(g_sphere[i]);
		}


		// draw plane, walls, and spheres
		g_legoPlane.draw(Device, g_mWorld);
		for (i = 0; i < 3; i++) {
			g_legowall[i].draw(Device, g_mWorld);
		}
		for (i = 0; i < NUM_OF_SPHERE; i++) {
			g_sphere[i].draw(Device, g_mWorld);
		}

		for (i = 0; i < 2; i++) {
			g_rod[i].draw(Device, g_mWorld);
		}

		g_light.draw(Device);


		// Create a colour for the text - in this case blue
		D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 0, 0, 255);

		// Create a rectangle to indicate where on the screen it should be drawn
		RECT scoreRect;
		scoreRect.left = 50;
		scoreRect.right = 780;
		scoreRect.top = 50;
		scoreRect.bottom = scoreRect.top + 80;

		RECT gameoverRect;
		gameoverRect.left = 375;
		gameoverRect.right = 780;
		gameoverRect.top = 500;
		gameoverRect.bottom = gameoverRect.top + 80;


		// Draw some text
		char scoreBuffer[20];
		//char debugBuffer[20];
		char gameoverBuffer[10] = "game over";
		_itoa_s(score, scoreBuffer, 20, 10);
		//_itoa_s(interSectedCount, debugBuffer, 20, 10);
		g_font->DrawText(NULL, scoreBuffer, -1, &scoreRect, 0, fontColor);
		if (g_sphere[3].getCenter().x < -4.56 - g_sphere[3].getRadius())
		{
			fontColor = D3DCOLOR_ARGB(255, 255, 0, 0);
			g_font->DrawTextA(NULL, gameoverBuffer, -1, &gameoverRect, 0, fontColor);
			fontColor = D3DCOLOR_ARGB(255, 0, 0, 255);
		}
		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture(0, NULL);

	}

	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
	static int old_x = 0;
	static int old_y = 0;
	static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

	switch (msg) {
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_KEYUP:
	{
		switch (wParam) {
		case VK_RIGHT:
			//손을 놓으면 막대 오른쪽이 내려간다.	
			if (g_rod[0].getRotation() == 0.5f) {
				g_rod[0].rotate(0.0f, 0.12f, 1.3f, -1.0f);
			}						
			break;
		case VK_LEFT:
			//손을 놓으면 막대 왼쪽이 내려간다.
			if (g_rod[1].getRotation() == -0.5f) {
				g_rod[1].rotate(0.0f, 0.12f, -1.3f, 1.0f);
			}	
			break;
		}
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam) {
		case VK_ESCAPE:
			::DestroyWindow(hwnd);
			break;
		case VK_RETURN:
			if (NULL != Device) {
				wire = !wire;
				Device->SetRenderState(D3DRS_FILLMODE,
					(wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
			}
			break;

		case VK_RIGHT: {
			//막대 오른쪽이 움직인다.
			if (g_rod[0].getRotation() == -0.5f) {
				g_rod[0].rotate(0.0f, 0.12f, 1.3f, 1.0f);
				g_rod[0].hitRodBy(g_sphere[3]);
			}			
			break;
		}
		case VK_LEFT: {
			//막대 왼쪽이 움직인다.
			if (g_rod[1].getRotation() == 0.5f) {
				g_rod[1].rotate(0.0f, 0.12f, -1.3f, -1.0f);
				g_rod[1].hitRodBy(g_sphere[3]);
			}
			break;
		}

		case VK_SPACE:
			g_sphere[3].setCenter(spherePos[3][0], (float)M_RADIUS, spherePos[3][1]);
			g_sphere[3].setPower(-1, 0);
			score = 0;
			/*D3DXVECTOR3 targetPos = g_target_blueball.getCenter();
			D3DXVECTOR3	whitePos = g_sphere[3].getCenter();
			double theta = acos(sqrt(pow(targetPos.x - whitePos.x, 2)) / sqrt(pow(targetPos.x - whitePos.x, 2) +
				pow(targetPos.z - whitePos.z, 2)));		// 기본 1 사분면
			if (targetPos.z - whitePos.z <= 0 && targetPos.x - whitePos.x >= 0) { theta = -theta; }	//4 사분면
			if (targetPos.z - whitePos.z >= 0 && targetPos.x - whitePos.x <= 0) { theta = PI - theta; } //2 사분면
			if (targetPos.z - whitePos.z <= 0 && targetPos.x - whitePos.x <= 0) { theta = PI + theta; } // 3 사분면
			double distance = sqrt(pow(targetPos.x - whitePos.x, 2) + pow(targetPos.z - whitePos.z, 2));
			g_sphere[3].setPower(distance * cos(theta), distance * sin(theta));*/
			break;
		}
		break;
	}

	/*case WM_MOUSEMOVE:
	{
		int new_x = LOWORD(lParam);
		int new_y = HIWORD(lParam);
		float dx;
		float dy;

		// 핀볼 보드 고정
		/*if (LOWORD(wParam) & MK_LBUTTON) {

			if (isReset) {
				isReset = false;
			}
			else {
				D3DXVECTOR3 vDist;
				D3DXVECTOR3 vTrans;
				D3DXMATRIX mTrans;
				D3DXMATRIX mX;
				D3DXMATRIX mY;

				switch (move) {
				case WORLD_MOVE:
					dx = (old_x - new_x) * 0.01f;
					dy = (old_y - new_y) * 0.01f;
					D3DXMatrixRotationY(&mX, dx);
					D3DXMatrixRotationX(&mY, dy);
					g_mWorld = g_mWorld * mX * mY;

					break;
				}
			}

			old_x = new_x;
			old_y = new_y;

		}
		else*/ //{
			/*isReset = true;

		if (LOWORD(wParam) & MK_RBUTTON) {
			dx = (old_x - new_x);// * 0.01f;
			dy = (old_y - new_y);// * 0.01f;

			D3DXVECTOR3 coord3d = g_target_blueball.getCenter();
			g_target_blueball.setCenter(coord3d.x + dx*(-0.007f), coord3d.y, coord3d.z + dy*0.007f);
		}
		old_x = new_x;
		old_y = new_y;

		move = WORLD_MOVE;
		//}*/
		//break;
		//}
	}

	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd)
{
	srand(static_cast<unsigned int>(time(NULL)));

	if (!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}
