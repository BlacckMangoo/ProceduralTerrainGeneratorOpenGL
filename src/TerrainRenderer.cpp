#include "TerrainRenderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"
#include "TerrainGenerator.h"
#include <Light.h>
#include "UiManager.h"
#include <iostream>

TerrainRenderer::TerrainRenderer(Shader& shader)
{
    this->shader = shader;
    this->terrainGenerated = false;
    this->terrainVertexCount = 0;
    this->initTerrainRenderData();
    this->initTerrainData();
}

TerrainRenderer::~TerrainRenderer()
{
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
    if (terrainGenerated) {
        glDeleteVertexArrays(1, &this->terrainVAO);
        glDeleteBuffers(1, &this->terrainVBO);
    }
}

void TerrainRenderer::initTerrainRenderData()
{
    // Initialize VAO and VBO for potential future use
    // Cube vertices removed - using terrain generation instead
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
}

void TerrainRenderer::Draw(RenderModes mode ,const  Camera& camera )
{
    // Check if shader is valid
    if (shader.ID == 0) {
        std::cout << "Error: Shader not initialized in TerrainRenderer::Draw" << std::endl;
        return;
    }

    // Set polygon mode
    if (mode == RENDER_MODE_WIRE_FRAME)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else if (mode == RENDER_MODE_FILL)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else if (mode == RENDER_MODE_POINTS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    shader.Use();

    // Matrices
	glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view =  glm::lookAt(camera.cameraPos, camera.cameraPos + camera.cameraFront, camera.cameraUp); // Camera view matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f); // You can pass aspect

    glm::mat4 mvp = projection * view * model;

    // Upload to shader with error checking
    GLuint mvpLoc = glGetUniformLocation(shader.ID, "u_MVP");
    if (mvpLoc != -1) {
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    } else {
        std::cout << "Warning: Uniform 'u_MVP' not found in shader" << std::endl;
    }

    // Check if VAO is valid before drawing
    if (VAO != 0) {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    } else {
        std::cout << "Error: VAO not initialized in TerrainRenderer::Draw" << std::endl;
    }
}

void TerrainRenderer::initTerrainData()
{
    // Initialize terrain VAO and VBO (empty for now)
    glGenVertexArrays(1, &this->terrainVAO);
    glGenBuffers(1, &this->terrainVBO);
}

void TerrainRenderer::GenerateTerrain(int width, int height, float scale, float frequency, float amplitude){

    // Generate terrain data using TerrainGenerator
    terrainVertices = terrainGen.generateTerrain(width, height, scale, frequency,
        amplitude);
    terrainVertexCount = terrainVertices.size() / 6; // 6 components per vertex (x, y, z, nx, ny, nz)

    // Bind and upload terrain data to GPU
    glBindVertexArray(terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(float), terrainVertices.data(), GL_STATIC_DRAW);

    // Position attribute (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    // Normal attribute (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    terrainGenerated = true;
}

void TerrainRenderer::DrawTerrain(RenderModes mode, const Camera& camera, std::vector<Light*> lights){
    if (!terrainGenerated) {
        return; // No terrain to render
    }

    // Check if shader is valid
    if (shader.ID == 0) {
        std::cout << "Error: Shader not initialized in TerrainRenderer::DrawTerrain" << std::endl;
        return;
    }

    // Set polygon mode
    if (mode == RENDER_MODE_WIRE_FRAME)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else if (mode == RENDER_MODE_FILL)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else if (mode == RENDER_MODE_POINTS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    shader.Use();

    // Matrices
    glm::mat4 model = glm::mat4(1.0f); // Keep terrain static, no rotation
    glm::mat4 view = glm::lookAt(camera.cameraPos, camera.cameraPos + camera.cameraFront, camera.cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(camera.getFOV()), 16.0f/9.0f, 0.01f, 2000.0f);
    glm::mat4 mvp = projection * view * model;

    // Upload matrices with error checking
    GLuint mvpLoc = glGetUniformLocation(shader.ID, "u_MVP");
    if (mvpLoc != -1) {
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    } else {
        std::cout << "Warning: Uniform 'u_MVP' not found in shader" << std::endl;
    }

    GLuint modelLoc = glGetUniformLocation(shader.ID, "u_Model");
    if (modelLoc != -1) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    } else {
        std::cout << "Warning: Uniform 'u_Model' not found in shader" << std::endl;
    }

    // Upload lighting data with error checking
    const int MAX_LIGHTS = 16; // Must match shader
    int numLights = std::min((int)lights.size(), MAX_LIGHTS);
    std::vector<glm::vec3> lightPositions(numLights);
    std::vector<glm::vec3> lightColors(numLights);
    for (int i = 0; i < numLights; ++i) {
        lightPositions[i] = lights[i]->position;
        lightColors[i] = lights[i]->color;
    }
    
    GLuint numLightsLoc = glGetUniformLocation(shader.ID, "numLights");
    if (numLightsLoc != -1) {
        shader.SetInteger("numLights", numLights);
    } else {
        std::cout << "Warning: Uniform 'numLights' not found in shader" << std::endl;
    }
    
    for (int i = 0; i < numLights; ++i) {
        std::string posName = "lightPositions[" + std::to_string(i) + "]";
        std::string colorName = "lightColors[" + std::to_string(i) + "]";
        
        GLuint posLoc = glGetUniformLocation(shader.ID, posName.c_str());
        GLuint colorLoc = glGetUniformLocation(shader.ID, colorName.c_str());
        
        if (posLoc != -1) {
            shader.SetVector3f(posName.c_str(), lightPositions[i]);
        }
        if (colorLoc != -1) {
            shader.SetVector3f(colorName.c_str(), lightColors[i]);
        }
    }
    
    GLuint objectColorLoc = glGetUniformLocation(shader.ID, "objectColor");
    if (objectColorLoc != -1) {
        shader.SetVector3f("objectColor", glm::vec3(0.5f)); // Mid-gray
    } else {
        std::cout << "Warning: Uniform 'objectColor' not found in shader" << std::endl;
    }

    // Draw terrain with error checking
    if (terrainVAO != 0 && terrainVertexCount > 0) {
        glBindVertexArray(terrainVAO);
        glDrawArrays(GL_TRIANGLES, 0, terrainVertexCount);
        glBindVertexArray(0);
    } else {
        std::cout << "Error: Terrain VAO not properly initialized or no vertices" << std::endl;
    }

    // Reset polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
