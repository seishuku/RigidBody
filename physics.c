#include <stdint.h>
#include "math.h"
#include "physics.h"

// Render area width/height from main
extern int Width, Height;

RigidBody_t Bodies[NUM_BODIES];
RigidBody_t aabb;

void apply_border_constraints(RigidBody_t *body)
{
	float left=0.0f, right=(float)Width;
	float top=0.0f, bottom=(float)Height;

	// Clamp velocity to "a number".
	//   This reduces the chance of the simulation going unstable.
	if(body->velocity[0]>1000.0f)
		body->velocity[0]=1000.0f;
	else if(body->velocity[0]<-1000.0f)
		body->velocity[0]=-1000.0f;

	if(body->velocity[1]>1000.0f)
		body->velocity[1]=1000.0f;
	else if(body->velocity[1]<-1000.0f)
		body->velocity[1]=-1000.0f;

	if(body->velocity[2]>1000.0f)
		body->velocity[2]=1000.0f;
	else if(body->velocity[2]<-1000.0f)
		body->velocity[2]=-1000.0f;

	// Reflect the bodies off the walls of the window
	if(body->position[0]<left)
	{
		body->position[0]=left+(left-body->position[0]);
		body->velocity[0]=-body->velocity[0];
	}
	else if(body->position[0]>right)
	{
		body->position[0]=right-(body->position[0]-right);
		body->velocity[0]=-body->velocity[0];
	}

	if(body->position[1]<top)
	{
		body->position[1]=top+(top-body->position[1]);
		body->velocity[1]=-body->velocity[1];
	}
	else if(body->position[1]>bottom)
	{
		body->position[1]=bottom-(body->position[1]-bottom);
		body->velocity[1]=-body->velocity[1];
	}
}

void integrate(RigidBody_t *body, float dt)
{
	vec3 gravity={ 0.0f, 9.81f, 0.0f };
	const float damping=0.01f;

	// Clamp delta time, if it's longer than 16MS, clamp it to that.
	//   This reduces the chance of the simulation going unstable.
	if(dt>0.016f)
		dt=0.016f;

	// Apply damping force
	vec3 damping_force;
	Vec3_Setv(damping_force, body->velocity);
	Vec3_Muls(damping_force, -damping);

	body->force[0]+=damping_force[0];
	body->force[1]+=damping_force[1];
	body->force[2]+=damping_force[2];

	// Euler integration of position and velocity
	float dtSq=dt*dt;

	body->position[0]+=body->velocity[0]*dt+0.5f*body->force[0]*body->inverse_mass*dtSq;
	body->position[1]+=body->velocity[1]*dt+0.5f*body->force[1]*body->inverse_mass*dtSq;
	body->position[2]+=body->velocity[2]*dt+0.5f*body->force[2]*body->inverse_mass*dtSq;

	body->velocity[0]+=body->force[0]*body->inverse_mass*dt;
	body->velocity[1]+=body->force[1]*body->inverse_mass*dt;
	body->velocity[2]+=body->force[2]*body->inverse_mass*dt;

	// Apply gravity
	body->force[0]+=(gravity[0]*WORLD_SCALE)*body->mass*dt;
	body->force[1]+=(gravity[1]*WORLD_SCALE)*body->mass*dt;
	body->force[2]+=(gravity[2]*WORLD_SCALE)*body->mass*dt;

	apply_border_constraints(body);
}

void explode(RigidBody_t *bodies)
{
	// Apply impulse forces to each fragment
	vec3 explosion_center={ (float)Width/2.0f, (float)Height, 0.0f };

	for(int i=0;i<NUM_BODIES;i++)
	{
		// Random -1.0 to 1.0, normalized to get a random spherical vector
		Vec3_Set(bodies[i].velocity, (float)rand()/RAND_MAX*2.0f-1.0f, (float)rand()/RAND_MAX*2.0f-1.0f, (float)rand()/RAND_MAX*2.0f-1.0f);
		bodies[i].velocity[2]=0.0f;
		Vec3_Normalize(bodies[i].velocity);

		// Calculate distance and direction from explosion center to fragment
		vec3 direction;
		Vec3_Setv(direction, bodies[i].position);
		Vec3_Subv(direction, explosion_center);
		Vec3_Normalize(direction);

		// Calculate acceleration and impulse force
		vec3 acceleration;
		Vec3_Setv(acceleration, direction);
		Vec3_Muls(acceleration, EXPLOSION_POWER);

		// F=M*A bla bla...
		vec3 force;
		Vec3_Setv(force, acceleration);
		Vec3_Muls(force, bodies[i].mass);

		// Add it into object's velocity
		Vec3_Addv(bodies[i].velocity, force);
	}
}

void sphere_sphere_collision(RigidBody_t *a, RigidBody_t *b)
{
	vec3 center;
	Vec3_Setv(center, b->position);
	Vec3_Subv(center, a->position);
	float distance_squared=Vec3_Dot(center, center);

	float radius_sum=a->radius+b->radius;
	float radius_sum_squared=radius_sum*radius_sum;

	if(distance_squared<radius_sum_squared)
	{
		float distance=sqrtf(distance_squared);
		float penetration=radius_sum-distance;

		distance=1.0f/distance;

		// calculate the collision normal
		vec3 normal;
		Vec3_Setv(normal, b->position);
		Vec3_Subv(normal, a->position);
		Vec3_Muls(normal, distance);

		// calculate the relative velocity
		vec3 rel_velocity;
		Vec3_Setv(rel_velocity, b->velocity);
		Vec3_Subv(rel_velocity, a->velocity);

		float vel_along_normal=Vec3_Dot(rel_velocity, normal);

		// ignore the collision if the bodies are moving apart
		if(vel_along_normal>0.0f)
			return;

		// calculate the restitution coefficient
		float elasticity=0.8f; // typical values range from 0.1 to 1.0

		// calculate the impulse scalar
		float j=-(1.0f+elasticity)*vel_along_normal/(a->inverse_mass+b->inverse_mass);

		// apply the impulse
		vec3 a_impulse;
		Vec3_Setv(a_impulse, normal);
		Vec3_Muls(a_impulse, a->inverse_mass);
		Vec3_Muls(a_impulse, j);
		Vec3_Subv(a->velocity, a_impulse);

		vec3 b_impulse;
		Vec3_Setv(b_impulse, normal);
		Vec3_Muls(b_impulse, b->inverse_mass);
		Vec3_Muls(b_impulse, j);
		Vec3_Addv(b->velocity, b_impulse);

		// correct the position of the bodies to resolve the collision
		float k=0.1f; // typical values range from 0.01 to 0.1
		float percent=0.2f; // typical values range from 0.1 to 0.3
		float correction=penetration/(a->inverse_mass+b->inverse_mass)*percent*k;

		vec3 a_correction;
		Vec3_Setv(a_correction, normal);
		Vec3_Muls(a_correction, a->inverse_mass);
		Vec3_Muls(a_correction, correction);
		Vec3_Subv(a->position, a_correction);

		vec3 b_correction;
		Vec3_Setv(b_correction, normal);
		Vec3_Muls(b_correction, b->inverse_mass);
		Vec3_Muls(b_correction, correction);
		Vec3_Addv(b->position, b_correction);
	}
}

void sphere_aabb_collision(RigidBody_t *sphere, RigidBody_t *aabb)
{
	// Calculate the half extents of the AABB
	vec3 half;
	Vec3_Setv(half, aabb->size);
	Vec3_Muls(half, 0.5f);

	// Calculate the AABB's min and max points
	vec3 aabbMin;
	Vec3_Setv(aabbMin, aabb->position);
	Vec3_Subv(aabbMin, half);

	vec3 aabbMax;
	Vec3_Setv(aabbMax, aabb->position);
	Vec3_Addv(aabbMax, half);

	// Find the closest point on the AABB to the sphere
	vec3 closest;
	Vec3_Set(closest,
			 fmaxf(aabbMin[0], fminf(sphere->position[0], aabbMax[0])),
			 fmaxf(aabbMin[1], fminf(sphere->position[1], aabbMax[1])),
			 fmaxf(aabbMin[2], fminf(sphere->position[2], aabbMax[2])));

	// Calculate the distance between the closest point and the sphere's center
	vec3 distance;
	Vec3_Setv(distance, closest);
	Vec3_Subv(distance, sphere->position);

	float distanceSquared=Vec3_Dot(distance, distance);

	// Check if the distance is less than the sphere's radius
	if(distanceSquared<=sphere->radius*sphere->radius)
	{
		vec3 normal;
		Vec3_Setv(normal, sphere->position);
		Vec3_Subv(normal, closest);

		// Normalize the collision normal
		Vec3_Normalize(normal);

		// Reflect the sphere's velocity based on the collision normal
		float VdotN=Vec3_Dot(sphere->velocity, normal)*2.0f;
		vec3 reflection;
		Vec3_Setv(reflection, normal);
		Vec3_Muls(reflection, VdotN);

		// Apply to velocity
		Vec3_Subv(sphere->velocity, reflection);

		// Calculate the penetration depth
		float penetration=sphere->radius-sqrtf(distanceSquared);

		// correct the position of the bodies to resolve the collision
		float k=0.1f; // typical values range from 0.01 to 0.1
		float percent=0.2f; // typical values range from 0.1 to 0.3
		float correction=penetration/(sphere->inverse_mass+aabb->inverse_mass)*percent*k;

		// Calculate the correction
		vec3 sphere_correction;
		Vec3_Setv(sphere_correction, normal);
		Vec3_Muls(sphere_correction, sphere->inverse_mass);
		Vec3_Muls(sphere_correction, correction);

		// Adjust the sphere's position to resolve the collision
		Vec3_Addv(sphere->position, sphere_correction);
	}
}
