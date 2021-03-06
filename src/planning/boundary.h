#pragma once

#include <filter/low_pass_filter.h>
#include <math/vector3.h>


#define PROXIMITY_NUM_SECTORS         8       // number of sectors
#define PROXIMITY_NUM_LAYERS          5       // num of layers in a sector
#define PROXIMITY_MIDDLE_LAYER        2       // middle layer
#define PROXIMITY_PITCH_WIDTH_DEG     30      // width between each layer in degrees
#define PROXIMITY_SECTOR_WIDTH_DEG    (360.0f/PROXIMITY_NUM_SECTORS)   // width of sectors in degrees
#define PROXIMITY_BOUNDARY_DIST_MIN   0.6f    // minimum distance for a boundary point.  This ensures the object avoidance code doesn't think we are outside the boundary.
#define PROXIMITY_BOUNDARY_DIST_DEFAULT 100   // if we have no data for a sector, boundary is placed 100m out
#define PROXIMITY_FILT_RESET_TIME     1000    // reset filter if last distance was pushed more than this many ms away

class AP_Proximity_Boundary_3D 
{ 
public:
    // constructor. This incorporates initialisation as well.
	AP_Proximity_Boundary_3D();

    // stores the layer and sector as a single object to access and modify the 3-D boundary
    // Objects of this class are used temporarily to modify the boundary, i,e they are not persistant or stored anywhere  
    class Face
    {
    public:

	    // constructor, invalidate id and distance
	    Face() { layer = sector = 255; }
	    Face(uint8_t _layer, uint8_t _sector) { layer = _layer; sector = _sector; }

	    // return true if face has valid layer and sector values
	    bool valid() const { return ((layer < PROXIMITY_NUM_LAYERS) && (sector < PROXIMITY_NUM_SECTORS)); }

	    // comparison operator
	    bool operator ==(const Face &other) const { return ((layer == other.layer) && (sector == other.sector)); }
	    bool operator !=(const Face &other) const { return ((layer != other.layer) || (sector != other.sector)); }

        uint8_t layer;  // vertical "steps" on the 3D Boundary. 0th layer is the bottom most layer, 1st layer is 30 degrees above (in body frame) and so on
        uint8_t sector; // horizontal "steps" on the 3D Boundary. 0th sector is directly in front of the vehicle. Each sector is 45 degrees wide.
    };

    // returns face corresponding to the provided yaw and (optionally) pitch
    // pitch is the vertical body-frame angle (in degrees) to the obstacle (0=directly ahead, 90 is above the vehicle?)
    // yaw is the horizontal body-frame angle (in degrees) to the obstacle (0=directly ahead of the vehicle, 90 is to the right of the vehicle)
    Face get_face(float pitch, float yaw) const;
    Face get_face(float yaw) const { return get_face(0, yaw); }

    // Set the actual body-frame angle(yaw), pitch, and distance of the detected object.
    // This method will also mark the sector and layer to be "valid",
    // This distance can then be used for Obstacle Avoidance
    // Assume detected obstacle is horizontal (zero pitch), if no pitch is passed
    void set_face_attributes(const Face &face, float pitch, float yaw, float distance);
    void set_face_attributes(const Face &face, float yaw, float distance) { set_face_attributes(face, 0, yaw, distance); }

    // add a distance to the boundary if it is shorter than any other provided distance since the last time the boundary was reset
    // pitch and yaw are in degrees, distance is in meters
    void add_distance(float pitch, float yaw, float distance);
    void add_distance(float yaw, float distance) { add_distance(0, yaw, distance); }

    // update boundary points used for simple avoidance based on a single sector and pitch distance changing
    //   the boundary points lie on the line between sectors meaning two boundary points may be updated based on a single sector's distance changing
    //   the boundary point is set to the shortest distance found in the two adjacent sectors, this is a conservative boundary around the vehicle
    void update_boundary(const Face &face);

    // update middle layer boundary points
    void update_middle_boundary();

    // reset boundary.  marks all distances as invalid
    void reset();

    // Reset this location, specified by Face object, back to default
    // i.e Distance is marked as not-valid
    void reset_face(const Face &face);

    // get distance for a face.  returns true on success and fills in distance argument with distance in meters
    bool get_distance(const Face &face, float &distance) const;

    // Get the total number of obstacles 
    uint8_t get_obstacle_count() const;

    // Returns a body frame vector (in cm) to an obstacle
    // False is returned if the obstacle_num provided does not produce a valid obstacle
    bool get_obstacle(uint8_t obstacle_num, Vector3f& vec_to_boundary) const;

    // Returns a body frame vector (in cm) nearest to obstacle, in betwen seg_start and seg_end
    // FLT_MAX is returned if the obstacle_num provided does not produce a valid obstacle
    float distance_to_obstacle(uint8_t obstacle_num, const Vector3f& seg_start, const Vector3f& seg_end, Vector3f& closest_point) const;

    // get distance and angle to closest object (used for pre-arm check)
    //   returns true on success, false if no valid readings
    bool get_closest_object(float& angle_deg, float &distance) const;

    // get number of objects, angle and distance - used for non-GPS avoidance
    uint8_t get_horizontal_object_count() const;
    bool get_horizontal_object_angle_and_distance(uint8_t object_number, float& angle_deg, float &distance) const;

    // get number of layers
    uint8_t get_num_layers() const { return PROXIMITY_NUM_LAYERS; }

    // get raw and filtered distances in 8 directions per layer.
   // bool get_layer_distances(uint8_t layer_number, float dist_max, AP_Proximity::Proximity_Distance_Array &prx_dist_array, AP_Proximity::Proximity_Distance_Array &prx_filt_dist_array) const;

    // sectors
    static_assert(PROXIMITY_NUM_SECTORS == 8, "PROXIMITY_NUM_SECTOR must be 8");
    const uint16_t _sector_middle_deg[PROXIMITY_NUM_SECTORS] {0, 45, 90, 135, 180, 225, 270, 315};    // middle angle of each sector
    // layers
    static_assert(PROXIMITY_NUM_LAYERS == 5, "PROXIMITY_NUM_LAYERS must be 5");
    const int16_t _pitch_middle_deg[PROXIMITY_NUM_LAYERS] {-60, -30, 0, 30, 60};

private:

    // initialise the boundary and sector_edge_vector array used for object avoidance
    void init();

    // get the next sector which is CW to the passed sector
    uint8_t get_next_sector(uint8_t sector) const {return ((sector >= PROXIMITY_NUM_SECTORS-1) ? 0 : sector+1); }
    
    // get the prev sector which is CCW to the passed sector 
    uint8_t get_prev_sector(uint8_t sector) const {return ((sector <= 0) ? PROXIMITY_NUM_SECTORS-1 : sector-1); }

    // Converts obstacle_num passed from avoidance library into appropriate face of the boundary
    // Returns false if the face is invalid
    // "update_boundary" method manipulates two sectors ccw and one sector cw from any valid face.
    // Any boundary that does not fall into these manipulated faces are useless, and will be marked as false
    // The resultant is packed into a Boundary Location object and returned by reference as "face"
    bool convert_obstacle_num_to_face(uint8_t obstacle_num, Face& face) const WARN_IF_UNUSED;

    // Apply low pass filter on the raw distance
    void set_filtered_distance(const Face &face, float distance);

    // Return filtered distance for the passed in face
    bool get_filtered_distance(const Face &face, float &distance) const;

    Vector3f _sector_edge_vector[PROXIMITY_NUM_LAYERS][PROXIMITY_NUM_SECTORS];
    Vector3f _boundary_points[PROXIMITY_NUM_LAYERS][PROXIMITY_NUM_SECTORS];

    float _angle[PROXIMITY_NUM_LAYERS][PROXIMITY_NUM_SECTORS];          // yaw angle in degrees to closest object within each sector and layer
    float _pitch[PROXIMITY_NUM_LAYERS][PROXIMITY_NUM_SECTORS];          // pitch angle in degrees to the closest object within each sector and layer
    float _distance[PROXIMITY_NUM_LAYERS][PROXIMITY_NUM_SECTORS];       // distance to closest object within each sector and layer
    bool _distance_valid[PROXIMITY_NUM_LAYERS][PROXIMITY_NUM_SECTORS];  // true if a valid distance received for each sector and layer
    uint32_t _last_update_ms[PROXIMITY_NUM_LAYERS][PROXIMITY_NUM_SECTORS]; // time when distance was last updated
    LowPassFilterFloat _filtered_distance[PROXIMITY_NUM_LAYERS][PROXIMITY_NUM_SECTORS]; // low pass filter
};

