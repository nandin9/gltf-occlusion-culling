#pragma once

#include <array>
#include <tuple>

#include "global.hpp"

#include <glm/gtc/type_ptr.hpp>

class Triangle {
  public:
    std::array<vec3, 3> v; // Spatial coordinate of the 3 vertices, in
                           // counter-clockwise order
    vec3 facing; // Facing direction (for face culling), computed by:
                 // glm::cross(v[1] - v[0], v[2] - v[1]);
    std::array<vec3, 3>  nor; // Normal directions of the 3 vertices
    std::array<vec2, 3>  tex; // Texture coordinates of the 3 vertices
    std::array<Color, 3> col; // Color values of the 3 vertices

    bool has_material;
    int  matid;

    // Bounding box of this triangle.
    BBox bbox;

  private:
    void _init();

  public:
    Triangle();
    // Assumes indices is {0, 1, 2}
    Triangle(vec3 const &a, vec3 const &b, vec3 const &c, // Spatial positions
             vec3 const &na = vec3{0}, vec3 const &nb = vec3{0},
             vec3 const &nc = vec3{0}, // Normals
             vec2 const &ta = vec2{0}, vec2 const &tb = vec2{0},
             vec2 const & tc = vec2{0}, // Texture coordinates
             Color const &ca = Color{0}, Color const &cb = Color{0},
             Color const &cc = Color{0} // Colors
    );
    // 改5、新建构造函数
	Triangle(vec3 const& a, vec3 const& b, vec3 const& c, 
		int const& i1,int const& i2, int const& i3, 
		vec3 const& na = vec3{ 0 }, vec3 const& nb = vec3{ 0 },
		vec3 const& nc = vec3{ 0 }, // Normals
		vec2 const& ta = vec2{ 0 }, vec2 const& tb = vec2{ 0 },
		vec2 const& tc = vec2{ 0 }, // Texture coordinates
		Color const& ca = Color{ 0 }, Color const& cb = Color{ 0 },
		Color const& cc = Color{ 0 } // Colors
	);
	
    Triangle(std::array<vec3, 3> const &vtx,  // Spatial positions
             std::array<vec3, 3> const &nor = // Normals
             {
                 vec3{0},
                 vec3{0},
                 vec3{0},
             },
             std::array<vec2, 3> const &tex = // Texture coordinates
             {
                 vec2{0},
                 vec2{0},
                 vec2{0},
             },
             std::array<Color, 3> const &col = // Colors
             {
                 Color{0},
                 Color{0},
                 Color{0},
             });
    // Set material
    void set_material(int const &mat_id);

    vec3 const &a() const; // Returns spatial location of the first vertex
    vec3 const &b() const; // Returns spatial location of the second vertex
    vec3 const &c() const; // Returns spatial location of the third vertex
    
    // 改1、添加 索引 
	long index_a; 
	long index_b;
	long index_c;
	// 改2、添加是否被删除 ：true被删除，默认是false
	bool deleted  = false ;
  int indexOfTriangles ;
  // 改 添加： 三角形属于的mesh name
  std::string meshName = "" ;

    // Returns normal direction of the first vertex
    vec3 const &na() const;
    // Returns normal direction of the second vertex
    vec3 const &nb() const;
    // Returns normal direction of the third vertex
    vec3 const &nc() const;

    // Returns color value of the first vertex
    Color const &ca() const;
    // Returns color value of the second vertex
    Color const &cb() const;
    // Returns color value of the third vertex
    Color const &cc() const;

    BBox const &boundingbox() const;

    int const &material() const;

    // Returns area of the triangle's orthographic projection onto the xOy
    // plane (computes slightly slower than function Triangle::doublearea()).
    flt area() const;
    // Returns area doubled of the triangle's orthographic projection onto the
    // xOy plane.
    flt doublearea() const;

    // Determine whether this triangle has a vertex inside the canonical box.
    bool vert_in_canonical() const;

    bool contains(flt x, flt y) const;
    // When calling this function, make sure the triangle has **view-space**
    // coordinates (as opposed to screen-space coordinates).
    Color color_at(flt const &ca, flt const &cb, flt const &cc,
                   flt const &z_viewspace) const;

  public: // Operator overrides
    Triangle                  operator*(mat4 const &m) const;
    Triangle friend           operator*(mat4 const &m, Triangle const &rhs);
    std::tuple<flt, flt, flt> operator%(vec3 const &pos) const;
};

// Author: Blurgy <gy@blurgy.xyz>
// Date:   Nov 23 2020, 13:20 [CST]
