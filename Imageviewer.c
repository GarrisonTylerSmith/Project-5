typedef struct{
	float position[3];
	float color[4];
	float textcoord[3];
}Vertex;

const Vertex Vertices[] = {
  {{1, -1, 0}, {1, 0, 0, 1}},
  {{1, 1, 0}, {0, 1, 0, 1}},
  {{-1, 1, 0}, {0, 0, 1, 1}},
  {{-1, -1, 0}, {0, 0, 0, 1}}
};

GLuint myTexture;
glGenTextures(1, &myTexture);
glBindTextures(GL_TEXTURE_2D, myTexture);
glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, heigth, 0 , GL_RGBA, GL_UNSIGNED_BYTE, your_image_data);

// compiling shaders

textCoordSlot = glGetAttribLocation(programID, "TextCoordID");
glEnableVertexAttribArray(textCoordSlot);
textureUniform = glGetUniformLocation(programID, "Texture");

// Before glDrawElements
glVertexAttribPointer(textCoordSlot, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), sizeof(float)*7);

glActivateTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, myTexture);
glUniformi(textureUniform, 0);