/*
 *  qPhysics
 *  qWorld.cpp
 *
 *	Copyright (c) 2001, AVS
 *	All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *	1.	Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 *	2.	Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 *	3.	All advertising materials mentioning features or use of this software
 *		must display the following acknowledgement:
 *		This product includes software developed by the AVS.
 *	4.	Neither the name of the AVS nor the
 *		names of its contributors may be used to endorse or promote products
 *		derived from this software without specific prior written permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY AVS ''AS IS'' AND ANY
 *	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *	DISCLAIMED. IN NO EVENT SHALL AVS BE LIABLE FOR ANY
 *	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "BulletCollision/CollisionShapes/btBox2dShape.h"
#include "BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.h"

#include "BulletCollision/CollisionShapes/btBox2dShape.h"
#include "BulletCollision/CollisionShapes/btConvex2dShape.h"
#include "BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h"

#include "qWorld.h"

namespace qLib
{
	namespace Physics
	{
		qWorld::qWorld()
		:	m_dynamicsWorld(0),
			m_pickConstraint(0),
			m_defaultContactProcessingThreshold(BT_LARGE_FLOAT)
		{
			this->m_broadphase = new btDbvtBroadphase();
			this->m_collisionConfiguration = new btDefaultCollisionConfiguration();
			this->m_dispatcher = new btCollisionDispatcher(this->m_collisionConfiguration);
			
			btVoronoiSimplexSolver* simplex = new btVoronoiSimplexSolver();
			btMinkowskiPenetrationDepthSolver* pdSolver = new btMinkowskiPenetrationDepthSolver();
			
			
			/*btConvex2dConvex2dAlgorithm::CreateFunc* convexAlgo2d = new btConvex2dConvex2dAlgorithm::CreateFunc(simplex,pdSolver);
			
			this->m_dispatcher->registerCollisionCreateFunc(CONVEX_2D_SHAPE_PROXYTYPE,CONVEX_2D_SHAPE_PROXYTYPE,convexAlgo2d);
			this->m_dispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE,CONVEX_2D_SHAPE_PROXYTYPE,convexAlgo2d);
			this->m_dispatcher->registerCollisionCreateFunc(CONVEX_2D_SHAPE_PROXYTYPE,BOX_2D_SHAPE_PROXYTYPE,convexAlgo2d);
			this->m_dispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE,BOX_2D_SHAPE_PROXYTYPE,new btBox2dBox2dCollisionAlgorithm::CreateFunc());*/
			
			this->m_solver = new btSequentialImpulseConstraintSolver;
			this->m_dynamicsWorld = new btDiscreteDynamicsWorld(this->m_dispatcher,
																this->m_broadphase,
																this->m_solver,
																this->m_collisionConfiguration);
			
			this->m_dynamicsWorld->setGravity(btVector3(0,-10,0));
		}
		
		void qWorld::update(const float &dt)
		{
			this->m_dynamicsWorld->stepSimulation(dt);
		}
		
		btRigidBody *qWorld::createRigidBody(float mass, const btTransform &startTransform, btCollisionShape *shape)
		{
			btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));
			
			//rigidbody is dynamic if and only if mass is non zero, otherwise static
			bool isDynamic = (mass != 0.f);
			
			btVector3 localInertia(0,0,0);
			if (isDynamic)
				shape->calculateLocalInertia(mass,localInertia);
			
			//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
			
			btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
			
			btRigidBody::btRigidBodyConstructionInfo cInfo(mass,myMotionState,shape,localInertia);
			
			btRigidBody* body = new btRigidBody(cInfo);
			body->setContactProcessingThreshold(m_defaultContactProcessingThreshold);
			
			m_dynamicsWorld->addRigidBody(body);
			
			return body;
		}
	}
}