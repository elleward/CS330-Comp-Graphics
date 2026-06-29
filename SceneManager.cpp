///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
//
//  Student: Elle Ward
//  Course: CS-330 Computational Graphics and Visualization
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  Loads textures from image files, configures texture
 *  mapping parameters, generates mipmaps, and loads the
 *  texture into the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		// register the loaded texture and associate it with the tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  Binds loaded textures to OpenGL texture memory slots.
 *  There are up to 16 slots available.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  Frees memory in all used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  Returns the ID for the loaded texture associated
 *  with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  Returns the slot index for the loaded texture associated
 *  with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  Returns a material from the defined materials list
 *  associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  Sets the transform buffer using the passed in
 *  transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	scale = glm::scale(scaleXYZ);
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  Sets the passed in color into the shader for the
 *  next draw command.
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  Sets the texture data associated with the passed in
 *  tag into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  Sets the texture UV scale values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  Passes material values into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  Configures material settings for all objects in the scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// -------------------------------------------------------
	// Wood material for the desk surface
	// Low shininess simulates a matte flat wood finish
	// -------------------------------------------------------
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.2f, 0.15f, 0.1f);
	woodMaterial.ambientStrength = 0.3f;
	woodMaterial.diffuseColor = glm::vec3(0.8f, 0.6f, 0.4f);
	woodMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial.shininess = 8.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// -------------------------------------------------------
	// Metal material for the monitor bezel
	// High shininess simulates a smooth metallic surface
	// -------------------------------------------------------
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	metalMaterial.ambientStrength = 0.2f;
	metalMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	metalMaterial.shininess = 96.0f;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	// -------------------------------------------------------
	// Brushed gold material for the monitor stand
	// Moderate shininess simulates brushed metal
	// -------------------------------------------------------
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.15f, 0.05f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.8f, 0.6f, 0.2f);
	goldMaterial.specularColor = glm::vec3(1.0f, 0.9f, 0.4f);
	goldMaterial.shininess = 32.0f;
	goldMaterial.tag = "gold";
	m_objectMaterials.push_back(goldMaterial);

	// -------------------------------------------------------
	// Ceramic material for the coffee mug
	// Mid-range shininess simulates a glazed ceramic surface
	// -------------------------------------------------------
	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.15f);
	ceramicMaterial.ambientStrength = 0.2f;
	ceramicMaterial.diffuseColor = glm::vec3(0.3f, 0.4f, 0.7f);
	ceramicMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.8f);
	ceramicMaterial.shininess = 48.0f;
	ceramicMaterial.tag = "ceramic";
	m_objectMaterials.push_back(ceramicMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  Configures light sources for the 3D scene.
 *  Up to 4 light sources can be defined.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable custom lighting in the shader
	// Without this the scene renders completely black
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// -------------------------------------------------------
	// LIGHT 0: Primary overhead light (warm white)
	// Simulates an office ceiling light above the desk
	// High focal strength produces tight defined highlights
	// -------------------------------------------------------
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(0.0f, 10.0f, 2.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.3f, 0.3f, 0.3f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 0.95f, 0.8f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.8f);

	// -------------------------------------------------------
	// LIGHT 1: Secondary fill light (cool blue)
	// Simulates ambient room light from the left side
	// Lower focal strength produces softer wider highlights
	// -------------------------------------------------------
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-8.0f, 5.0f, 5.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.05f, 0.05f, 0.1f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.3f, 0.5f, 0.8f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.4f, 0.6f, 1.0f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.4f);
}

/***********************************************************
 *  LoadSceneTextures()
 *
 *  Loads image files into OpenGL texture memory slots
 *  for use in the rendered 3D scene.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	// load wood texture for the desk surface
	bReturn = CreateGLTexture("textures/rusticwood.jpg", "wood");

	// load dark metallic texture for the monitor bezel
	bReturn = CreateGLTexture("textures/stainless.jpg", "monitor");

	// load brushed metal texture for the monitor stand
	bReturn = CreateGLTexture("textures/circular-brushed-gold-texture.jpg", "stand");

	// load blue texture for the coffee mug body
	bReturn = CreateGLTexture("textures/bluecup.jpg", "bluecup");

	// bind all loaded textures to OpenGL texture slots
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  Loads shapes and textures into memory to support
 *  the 3D scene rendering.
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// define materials for objects in the scene
	DefineObjectMaterials();
	// add and configure light sources
	SetupSceneLights();
	// load textures before drawing
	LoadSceneTextures();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	// added for coffee mug
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  Transforms and draws all 3D shapes in the scene.
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	// Desk surface - plane mesh with wood texture
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// tile wood texture across the large desk surface
	SetShaderTexture("wood");
	SetTextureUVScale(4.0f, 4.0f);
	// wood material reflects light with a dull matte finish
	SetShaderMaterial("wood");
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/****************************************************************/
	// Monitor screen - box mesh with metallic texture
	scaleXYZ = glm::vec3(6.0f, 4.0f, 0.3f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 3.5f, 1.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// apply dark metallic texture to the monitor bezel
	SetShaderTexture("monitor");
	SetTextureUVScale(1.0f, 1.0f);
	// metal material produces tight bright specular highlights
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/****************************************************************/
	// Monitor stand - tapered cylinder with brushed gold texture
	scaleXYZ = glm::vec3(1.5f, 2.0f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// apply brushed metal texture to the monitor stand
	SetShaderTexture("stand");
	SetTextureUVScale(1.0f, 1.0f);
	// gold material produces warm moderate specular highlights
	SetShaderMaterial("gold");
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	/****************************************************************/
	// COFFEE MUG BODY (Cylinder)
	// Upright cylinder sitting to the right of the monitor
	scaleXYZ = glm::vec3(1.0f, 2.5f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.5f, 0.0f, 1.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// apply blue texture to the mug body
	SetShaderTexture("bluecup");
	SetTextureUVScale(1.0f, 1.0f);
	// ceramic material simulates a glazed mug surface
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/****************************************************************/
	// COFFEE MUG HANDLE (Torus)
	// Loop attached to the right side of the mug body
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.15f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(6.6f, 1.2f, 1.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// handle matches the mug body color
	SetShaderColor(0.12f, 0.16f, 0.35f, 1.0f);
	// ceramic material for consistent shading with the mug body
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
}