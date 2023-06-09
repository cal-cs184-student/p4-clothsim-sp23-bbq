#include <iostream>
#include <math.h>
#include <random>
#include <vector>

#include "cloth.h"
#include "collision/plane.h"
#include "collision/sphere.h"

using namespace std;

Cloth::Cloth(double width, double height, int num_width_points,
             int num_height_points, float thickness) {
  this->width = width;
  this->height = height;
  this->num_width_points = num_width_points;
  this->num_height_points = num_height_points;
  this->thickness = thickness;

  buildGrid();
  buildClothMesh();
}

Cloth::~Cloth() {
  point_masses.clear();
  springs.clear();

  if (clothMesh) {
    delete clothMesh;
  }
}

void Cloth::buildGrid() {
  // TODO (Part 1): Build a grid of masses and springs.
    double width_size = width / num_width_points;
    double height_size = height / num_height_points;

    for (int y = 0; y < num_height_points; y++) {
        for (int x = 0; x < num_width_points; x++) {

            bool is_pinned = false;
            for (auto pin_iter : pinned) {
                if (pin_iter[0] == x && pin_iter[1] == y) {
                    is_pinned = true;
                    break;
                }
            }

            Vector3D coord;
            if (orientation == HORIZONTAL) {
                coord = Vector3D(width_size * x, 1.0, height_size * y);
            }
            else {
                double z = ((double) rand() / (double) RAND_MAX) * 0.002 - 0.001;
                coord = Vector3D(width_size * x, height_size * y, z);
            }

            point_masses.emplace_back(PointMass(coord, is_pinned));

        }
    }

    for (int y = 0; y < num_height_points; y++) {
        for (int x = 0; x < num_width_points; x++) {
            int curr = y * num_width_points + x;
            int next;
            // Structural
            if (x > 0) {
                next = y * num_width_points + x - 1;
                Spring s = Spring(&point_masses[curr], &point_masses[next], STRUCTURAL);
                springs.emplace_back(s);
            }
            if (y > 0) {
                next = (y - 1) * num_width_points + x;
                Spring s = Spring(&point_masses[curr], &point_masses[next], STRUCTURAL);
                springs.emplace_back(s);
            }
            // Shearing
            if (x > 0 && y > 0) {
                next = (y - 1) * num_width_points + x - 1;
                Spring s = Spring(&point_masses[curr], &point_masses[next], SHEARING);
                springs.emplace_back(s);
            }
            if (x < num_width_points - 1 && y > 0) {
                next = (y - 1) * num_width_points + x + 1;
                Spring s = Spring(&point_masses[curr], &point_masses[next], SHEARING);
                springs.emplace_back(s);
            }
            // Bending
            if (x > 1) {
                next = y * num_width_points + x - 2;
                Spring s = Spring(&point_masses[curr], &point_masses[next], BENDING);
                springs.emplace_back(s);
            }
            if (y > 1) {
                next = (y - 2) * num_width_points + x;
                Spring s = Spring(&point_masses[curr], &point_masses[next], BENDING);
                springs.emplace_back(s);
            }
        }
    }

}

void Cloth::simulate(double frames_per_sec, double simulation_steps, ClothParameters *cp,
                     vector<Vector3D> external_accelerations,
                     vector<CollisionObject *> *collision_objects) {
  double mass = width * height * cp->density / num_width_points / num_height_points;
  double delta_t = 1.0f / frames_per_sec / simulation_steps;

  // TODO (Part 2): Compute total force acting on each point mass.
  Vector3D total_external_force = Vector3D();
  for (auto const &external_acceleration : external_accelerations) {
      total_external_force += mass * external_acceleration;
  }
  for (auto &point_mass : point_masses) {
      point_mass.forces = total_external_force;
  }
  for (auto &spring : springs) {
      if ((spring.spring_type == STRUCTURAL && cp->enable_structural_constraints) ||
          (spring.spring_type == BENDING && cp->enable_bending_constraints) ||
          (spring.spring_type == SHEARING && cp->enable_shearing_constraints)) {
          double ks = spring.spring_type == BENDING ? cp->ks * 0.2 : cp->ks;
          Vector3D Fs = (spring.pm_b->position - spring.pm_a->position).unit();
          Fs *= ks * ((spring.pm_a->position - spring.pm_b->position).norm() - spring.rest_length);
          spring.pm_a->forces += Fs;
          spring.pm_b->forces -= Fs;
      }
  }

  // TODO (Part 2): Use Verlet integration to compute new point mass positions
  for (auto &point_mass : point_masses) {
      if (!point_mass.pinned) {
          Vector3D at = point_mass.forces / mass;
          Vector3D xtdt = point_mass.position + (1 - cp->damping / 100.0) * (point_mass.position - point_mass.last_position) + at * delta_t * delta_t;
          point_mass.last_position = point_mass.position;
          point_mass.position = xtdt;
      }
  }

  // TODO (Part 4): Handle self-collisions.
  build_spatial_map();
  for (auto &point_mass : point_masses) {
      self_collide(point_mass, simulation_steps);
  }


  // TODO (Part 3): Handle collisions with other primitives.
  for (auto &point_mass : point_masses) {
      for (auto collision_object : *collision_objects) {
          collision_object->collide(point_mass);
      }
  }

  // TODO (Part 2): Constrain the changes to be such that the spring does not change
  // in length more than 10% per timestep [Provot 1995].
  for (auto &spring : springs) {
      Vector3D direction = (spring.pm_b->position - spring.pm_a->position).unit();
      double orig_length = (spring.pm_b->position - spring.pm_a->position).norm();
      if (orig_length > spring.rest_length * 1.10) {
          Vector3D diff = (orig_length - spring.rest_length * 1.10) * direction;
          if (spring.pm_a->pinned && !spring.pm_b->pinned) {
              spring.pm_b->position -= diff;
          } else if (!spring.pm_a->pinned && spring.pm_b->pinned) {
              spring.pm_a->position += diff;
          } else if (!spring.pm_a->pinned && !spring.pm_b->pinned) {
              spring.pm_a->position += diff/2;
              spring.pm_b->position -= diff/2;
          }
      }
  }

}

void Cloth::build_spatial_map() {
  for (const auto &entry : map) {
    delete(entry.second);
  }
  map.clear();

  // TODO (Part 4): Build a spatial map out of all of the point masses.
  for (int i = 0; i < point_masses.size(); i++) {
      // if not in map, add pm to hashtable
      if (map.find(hash_position(point_masses[i].position)) == map.end()) {
          // no existing key
          vector<PointMass *> *new_v = new vector<PointMass *>;
          map[hash_position(point_masses[i].position)] = new_v;
      }
      map[hash_position(point_masses[i].position)]->push_back(&point_masses[i]);
  }
}

void Cloth::self_collide(PointMass &pm, double simulation_steps) {
  // TODO (Part 4): Handle self-collision for a given point mass.
  vector<PointMass *> *candidates = map[hash_position(pm.position)];

  Vector3D final_correction_vector = Vector3D();

  for (PointMass *p : *candidates) {
      // prevent pm self collision
      if (p->position == pm.position) {
          continue;
      }

      Vector3D d = pm.position - p->position;
      d.normalize();

      if ((p->position - pm.position).norm() <= 2.0 * thickness) {
          // correction vector
          final_correction_vector += (2.0 * thickness - (p->position - pm.position).norm()) * d;
      }
  }

  pm.position += (final_correction_vector / simulation_steps);
}

float Cloth::hash_position(Vector3D pos) {
  // TODO (Part 4): Hash a 3D position into a unique float identifier that represents membership in some 3D box volume.
  double box_width = 3.0 * this->width / num_width_points;
  double box_height = 3.0 * this->height / num_height_points;
  double box_t = max(box_height, box_width);

  Vector3D pos_new;
    pos_new.x = (pos.x - fmod(pos.x, box_width)) / box_width;
    pos_new.y = (pos.y - fmod(pos.y, box_height)) / box_height;
    pos_new.z = (pos.z - fmod(pos.z, box_t)) / box_t;

  return float(17 * pos_new.x + 17 / pos_new.y - pow(pos_new.z, 17));
}

///////////////////////////////////////////////////////
/// YOU DO NOT NEED TO REFER TO ANY CODE BELOW THIS ///
///////////////////////////////////////////////////////

void Cloth::reset() {
  PointMass *pm = &point_masses[0];
  for (int i = 0; i < point_masses.size(); i++) {
    pm->position = pm->start_position;
    pm->last_position = pm->start_position;
    pm++;
  }
}

void Cloth::buildClothMesh() {
  if (point_masses.size() == 0) return;

  ClothMesh *clothMesh = new ClothMesh();
  vector<Triangle *> triangles;

  // Create vector of triangles
  for (int y = 0; y < num_height_points - 1; y++) {
    for (int x = 0; x < num_width_points - 1; x++) {
      PointMass *pm = &point_masses[y * num_width_points + x];
      // Get neighboring point masses:
      /*                      *
       * pm_A -------- pm_B   *
       *             /        *
       *  |         /   |     *
       *  |        /    |     *
       *  |       /     |     *
       *  |      /      |     *
       *  |     /       |     *
       *  |    /        |     *
       *      /               *
       * pm_C -------- pm_D   *
       *                      *
       */
      
      float u_min = x;
      u_min /= num_width_points - 1;
      float u_max = x + 1;
      u_max /= num_width_points - 1;
      float v_min = y;
      v_min /= num_height_points - 1;
      float v_max = y + 1;
      v_max /= num_height_points - 1;
      
      PointMass *pm_A = pm                       ;
      PointMass *pm_B = pm                    + 1;
      PointMass *pm_C = pm + num_width_points    ;
      PointMass *pm_D = pm + num_width_points + 1;
      
      Vector3D uv_A = Vector3D(u_min, v_min, 0);
      Vector3D uv_B = Vector3D(u_max, v_min, 0);
      Vector3D uv_C = Vector3D(u_min, v_max, 0);
      Vector3D uv_D = Vector3D(u_max, v_max, 0);
      
      
      // Both triangles defined by vertices in counter-clockwise orientation
      triangles.push_back(new Triangle(pm_A, pm_C, pm_B, 
                                       uv_A, uv_C, uv_B));
      triangles.push_back(new Triangle(pm_B, pm_C, pm_D, 
                                       uv_B, uv_C, uv_D));
    }
  }

  // For each triangle in row-order, create 3 edges and 3 internal halfedges
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    // Allocate new halfedges on heap
    Halfedge *h1 = new Halfedge();
    Halfedge *h2 = new Halfedge();
    Halfedge *h3 = new Halfedge();

    // Allocate new edges on heap
    Edge *e1 = new Edge();
    Edge *e2 = new Edge();
    Edge *e3 = new Edge();

    // Assign a halfedge pointer to the triangle
    t->halfedge = h1;

    // Assign halfedge pointers to point masses
    t->pm1->halfedge = h1;
    t->pm2->halfedge = h2;
    t->pm3->halfedge = h3;

    // Update all halfedge pointers
    h1->edge = e1;
    h1->next = h2;
    h1->pm = t->pm1;
    h1->triangle = t;

    h2->edge = e2;
    h2->next = h3;
    h2->pm = t->pm2;
    h2->triangle = t;

    h3->edge = e3;
    h3->next = h1;
    h3->pm = t->pm3;
    h3->triangle = t;
  }

  // Go back through the cloth mesh and link triangles together using halfedge
  // twin pointers

  // Convenient variables for math
  int num_height_tris = (num_height_points - 1) * 2;
  int num_width_tris = (num_width_points - 1) * 2;

  bool topLeft = true;
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    if (topLeft) {
      // Get left triangle, if it exists
      if (i % num_width_tris != 0) { // Not a left-most triangle
        Triangle *temp = triangles[i - 1];
        t->pm1->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm1->halfedge->twin = nullptr;
      }

      // Get triangle above, if it exists
      if (i >= num_width_tris) { // Not a top-most triangle
        Triangle *temp = triangles[i - num_width_tris + 1];
        t->pm3->halfedge->twin = temp->pm2->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle to bottom right; guaranteed to exist
      Triangle *temp = triangles[i + 1];
      t->pm2->halfedge->twin = temp->pm1->halfedge;
    } else {
      // Get right triangle, if it exists
      if (i % num_width_tris != num_width_tris - 1) { // Not a right-most triangle
        Triangle *temp = triangles[i + 1];
        t->pm3->halfedge->twin = temp->pm1->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle below, if it exists
      if (i + num_width_tris - 1 < 1.0f * num_width_tris * num_height_tris / 2.0f) { // Not a bottom-most triangle
        Triangle *temp = triangles[i + num_width_tris - 1];
        t->pm2->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm2->halfedge->twin = nullptr;
      }

      // Get triangle to top left; guaranteed to exist
      Triangle *temp = triangles[i - 1];
      t->pm1->halfedge->twin = temp->pm2->halfedge;
    }

    topLeft = !topLeft;
  }

  clothMesh->triangles = triangles;
  this->clothMesh = clothMesh;
}
