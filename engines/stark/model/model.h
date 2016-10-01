/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef STARK_MODEL_MODEL_H
#define STARK_MODEL_MODEL_H

#include "common/array.h"
#include "common/str.h"

#include "math/ray.h"
#include "math/vector3d.h"

namespace Stark {

namespace Gfx {
class TextureSet;
}

class ArchiveReadStream;

class VertNode {
public:
	Math::Vector3d _pos1, _pos2;
	Math::Vector3d _normal;
	float _texS, _texT;
	uint32 _bone1, _bone2;
	float _boneWeight;
};

class TriNode {
public:
	uint32 _vert1, _vert2, _vert3;
};

class FaceNode {
public:
	FaceNode() : _matIdx(0) {}

	~FaceNode() {
		for (Common::Array<VertNode *>::iterator it = _verts.begin(); it != _verts.end(); ++it)
			delete *it;

		for (Common::Array<TriNode *>::iterator it = _tris.begin(); it != _tris.end(); ++it)
			delete *it;
	}

	uint32 _matIdx;
	Common::Array<VertNode *> _verts;
	Common::Array<TriNode *> _tris;
};

class MeshNode {
public:
	MeshNode() {}
	~MeshNode() {
		Common::Array<FaceNode *>::iterator it = _faces.begin();
		while (it != _faces.end()) {
			delete *it;
			++it;
		}

	}
	Common::String _name;
	Common::Array<FaceNode *> _faces;
};

class MaterialNode {
public:
	Common::String _name;
	uint32 _unknown1;
	Common::String _texName;
	float _r, _g, _b;
};

class BoneNode {
public:
	BoneNode() : _parent(-1), _idx(0), _u1(0) {}
	~BoneNode() { }

	/** Perform a collision test with the ray */
	bool intersectRay(const Math::Ray &ray) const;

	Common::String _name;
	float _u1;
	Common::Array<uint32> _children;
	int _parent;
	uint32 _idx;

	Math::Vector3d _animPos;
	Math::Quaternion _animRot;

	/** Bone space bounding box */
	Math::AABB _boundingBox;
};

/**
 * A 3D Model
 */
class Model {
public:
	Model();
	~Model();

	/**
	 * Try and initialise object from the specified stream
	 */
	void readFromStream(ArchiveReadStream *stream);

	const Common::Array<MeshNode *> &getMeshes() const { return _meshes; }
	const Common::Array<MaterialNode *> &getMaterials() const { return _materials; }
	const Common::Array<BoneNode *> &getBones() const { return _bones; };

	/** Perform a collision test with a ray */
	bool intersectRay(const Math::Ray &ray) const;

private:
	void buildBonesBoundingBoxes();
	void buildBoneBoundingBox(BoneNode *bone) const;
	void readBones(ArchiveReadStream *stream);

	uint32 _u1;
	float _u2;

	Common::Array<MaterialNode *> _materials;
	Common::Array<MeshNode *> _meshes;
	Common::Array<BoneNode *> _bones;
};

} // End of namespace Stark

#endif // STARK_MODEL_MODEL_H
