class Skybox {
public:
	GLuint cubemap;
public:
	Skybox(GLuint cubemapbind)
	{
		cubemap = cubemapbind;
		cube = gen_mapcube();
	};
	void display(void) const 
	{
		glDepthFunc(GL_LEQUAL);
		activate_texture(GL_TEXTURE0, GL_TEXTURE_CUBE_MAP, cubemap);
		glBindVertexArray(cube.VAO);
		glDrawElements(cube.mode, cube.ecount, GL_UNSIGNED_SHORT, NULL);
		glDepthFunc(GL_LESS);
	};
private:
	struct mesh cube;
};
