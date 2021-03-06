#include "bendyruler.h"
#include "database.h"
#include <cmath>
#include <math/math_utils.h>
#include <math/common_math.h>

// parameter defaults
const float OA_BENDYRULER_LOOKAHEAD_DEFAULT = 15.0f;
const float OA_BENDYRULER_RATIO_DEFAULT = 1.5f;
const int16_t OA_BENDYRULER_ANGLE_DEFAULT = 75;
const int16_t OA_BENDYRULER_TYPE_DEFAULT = 1;

const int16_t OA_BENDYRULER_BEARING_INC_XY = 5;            // check every 5 degrees around vehicle
const int16_t OA_BENDYRULER_BEARING_INC_VERTICAL = 90;
const float OA_BENDYRULER_LOOKAHEAD_STEP2_RATIO = 1.0f; // step2's lookahead length as a ratio of step1's lookahead length
const float OA_BENDYRULER_LOOKAHEAD_STEP2_MIN = 2.0f;   // step2 checks at least this many meters past step1's location
const float OA_BENDYRULER_LOOKAHEAD_PAST_DEST = 2.0f;   // lookahead length will be at least this many meters past the destination
const float OA_BENDYRULER_LOW_SPEED_SQUARED = (0.2f * 0.2f);    // when ground course is below this speed squared, vehicle's heading will be used

AP_OABendyRuler::AP_OABendyRuler() 
{ 
    _bearing_prev = FLT_MAX;
}


void AP_OABendyRuler::Init(const PlanningConf &conf)
{
     _lookahead          = conf.BR_lookahead;
     _bendy_ratio        = conf.BR_bendy_ratio;
     _bendy_angle        = conf.BR_bendy_angle;
     _margin_max         = conf.BR_margin_max;

     _max_deviate_angle  = conf.BR_max_deviate_angle;
     _min_near_distance  = conf.BR_min_near_distance;

     _shoreline_safe_pb       = conf.BR_shoreline_safe_pb;
     _shoreline_safe_distance = conf.BR_shoreline_safe_distance;
     _shoreline_safe_angle    = conf.BR_shoreline_safe_angle;
}

// run background task to find best path and update avoidance_results
// returns true and updates origin_new and destination_new if a best path has been found
bool AP_OABendyRuler::update(const Location& current_loc,const Location &origin,const Location& destination, const float ground_course_deg, Location &origin_new, Location &destination_new, bool proximity_only)
{   
    // bendy ruler always sets origin to current_loc
    origin_new = current_loc;

    // calculate bearing and distance to final destination
    const float bearing_to_dest  = current_loc.get_bearing_to(destination) * 0.01f;
    const float distance_to_dest = current_loc.get_distance(destination);
    const float origin_to_dest   = origin.get_bearing_to(destination) * 0.01f;

    // make sure user has set a meaningful value for _lookahead
    _lookahead = std::fmax(_lookahead,1.0f);

    // lookahead distance is adjusted dynamically based on avoidance results
    _current_lookahead = math::Clamp(_current_lookahead, _lookahead * 0.5f, _lookahead);

    // calculate lookahead dist and time for step1.  distance can be slightly longer than
    // the distance to the destination to allow room to dodge after reaching the destination
    const float lookahead_step1_dist = std::fmin(_current_lookahead, distance_to_dest + OA_BENDYRULER_LOOKAHEAD_PAST_DEST);

    // calculate lookahead dist for step2
    const float lookahead_step2_dist = _current_lookahead * OA_BENDYRULER_LOOKAHEAD_STEP2_RATIO;

    bool ret;
    ret = search_xy_path(current_loc, destination, ground_course_deg, destination_new, lookahead_step1_dist, lookahead_step2_dist, origin_to_dest,bearing_to_dest, distance_to_dest, proximity_only);
    return ret;
}

// Search for path in the horizontal directions
bool AP_OABendyRuler::search_xy_path(const Location& current_loc, const Location& destination, float ground_course_deg, Location &destination_new, float lookahead_step1_dist, float lookahead_step2_dist,float origin_to_dest,float bearing_to_dest, float distance_to_dest, bool proximity_only) 
{
    // check OA_BEARING_INC definition allows checking in all directions
    static_assert(360 % OA_BENDYRULER_BEARING_INC_XY == 0, "check 360 is a multiple of OA_BEARING_INC");
    destination_unreachable_   = false;
    destinatoin_near_obstacle_ = false;

    // search in OA_BENDYRULER_BEARING_INC degree increments around the vehicle alternating left
    // and right. For each direction check if vehicle would avoid all obstacles
    float best_bearing = bearing_to_dest;
    float best_bearing_margin = -FLT_MAX;
    bool  have_best_bearing = false;
    float best_margin = -FLT_MAX;
    float best_margin_bearing = best_bearing;

    for (uint8_t i = 0; i <= (170 / OA_BENDYRULER_BEARING_INC_XY); i++) {
        for (uint8_t bdir = 0; bdir <= 1; bdir++) {
            // skip duplicate check of bearing straight towards destination
            if ((i==0) && (bdir > 0)) {
                continue;
            }
            // bearing that we are probing
            const float bearing_delta = i * OA_BENDYRULER_BEARING_INC_XY * (bdir == 0 ? -1.0f : 1.0f);
            const float bearing_test = math::wrap_180(bearing_to_dest + bearing_delta);

            // ToDo: add effective groundspeed calculations using airspeed
            // ToDo: add prediction of vehicle's position change as part of turn to desired heading

            // test location is projected from current location at test bearing
            Location test_loc = current_loc;
            test_loc.offset_bearing(bearing_test, lookahead_step1_dist);

            // calculate margin from obstacles for this scenario
            float margin = calc_avoidance_margin(current_loc, test_loc, proximity_only);
            if (margin > best_margin) {
                best_margin_bearing = bearing_test;
                best_margin = margin;
            }
            if (margin > _margin_max) {
                // this bearing avoids obstacles out to the lookahead_step1_dist
                // now check in there is a clear path in three directions towards the destination
                if (!have_best_bearing) {
                    best_bearing = bearing_test;
                    best_bearing_margin = margin;
                    have_best_bearing = true;
                } else if (fabsf(math::wrap_180(ground_course_deg - bearing_test)) <
                           fabsf(math::wrap_180(ground_course_deg - best_bearing))) {
                    // replace bearing with one that is closer to our current ground course
                    best_bearing = bearing_test;
                    best_bearing_margin = margin;
                }

                // perform second stage test in three directions looking for obstacles
                const float test_bearings[] { 0.0f, 45.0f, -45.0f };
                const float bearing_to_dest2 = test_loc.get_bearing_to(destination) * 0.01f;
                float distance2 = math::Clamp(lookahead_step2_dist, OA_BENDYRULER_LOOKAHEAD_STEP2_MIN, test_loc.get_distance(destination));
                for (uint8_t j = 0; j < ARRAY_SIZE(test_bearings); j++) {
                    float bearing_test2 = math::wrap_180(bearing_to_dest2 + test_bearings[j]);
                    Location test_loc2 = test_loc;
                    test_loc2.offset_bearing(bearing_test2, distance2);

                    // calculate minimum margin to fence and obstacles for this scenario
                    float margin2 = calc_avoidance_margin(test_loc, test_loc2, proximity_only);
                    if (margin2 > _margin_max) {
                        // if the chosen direction is directly towards the destination avoidance can be turned off
                        // i == 0 && j == 0 implies no deviation from bearing to destination 
                        const bool active = (i != 0 || j != 0);
                        float final_bearing = bearing_test;
                        float final_margin = margin;
                        // check if we need ignore test_bearing and continue on previous bearing
                        const bool ignore_bearing_change = resist_bearing_change(destination, current_loc, active, bearing_test, lookahead_step1_dist, margin, _destination_prev,_bearing_prev, final_bearing, final_margin, proximity_only);

                        // all good, now project in the chosen direction by the full distance
                        destination_new = current_loc;
                        destination_new.offset_bearing(final_bearing, std::fmin(distance_to_dest,lookahead_step1_dist));
                        _current_lookahead = std::fmin(_lookahead, _current_lookahead * 1.1f);
                        
                        //printf("final_bearing = %f,bearing_to_dest = %f\n",final_bearing,origin_to_dest);
                        // if final_bearing is too away from bearing_to_dest,we will give up destination
                        if(fabs(math::wrap_180(final_bearing -bearing_to_dest )) >_max_deviate_angle){
                            destination_unreachable_ = true;
                            printf("bendyruler1:: destination is unreachable!\n");
                            return false;
                        }

                        if(check_near_obstacle(current_loc,destination) == true && fabs(math::wrap_180(final_bearing -bearing_to_dest)) >=10.0){
                            destinatoin_near_obstacle_ = true;
                            printf("bendyruler1:: destination is too close obstacle or destination can not arrival\n");
                            return false;
                        }
                        return active;
                    }
                }
            }
        }
    }

    float chosen_bearing;
    float chosen_distance;
    if (have_best_bearing) {
        // none of the directions tested were OK for 2-step checks. Choose the direction
        // that was best for the first step
        chosen_bearing = best_bearing;
        chosen_distance = std::fmax(lookahead_step1_dist+std::fmin(best_bearing_margin,0),0);
        _current_lookahead = std::fmin(_lookahead, _current_lookahead * 1.05f);
    } else {
        // none of the possible paths had a positive margin. Choose
        // the one with the highest margin
        chosen_bearing = best_margin_bearing;
        chosen_distance = std::fmax(lookahead_step1_dist+std::fmin(best_margin,0),0);
        _current_lookahead = std::fmax(_lookahead * 0.5f, _current_lookahead * 0.9f);
    }

    // calculate new target based on best effort
    destination_new = current_loc;
    destination_new.offset_bearing(chosen_bearing, chosen_distance);

   // printf("chosen_bearing = %f,bearing_to_dest = %f\n",chosen_bearing,origin_to_dest);

    // if final_bearing is too away from bearing_to_dest,we will give up destination
    if(fabs(math::wrap_180(chosen_bearing -bearing_to_dest )) >_max_deviate_angle){
        destination_unreachable_ = true;
        printf("bendyruler2:: destination is unreachable!\n");
        return false;
    }

    if(check_near_obstacle(current_loc,destination) == true && fabs(math::wrap_180(chosen_bearing -bearing_to_dest)) >=10.0){
        destinatoin_near_obstacle_ = true;
        printf("bendyruler2:: destination is too close obstacle or destination can not arrival\n");
        return false;
    }
    return true;
}


/*
This function is called when BendyRuler has found a bearing which is obstacles free at atleast lookahead_step1_dist and  then lookahead_step2_dist from the present location
In many situations, this new bearing can be either left or right of the obstacle, and BendyRuler can have a tough time deciding between the two.
It has the tendency to move the vehicle back and forth, if the margin obtained is even slightly better in the newer iteration.
Therefore, this method attempts to avoid changing direction of the vehicle by more than _bendy_angle degrees, 
unless the new margin is atleast _bendy_ratio times better than the margin with previously calculated bearing.
We return true if we have resisted the change and will follow the last calculated bearing. 
*/
bool AP_OABendyRuler::resist_bearing_change(const Location &destination, const Location &current_loc, bool active, float bearing_test, float lookahead_step1_dist, float margin, Location &prev_dest, float &prev_bearing, float &final_bearing, float &final_margin, bool proximity_only) const
{      
    bool resisted_change = false;
    // see if there was a change in destination, if so, do not resist changing bearing 
    bool dest_change = false;
    if (!destination.same_latlon_as(prev_dest)) {
        dest_change = true;
        prev_dest = destination;
    }
                        
    // check if we need to resist the change in direction of the vehicle. If we have a clear path to destination, go there any how  
    if (active && !dest_change && math::IsPositive(_bendy_ratio)) { 
        // check the change in bearing between freshly calculated and previous stored BendyRuler bearing
        if ((fabsf(math::wrap_180(prev_bearing-bearing_test)) > _bendy_angle) && (!math::IsZero(prev_bearing-FLT_MAX))) {
            // check margin in last bearing's direction
            Location test_loc_previous_bearing = current_loc;
            test_loc_previous_bearing.offset_bearing(math::wrap_180(prev_bearing), lookahead_step1_dist);
            float previous_bearing_margin = calc_avoidance_margin(current_loc,test_loc_previous_bearing, proximity_only);

            if (margin < (_bendy_ratio * previous_bearing_margin)) {
                // don't change direction abruptly. If margin difference is not significant, follow the last direction
                final_bearing = prev_bearing;
                final_margin  = previous_bearing_margin;
                resisted_change = true;
            } 
        } 
    } else {
        // reset stored bearing if BendyRuler is not active or if WP has changed for unnecessary resistance to path change
        prev_bearing = FLT_MAX;
    }
    if (!resisted_change) {
        // we are not resisting the change, hence store BendyRuler's presently calculated bearing for future iterations
        prev_bearing = bearing_test;
    }

    return resisted_change;
}

// calculate minimum distance between a segment and any obstacle
float AP_OABendyRuler::calc_avoidance_margin(const Location &start, const Location &end, bool proximity_only) const
{
    float margin_min = FLT_MAX;

    float latest_margin;
    
    if (calc_margin_from_object_database(start, end, latest_margin)) {
        margin_min = std::fmin(margin_min, latest_margin);
    }
    
    if (proximity_only) {
        // only need margin from proximity data
        return margin_min;
    }
    
    // return smallest margin from any obstacle
    return margin_min;
}


extern LOC::Location ekf_origin_;
// calculate minimum distance between a path and proximity sensor obstacles
// on success returns true and updates margin
bool AP_OABendyRuler::calc_margin_from_object_database(const Location &start, const Location &end, float &margin) const
{
    // exit immediately if db is empty
    AP_OADatabase *oaDb = AP::oadatabase();
    if (oaDb == nullptr || !oaDb->healthy()) {
        printf("bendyruler::OADatabase is unhealthy!\n");
        return false;
    }

    // convert start and end to offsets (in cm) from EKF origin
    Vector2f start_NEU,end_NEU;
    if (!start.get_vector_xy_from_origin_NE(start_NEU,ekf_origin_) || !end.get_vector_xy_from_origin_NE(end_NEU,ekf_origin_)) {
        return false;
    }
    if (start_NEU == end_NEU) {
        return false;
    }

    // check each obstacle's distance from segment
    float smallest_margin = FLT_MAX;
    for (uint16_t i=0; i<oaDb->database_count(); i++) {
        const AP_OADatabase::OA_DbItem& item = oaDb->get_item(i);
        const Vector2f point_cm {item.pos.x * 100.0f,item.pos.y *100.0f};
        // margin is distance between line segment and obstacle minus obstacle's radius
        const float m = Vector2f::closest_distance_between_line_and_point(start_NEU, end_NEU, point_cm) * 0.01f - item.radius;
        if (m < smallest_margin) {
            smallest_margin = m;
        }
    }

    // return smallest margin
    if (smallest_margin < FLT_MAX) {
        margin = smallest_margin;
        return true;
    }

    return false;
}


bool AP_OABendyRuler::check_near_obstacle(const Location &start, const Location &end) const
{
    // exit immediately if db is empty
    AP_OADatabase *oaDb = AP::oadatabase();
    if (oaDb == nullptr || !oaDb->healthy()) {
        printf("bendyruler::OADatabase is unhealthy!\n");
        return true;
    }

    // check each obstacle's distance from segment
    float m_smallest_margin = FLT_MAX;
    float n_smallest_margin = FLT_MAX;
    bool res = false;

    const int16_t N = _shoreline_safe_angle/OA_BENDYRULER_BEARING_INC_XY;
    int16_t obstacle_distribution[2*N+1];
    memset(obstacle_distribution,0,sizeof(obstacle_distribution));
    float smallest_distance = FLT_MAX;

    for (uint16_t i=0; i<oaDb->database_count(); i++) {
        const AP_OADatabase::OA_DbItem& item = oaDb->get_item(i);
        Location obs(Vector3f(item.pos.x * 100.0f,item.pos.y *100.0f,0.0f),ekf_origin_);

        // margin is distance between line segment and obstacle minus obstacle's radius
        const float m = start.get_distance(obs);
        const float n = end.get_distance(obs);
        if(m < _min_near_distance &&   n <_min_near_distance){
            res = true;
        }

        // fill obstacle_distribution array
        const float theta  = math::wrap_180(start.get_bearing_to(obs) *0.01f - start.get_bearing_to(end)*0.01f);
        if(std::fabs(theta) < _shoreline_safe_angle + (OA_BENDYRULER_BEARING_INC_XY * 0.5f)){
            int16_t  sector = (theta + (OA_BENDYRULER_BEARING_INC_XY * 0.5f))/OA_BENDYRULER_BEARING_INC_XY;
            if(sector > N){sector = N;}
            if(sector < -N){sector = -N;}
            obstacle_distribution[sector + N] ++;
            if(smallest_distance > m){
                smallest_distance = m;
            }
        }
    }

    // check obstacle_distribution array
    bool      ret = false;
    uint16_t  no_obstacle_size = 0;
    for(uint16_t k = 0; k<2*N+1;k++){
        if(obstacle_distribution[k] == 0){
            no_obstacle_size++;
        }
    }
    if(no_obstacle_size <= (_shoreline_safe_pb * (2*N+1)) && smallest_distance <= _shoreline_safe_distance){ ret = true;}

   return (ret == true || res == true)?(true):(false);
}