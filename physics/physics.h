#ifndef __PHYSICS_H__
#define __PHYSICS_H__

#include "../math/math.h"

typedef struct Particle_s Particle_t;
typedef struct Camera_s Camera_t;

// Define constants
#define WORLD_SCALE 100.0f
#define EXPLOSION_POWER (5000.0f*WORLD_SCALE)

typedef struct RigidBody_s
{
	vec3 position;
	vec3 velocity;
	vec3 force;

	vec4 orientation;
	vec3 angularVelocity;
	float inertia, invInertia;

	float mass, invMass;

	float radius;	// radius if it's a sphere
	vec3 size;		// bounding box if it's an AABB
} RigidBody_t;

typedef enum
{
	PHYSICS_OBJECT_SPHERE=0,
	PHYSICS_OBJECT_AABB,
	PHYSICS_OBJECT_CAMERA,
	PHYSICS_OBJECT_PARTICLE,
	NUM_PHYSICS_OBJECT_TYPE,
} PhysicsObjectType;

void PhysicsIntegrate(RigidBody_t *body, const float dt);
void PhysicsAffect(RigidBody_t *body, const vec3 center, const float force);
float PhysicsSphereToSphereCollisionResponse(RigidBody_t *a, RigidBody_t *b);
float PhysicsSphereToAABBCollisionResponse(RigidBody_t *sphere, RigidBody_t *aabb);

#endif
