#include <SDL2/SDL.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

#define DEFAULT_SENSITIVITY 0.1f
#define DEFAULT_SPEED 25.f

Camera::Camera(glm::vec3 pos, float fov, float aspect, float near, float far) 
{
	yaw = 0.f;
	pitch = 0.f;
	
	sensitivity = DEFAULT_SENSITIVITY;
	speed = DEFAULT_SPEED;

	center = glm::vec3(0.f, 0.f, 0.f);
	up = glm::vec3(0.f, 1.f, 0.f);
	eye = pos;

	view = glm::mat4{1.f};
	project = glm::perspective(glm::radians(fov), aspect, near, far);

	FOV = fov;
	aspectratio = aspect;
	nearclip = near;
	farclip = far;
}

glm::mat4 make_view_matrix(glm::vec3 eye, glm::vec3 center, glm::vec3 up)
{
	glm::vec3 target = eye + center;

	glm::vec3 f = glm::normalize(target - eye);

	glm::vec3 s = glm::normalize(glm::cross(f, up));
	glm::vec3 u = glm::cross(s, f);

	glm::mat4 view = {
	s.x, u.x, -f.x, 0.0,
	s.y, u.y, -f.y, 0.0,
	s.z, u.z, -f.z, 0.0,
	-glm::dot(s, eye), -glm::dot(u, eye), glm::dot(f, eye), 1.0
	};

	return view;
}

void Camera::update_center(float delta)
{
	int x, y;
	const Uint8 *keystates = SDL_GetKeyboardState(NULL);
	SDL_GetRelativeMouseState(&x, &y);

	yaw += (float)x * sensitivity * 0.01f;
	pitch -= (float)y * sensitivity * 0.01f;

	const float MAX_ANGLE = 1.5f;
	const float MIN_ANGLE = -1.5f;
	if (pitch > MAX_ANGLE) { pitch = MAX_ANGLE; }
	if (pitch < MIN_ANGLE) { pitch = MIN_ANGLE; }

	// point the camera in a direction based on mouse input
	center.x = cos(yaw) * cos(pitch);
	center.y = sin(pitch);
	center.z = sin(yaw) * cos(pitch);
	center = glm::normalize(center);

	// move the camera in the new direction
	const float modifier = speed * delta;
	if (keystates[SDL_SCANCODE_W]) { eye += modifier * center; }
	if (keystates[SDL_SCANCODE_S]) { eye -= modifier * center; }
	if (keystates[SDL_SCANCODE_D]) { eye += modifier * glm::normalize(glm::cross(center, up)); }
	if (keystates[SDL_SCANCODE_A]) { eye -= modifier * glm::normalize(glm::cross(center, up)); }
}

void Camera::update_view(void)
{
	view = make_view_matrix(eye, center, up);
}
