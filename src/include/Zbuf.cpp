#include "Zbuf.hpp"
#include "glm/gtc/type_ptr.hpp"

Zbuf::Zbuf() { this->_init(); }
Zbuf::Zbuf(Scene const &s) : scene{s} { this->_init(); }
Zbuf::Zbuf(Scene const &s, size_t const &width, size_t const &height)
    : scene{s} {
    this->_init();
    this->init_viewport(width, height);
}

Image const &Zbuf::image() const { return this->img; }

void Zbuf::reset() {
    this->img.fill();
    this->zpyramid.clear(this->zpyramid.root);
}

void Zbuf::set_shader(
    std::function<Color(Triangle const &t, Triangle const &v,
                        std::tuple<flt, flt, flt> const &barycentric)>
        shader_func) {
    this->frag_shader = shader_func;
}

void Zbuf::init_cam(vec3 const &ey, flt const &fovy, flt const &aspect_ratio,
                    flt const &znear, flt const &zfar, vec3 const &gaze,
                    vec3 const &up) {
    this->cam.init(ey, fovy, aspect_ratio, znear, zfar, gaze, up);
    this->cam_initialized = true;
}
void Zbuf::init_cam(Camera const &camera) {
    this->cam             = camera;
    this->cam_initialized = true;
}

void Zbuf::set_model_transformation(mat4 const &model) {
    if (!this->cam_initialized) {
        errorm("Camera position is not initilized\n");
    }
    // Set model matrix
    this->model = model;

    // Set view matrix, according to camera information
    vec3 right = glm::normalize(glm::cross(cam.gaze(), cam.up()));
    // clang-format off
    flt trans_value[] = {
        1, 0, 0, -cam.pos().x,
        0, 1, 0, -cam.pos().y,
        0, 0, 1, -cam.pos().z,
        0, 0, 0,            1,
    };
    flt rot_value[] = {
              right.x,       right.y,       right.z, 0,
           cam.up().x,    cam.up().y,    cam.up().z, 0,
        -cam.gaze().x, -cam.gaze().y, -cam.gaze().z, 0,
                    0,             0,             0, 1,
    };
    // clang-format on
    this->view = glm::make_mat4(trans_value) * glm::make_mat4(rot_value);

    // Set projection matrix, according to fov, aspect ratio, etc.
    mat4 persp_ortho; // Squeezes the frustum into a rectangular box
    mat4 ortho_trans; // Centers the rectangular box at origin
    mat4 ortho_scale; // Scales the rectangular box to the canonical cube
    flt  n            = this->cam.znear();
    flt  f            = this->cam.zfar();
    flt  fovy         = this->cam.fovy() * degree;
    flt  aspect_ratio = this->cam.aspect_ratio();
    flt  screen_top   = std::tan(fovy / 2) * fabs(n);
    flt  screen_right = screen_top * aspect_ratio;
    // clang-format off
    flt po_value[] = { // values for persp_ortho
        n, 0,   0,    0,
        0, n,   0,    0,
        0, 0, n+f, -n*f,
        0, 0,   1,    0,
    };
    flt ot_value[] = { // values for ortho_trans
        1, 0, 0,        0,
        0, 1, 0,        0,
        0, 0, 1, -(n+f)/2,
        0, 0, 0,        1,
    };
    flt os_value[] = { // values for ortho_scale
        1/screen_right,            0,           0, 0,
                     0, 1/screen_top,           0, 0,
                     0,            0, 1/fabs(f-n), 0,
                     0,            0,           0, 1
    };
    // clang-format on
    persp_ortho      = glm::make_mat4(po_value);
    ortho_trans      = glm::make_mat4(ot_value);
    ortho_scale      = glm::make_mat4(os_value);
    this->projection = persp_ortho * ortho_trans * ortho_scale;

    // Set mvp
    this->mvp             = model * view * projection;
    this->mvp_initialized = true;
}

void Zbuf::init_viewport(const size_t &width, const size_t &height) {
    this->h = height;
    this->w = width;
    // clang-format off
    // flt viewport_value[] = {
    // Todo: use single initilization array
    flt vtrans_value[] = {
        1, 0, 0, 1,
        0, 1, 0, 1,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    flt vscale_value[] = {
        w*0.5,     0, 0, 0,
            0, h*0.5, 0, 0,
            0,     0, 1, 0,
            0,     0, 0, 1,
    };
    // clang-format on
    mat4 vtrans = glm::make_mat4(vtrans_value);
    mat4 vscale = glm::make_mat4(vscale_value);
    // viewport         = vscale * vtrans;
    this->viewport = vtrans * vscale;
    this->img.init(this->w, this->h);
    // Initilize the depth buffer, initial values are infinitely far (negative
    // infinity).
    this->zpyramid             = Pyramid(this->h, this->w);
    this->viewport_initialized = true;
}

void Zbuf::render(rendering_method const &type) {
    if (!this->cam_initialized) {
        errorm("Camera position is not initilized\n");
    }
    if (!this->mvp_initialized) {
        errorm("Transformation matrices are not initialized\n");
    }
    if (!this->viewport_initialized) {
        errorm("Viewport size is not initialized\n");
    }
    if (type == rendering_method::octree) {
        // 改:add FILE* file
        // modify: index data saved in out_index.obj
    	FILE* file = fopen("./out_index.obj", "w");
        if (!file) {
            printf("write_obj: can't write data file \"%s\".\n", "./out_index_in.obj");
            system("PAUSE");
            exit(0);
        }
        FILE* file2 = fopen("./out_index_culled.obj", "w");
		if (!file2) {
			printf("write_obj: can't write data file \"%s\".\n", "./out_index_culled.obj");
			system("PAUSE");
			exit(0);
		}
        this->_render_with_octree(this->scene.root,file,file2);
    } else {
        this->scene.to_viewspace(this->mvp, this->cam.gaze());
        for (Triangle const &v : this->scene.primitives()) {
            if (type == rendering_method::zpyramid) {
                this->_draw_triangle_with_zpyramid(v);
            } else if (type == rendering_method::naive) {
                this->_draw_triangle_with_aabb(v);
            } else {
                errorm("Unhandled rendering method encountered\n");
            }
        }
    }
}

// private:
void Zbuf::_init() {
    this->cam_initialized      = false;
    this->mvp_initialized      = false;
    this->viewport_initialized = false;
    this->frag_shader          = nullptr;
}

bool Zbuf::inside(flt x, flt y, Triangle const &t) const {
    return t.contains(x, y);
}

void Zbuf::set_pixel(size_t const &x, size_t const &y, Color const &color) {
    this->img(x, y) = color;
}

flt &Zbuf::z(size_t const &x, size_t const &y) {
    return this->zpyramid(x, y);
}

flt const &Zbuf::z(size_t const &x, size_t const &y) const {
    return this->zpyramid(x, y);
}

void Zbuf::_draw_triangle_with_aabb(Triangle const &v) {
    // Triangle with screen-space coordinates
    Triangle t(v * viewport);
    // AABB
    int xmin = std::floor(std::min(t.a().x, std::min(t.b().x, t.c().x)));
    int xmax = std::ceil(std::max(t.a().x, std::max(t.b().x, t.c().x)));
    int ymin = std::floor(std::min(t.a().y, std::min(t.b().y, t.c().y)));
    int ymax = std::ceil(std::max(t.a().y, std::max(t.b().y, t.c().y)));
    xmin = clamp(xmin, 0, w - 1), xmax = clamp(xmax, 0, w - 1);
    ymin = clamp(ymin, 0, h - 1), ymax = clamp(ymax, 0, h - 1);
    for (int j = ymin; j < ymax; ++j) {
        for (int i = xmin; i < xmax; ++i) {
            // todo: AA
            flt x = .5 + i;
            flt y = .5 + j;
            if (this->inside(x, y, t)) {
                // Screen space barycentric coordinates of (x, y) inside
                // triangle t.
                std::tuple<flt, flt, flt> barycentric = t % vec3{x, y, 0};
                // unpack the barycentric coordinates
                auto [ca, cb, cc] = barycentric;
                // z value in view-space
                flt real_z = 1 / (ca / v.a().z + cb / v.b().z + cc / v.c().z);
                if (real_z > this->z(i, j)) {
                    // Calculate interpolated color with given triangle's 3
                    // vertices.
                    // Note: t and v shall have same color values by now.
                    // Color icol    = v.color_at(ca, cb, cc, real_z);
                    Color icol    = this->frag_shader(t, v, barycentric);
                    this->z(i, j) = real_z;
                    this->set_pixel(i, j, icol);
                }
            }
        }
    }
}

// 改：返回值bool
bool Zbuf::_draw_triangle_with_zpyramid(Triangle const &v) {
    bool ret = false;
    // Triangle with screen-space coordinates
    Triangle t(v * viewport);
    if (this->zpyramid.visible(t, nullptr)) {
    	ret = true;
        // AABB
        /* 
        int xmin = std::floor(std::min(t.a().x, std::min(t.b().x, t.c().x)));
        int xmax = std::ceil(std::max(t.a().x, std::max(t.b().x, t.c().x)));
        int ymin = std::floor(std::min(t.a().y, std::min(t.b().y, t.c().y)));
        int ymax = std::ceil(std::max(t.a().y, std::max(t.b().y, t.c().y)));
        xmin = clamp(xmin, 0, w), xmax = clamp(xmax, 0, w);
        ymin = clamp(ymin, 0, h), ymax = clamp(ymax, 0, h);
        for (int j = ymin; j < ymax; ++j) {
            for (int i = xmin; i < xmax; ++i) {
                // todo: AA
                flt x = .5 + i;
                flt y = .5 + j;
                if (this->inside(x, y, t)) {
                    // Screen space barycentric coordinates of (x, y) inside
                    // triangle t.
                    std::tuple<flt, flt, flt> barycentric = t % vec3{x, y, 0};
                    // unpack the barycentric coordinates
                    auto [ca, cb, cc] = barycentric;
                    // z value in view-space
                    flt real_z =
                        1 / (ca / v.a().z + cb / v.b().z + cc / v.c().z);
                    if (real_z > this->z(i, j)) {
                        // Calculate interpolated color with given triangle's
                        // 3 vertices. Note: t and v shall have same color
                        // values by now. Color icol    = v.color_at(ca, cb,
                        // cc, real_z);
                        Color icol = this->frag_shader(t, v, barycentric);
                        this->zpyramid.setz(i, j, real_z);
                        this->set_pixel(i, j, icol);
                    }
                }
            }
        }
        
        */
    }
    return ret;
}

// node是scene->root
// 改：去掉 const
void Zbuf::_render_with_octree(Node8 *node,FILE* file,FILE* file2) {
    std::vector<Triangle> facets;
    // Convert to view space coordinates
    for (int i = 0; i < 12; ++i) {
        // Face culling
        if (glm::dot(this->cam.gaze(), node->facets[i].facing) >= 0) {
            continue;
        }
        facets.push_back(node->facets[i] * mvp);
    }
    printf("%d ",facets.size());
    // Check if this cube intersects with the view frustum (view frustum
    // culling)
    // Note: this is not deterministic but quite enough
    bool invisible{true};
    for (auto const &v : facets) {
        for (int i = 0; i < 3; ++i) {
            if (v.vert_in_canonical()) {
                invisible = false;
                break;
            }
        }
    }
    // When the cube does not intersects with the view frustum, it can be
    // safely ignored.
    if (invisible) {
        printf("continue ");
        return;
    }
    // When the cube does intersect with the view frustum, render the
    // triangles associated with it, and dive into its child nodes.
    // 改：去掉const &
    for (Triangle t : node->prims) {
        // Face culling
        //fprintf(file, "f %d// %d// %d//\n",t.index_a+1,t.index_b+1,t.index_c+1);
        if (glm::dot(this->cam.gaze(), t.facing) >= 0) {
            t.deleted = true ;
            this->scene.realworld_triangles[t.indexOfTriangles].deleted = true ;
            fprintf(file2, "f %d// %d// %d//\n",t.index_a+1,t.index_b+1,t.index_c+1);
            continue;
        }
        // Convert to view space
        Triangle v = t * this->mvp;
        // View frustum culling
        for (int i = 0; i < 3; ++i) {
            if (v.vert_in_canonical()) {
                // this->_draw_triangle_with_zpyramid(v);
                // 改：加判断
                if(this->_draw_triangle_with_zpyramid(v)){
                	//printf("%d %d %d \n",t.index_a,t.index_b,t.index_c);
                	
                    fprintf(file, "f %d// %d// %d//\n",t.index_a+1,t.index_b+1,t.index_c+1);
                }
                else{
                     printf("in ");
                     t.deleted = true ;
                     this->scene.realworld_triangles[t.indexOfTriangles].deleted = true ;
                     fprintf(file2, "f %d// %d// %d//\n",t.index_a+1,t.index_b+1,t.index_c+1);
                }
                
                break;
            }
        }
    }
    // Recurse into child nodes. // 8个children 
    for (Node8 *child : node->children) {
        if (child == nullptr) {
            continue;
        }
        this->_render_with_octree(child,file,file2);
    }
}

// Author: Blurgy <gy@blurgy.xyz>
// Date:   Nov 24 2020, 12:15 [CST]
