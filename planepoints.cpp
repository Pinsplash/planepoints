//Trigger outline display generator for Titanfall 2 maps.
//This code creates a series of console commands which can be run in VanillaPlus (or some similar mod) to create a simple outline of trigger entities.
//If you want to view triggers in Respawn's T2 maps, you don't need to run this. The files already exist elsewhere in this repository.
//Created by Pinsplash, much help from OxzyBox.
#include <stdio.h>
#include <math.h>
//#include <cmath.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

// Vector 3
struct Vector3
{
	float x = 0;
	float y = 0;
	float z = 0;

	// This just exists for compat with the std::vector code
	float& operator[](int i) {
		return reinterpret_cast<float*>(this)[i];
	}
	const float& operator[](int i) const {
		return reinterpret_cast<const float*>(this)[i];
	}
};

Vector3 operator+ (const Vector3& l, const Vector3& r)
{
	return { l.x + r.x, l.y + r.y, l.z + r.z };
}
Vector3 operator* (const Vector3& l, float r)
{
	return { l.x * r, l.y * r, l.z * r };
}
Vector3& operator+= (Vector3& l, const Vector3& r)
{
	l.x += r.x;
	l.y += r.y;
	l.z += r.z;
	return l;
}
Vector3& operator*= (Vector3& l, float r)
{
	l.x *= r;
	l.y *= r;
	l.z *= r;
	return l;
}

// Vector 4
struct Vector4
{
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;

	// This just exists for compat with the std::vector code
	float& operator[](int i) {
		return reinterpret_cast<float*>(this)[i];
	}
	const float& operator[](int i) const {
		return reinterpret_cast<const float*>(this)[i];
	}
};

Vector4 operator+ (const Vector4& l, const Vector4& r)
{
	return { l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w };
}
Vector4 operator* (const Vector4& l, float r)
{
	return { l.x * r, l.y * r, l.z * r, l.w * r };
}
Vector4& operator+= (Vector4& l, const Vector4& r)
{
	l.x += r.x;
	l.y += r.y;
	l.z += r.z;
	l.w += r.w;
	return l;
}
Vector4& operator*= (Vector4& l, float r)
{
	l.x *= r;
	l.y *= r;
	l.z *= r;
	l.w *= r;
	return l;
}
Vector4& operator/= (Vector4& l, float r)
{
	l.x /= r;
	l.y /= r;
	l.z /= r;
	l.w /= r;
	return l;
}

// Matrix 3x3
struct Matrix3x3
{
	Vector3 a;
	Vector3 b;
	Vector3 c;
};

struct Plane
{
	Vector3 normal;
	float dist = 0;
};

struct Edge
{
	// Stem is the starting position, tail is the ending position
	// I just... want the equal spacing...
	Vector3 stem;
	Vector3 tail;
};

struct Brush
{
	std::vector<Plane> planes;
	std::vector<Edge> edges;
};

struct Entity
{
	std::string editorclass;
	std::string classname;

	Vector3 origin;
	Vector3 mins;
	Vector3 maxs;

	std::vector<Brush> brushes;
};

Vector3 crossProduct(const Vector3& l, const Vector3& r)
{
	return { l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x };
}

float dotProduct(const Vector3& l, const Vector3& r)
{
	return l.x * r.x + l.y * r.y + l.z * r.z;
}


bool mat3x3_solve(const Matrix3x3& A, const Vector3& b, Vector3* pOut)
{
	// Transpose from column major to row major and augment in b
	float omat[3][4] = {
		{A.a.x, A.b.x, A.c.x, b.x},
		{A.a.y, A.b.y, A.c.y, b.y},
		{A.a.z, A.b.z, A.c.z, b.z},
	};

	// Keep the matrix as pointers so we can swap rows quickly without damaging our augment in omat
	float* mat[3] = {
		&omat[0][0],
		&omat[1][0],
		&omat[2][0],
	};

	// Get it into row echelon form
	for (int n = 0; n < 2; n++)
	{
		// Find pivots
		float largest = 0;
		float la = 0;
		int pivrow = -1;
		for (int m = n; m < 3; m++)
		{
			float v = mat[m][n];
			float va = fabsf(v);

			if (va > la)
			{
				pivrow = m;
				largest = v;
				la = va;
			}
		}

		// No pivot? No solution!
		if (pivrow == -1)
			return false;

		// Swap pivot to highest
		float* pivot = mat[pivrow];
		mat[pivrow] = mat[n];
		mat[n] = pivot;

		Vector4* pivotv = reinterpret_cast<Vector4*>(pivot);

		// Apply our pivot row to the rows below 
		for (int m = n + 1; m < 3; m++)
		{
			// Get the multiplier
			float* row = mat[m];
			float v = -row[n] / pivot[n];

			Vector4* rowv = reinterpret_cast<Vector4*>(row);
			*rowv += *pivotv * v;
		}
	}

	// Get it into reduced row echelon form
	for (int n = 2; n; n--)
	{
		for (int m = n - 1; m >= 0; m--)
		{
			float* pivot = mat[n];
			Vector4* pivotv = reinterpret_cast<Vector4*>(pivot);

			// Get the multiplier
			float* row = mat[m];
			float v = -row[n] / pivot[n];

			// Push that pivot up
			Vector4* rowv = reinterpret_cast<Vector4*>(row);
			*rowv += *pivotv * v;
		}
	}

	// Clean up our diagonal
	for (int n = 0; n < 3; n++)
	{
		float* rowf = mat[n];
		float v = rowf[n];

		// Check for zeros along the diagonal
		if (fabsf(v) <= FLT_EPSILON)
			return false;

		Vector4* row = reinterpret_cast<Vector4*>(rowf);
		*row /= v;
	}

	// Hoist the augment back off mat
	Vector3 v = { mat[0][3], mat[1][3], mat[2][3] };
	*pOut = v;
	return true;
}

bool PlaneIntersect(const Plane& plane1, const Plane& plane2, const Plane& plane3, Vector3* pOut)
{
	Vector3 n1 = plane1.normal;
	Vector3 n2 = plane2.normal;
	Vector3 n3 = plane3.normal;

	// Push all norms into the rows, transposed
	Matrix3x3 n = {
		{n1.x, n2.x, n3.x},
		{n1.y, n2.y, n3.y},
		{n1.z, n2.z, n3.z},
	};

	// The dist is the augment
	Vector3 d = { plane1.dist, plane2.dist, plane3.dist };

	// Solve x for n*x=d
	Vector3 i;
	if (mat3x3_solve(n, d, &i))
	{
		*pOut = i;
		return true;
	}

	return false;
}

Vector3 ParseVector(const std::string& str)
{
	// Parse the vector 
	Vector3 v;
	sscanf_s(str.c_str(), "%f %f %f", &v.x, &v.y, &v.z);
	return v;
}

Plane ParsePlane(const std::string& str)
{
	// Parse the plane
	Plane plane;
	sscanf_s(str.c_str(), "%f %f %f %f", &plane.normal.x, &plane.normal.y, &plane.normal.z, &plane.dist);
	return plane;
}

void ParseFile(std::ifstream& ReadFile, std::vector<Entity>& entities)
{
	std::string textLine;

	Entity newEntity;

	while (getline(ReadFile, textLine))
	{
		// Skip any blank lines
		if (textLine.size() == 0)
			continue;

		if (textLine[0] == '{')
		{
			// Start of entity
			// Clear out the entity
			newEntity = {};
			continue;
		}

		if (textLine[0] == '}')
		{
			// End of entity
			// Commit the entity
			entities.push_back(newEntity);
			continue;
		}

		// Everything here has a key and a value. Both the key and the value are surrounded by double quotes
		// Find everything between the pairs of double quotes
		// The + 1 is to move us over the double quote
		size_t keyStart = textLine.find('"');
		size_t keyEnd = textLine.find('"', keyStart + 1);
		size_t valueStart = textLine.find('"', keyEnd + 1);
		size_t valueEnd = textLine.find('"', valueStart + 1);

		// Move the string start over the double quote
		keyStart++;
		valueStart++;

		std::string key = textLine.substr(keyStart, keyEnd - keyStart);
		std::string value = textLine.substr(valueStart, valueEnd - valueStart);

		if (key == "editorclass")
		{
			std::cout << "Found editor class " << value << "\n";
			newEntity.editorclass = value;
		}
		else if (key == "origin")
		{
			newEntity.origin = ParseVector(value);
			std::cout << "Found origin " << newEntity.origin.x << " " << newEntity.origin.y << " " << newEntity.origin.z << "\n";
		}
		else if (key == "classname")
		{
			std::cout << "Found classname " << value << "\n";
			newEntity.classname = value;
		}
		else if (key.find("*trigger_brush_") != std::string::npos)
		{
			//std::cout << "string " << key << "\n";
			const std::string token1 = "*trigger_brush_";
			size_t brushDigitsPos = key.find("_", token1.length());//some triggers have 10+ brushes
			size_t brushNumDigits = brushDigitsPos - token1.length();
			int iBrush = stoi(key.substr(token1.length(), brushNumDigits));
			//std::cout << "brush " << brush << ", ";

			const std::string token2 = "_plane_";
			int iPlane = stoi(key.substr(token1.length() + brushNumDigits + token2.length()));
			//std::cout << "iPlane " << iPlane << ", ";

			// Parse the plane
			Plane plane = ParsePlane(value);

			// Normally, I'd just use pushback, but these have IDs soooo idk

			// Make room for the brush if we haven't yet
			if (newEntity.brushes.size() <= iBrush)
				newEntity.brushes.resize(iBrush + 1);

			// Grab the brush
			Brush& brush = newEntity.brushes[iBrush];

			// Make room for the plane if we haven't yet
			if (brush.planes.size() <= iPlane)
				brush.planes.resize(iPlane + 1);

			// Set the plane
			brush.planes[iPlane] = plane;
		}
		else if (key == "*trigger_bounds_mins")
		{
			newEntity.mins = ParseVector(value);
		}
		else if (key == "*trigger_bounds_maxs")
		{
			newEntity.maxs = ParseVector(value);
		}
	}
}

constexpr float k_flEpsilon = 0.0001f;


int main()
{
	std::vector<Entity> entities;

	//read entity data
	std::ifstream ReadFile("filename.txt");
	ParseFile(ReadFile, entities);
	ReadFile.close();

	//get line from two intersecting planes
	//every plane in a brush must be checked against all others in the brush
	for (Entity& ent : entities)
	{
		for (Brush& brush : ent.brushes)
		{
			int nPlanes = brush.planes.size();

			// We need at least 4 plans for a brush
			if (nPlanes < 4)
			{
				std::cout << "Less than 4 planes!\n";
				continue;
			}

			// Get all plane intersections
			for (int iPlane1 = 0; iPlane1 < nPlanes; iPlane1++)
			{
				Plane& plane1 = brush.planes[iPlane1];
				for (int iPlane2 = iPlane1 + 1; iPlane2 < nPlanes; iPlane2++)
				{
					Plane& plane2 = brush.planes[iPlane2];

					// Are 1 and 2 opposing and/or parallel? If so, there will never be a intersection
					float f = dotProduct(plane1.normal, plane2.normal);
					if (fabs(fabs(f) - 1.0) < k_flEpsilon) // f is near -1.0 or 1.0
						continue;

					// Planes 1 and 2 share an edge
					Edge edge;
					int intersectCount = 0;

					// Loop for the other remaining untested planes
					for (int iPlane3 = iPlane2 + 1; iPlane3 < nPlanes; iPlane3++)
					{
						Plane& plane3 = brush.planes[iPlane3];

						// Do our 3 planes intersect? If not, next plane.
						Vector3 p;
						if (!PlaneIntersect(plane1, plane2, plane3, &p))
							continue;
						// Hit!

						// Cull points outside of the solid
						bool in = true;
						for (Plane& k : brush.planes)
						{
							// Skip the planes we're currently working on
							if (&k == &plane1 || &k == &plane2 || &k == &plane3)
								continue;

							// Get the dist of the point for the plane normal
							if (dotProduct(p, k.normal) - k.dist > k_flEpsilon)
							{
								in = false;
								break;
							}
						}

						// Out of the solid. Skip it
						if (!in)
							continue;

						// This one's good! Store it onto the edge
						if (intersectCount == 0)
							edge.stem = p;
						else
							edge.tail = p;
						intersectCount++;

						if (intersectCount == 2)
						{
							// Completed edge!
							brush.edges.push_back(edge);
							break;
						}
					}

					if (intersectCount == 2)
					{
						// Completed edge!
						continue;
					}
				}
			}
		}
	}

	//write drawlines
	for (Entity& ent : entities)
	{
		for (Brush& brush : ent.brushes)
		{
			//std::cout << "\n";
			for (Edge& edge : brush.edges)
			{
				Vector3 stem = ent.origin + edge.stem;
				Vector3 tail = ent.origin + edge.tail;
#if 1
				std::cout << "script_client DebugDrawLine("
					<< "Vector(" << stem.x << ", " << stem.y << ", " << stem.z << "), "
					<< "Vector(" << tail.x << ", " << tail.y << ", " << tail.z << "), "
					<< "255, 255, 255, false, 60);\n";
#else
				// Desmos 3D lol
				std::cout << "["
					<< "(" << stem.x << ", " << stem.y << ", " << stem.z << "), "
					<< "(" << tail.x << ", " << tail.y << ", " << tail.z << ")"
					<< "]\n";
#endif
			}
		}
	}
}