class Camera {
public:
	glm::vec3 center;
	glm::vec3 eye;
	glm::mat4 view;
	glm::mat4 project;
	float FOV;
	float nearclip;
	float farclip;
	float aspectratio;
public:
	Camera(glm::vec3 pos, float fov, float aspect, float near, float far);
	void update_center(float delta);
	void update_view(void);

private:
	float yaw;
	float pitch;
	float sensitivity;
	float speed;
	int x, y;
	glm::vec3 up;
};
