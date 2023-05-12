#ifndef __PHYSICS_H__
#define __PHYSICS_H__

#include "math.h"

// Define constants
#define NUM_BODIES 200

#define WORLD_SCALE 40.0f
#define EXPLOSION_POWER (0.25f*WORLD_SCALE)

typedef struct
{
	vec3 position;
	vec3 velocity;
	vec3 force;
	float mass, inverse_mass;

	float radius;	// radius if it's a sphere
	vec3 size;		// bounding box if it's an AABB
} RigidBody_t;

extern RigidBody_t Bodies[];
extern RigidBody_t aabb;

void integrate(RigidBody_t *body, float dt);
void explode(RigidBody_t *bodies);
void sphere_sphere_collision(RigidBody_t *a, RigidBody_t *b);
void sphere_aabb_collision(RigidBody_t *sphere, RigidBody_t *aabb);

#endif
