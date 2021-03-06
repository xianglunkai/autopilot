#pragma once

#include <math/RingBuffer.h>
#include <mutex>
#include <math/vector3.h>
#include "planning_conf.h"

#define MAVLINK_COMM_NUM_BUFFERS 1

class AP_OADatabase {
public:

    AP_OADatabase();

    /* Do not allow copies */
    AP_OADatabase(const AP_OADatabase &other) = delete;
    AP_OADatabase &operator=(const AP_OADatabase&) = delete;

    // get singleton instance
    static AP_OADatabase *get_singleton() {
        return _singleton;
    }

    enum OA_DbItemImportance {
        Low, Normal, High
    };

    struct OA_DbItem {
        Vector3f pos;             // position of the object as an offset in meters from the EKF origin
        uint32_t timestamp_ms;    // system time that object was last updated
        float radius;             // objects radius in meters
        uint8_t send_to_gcs;      // bitmask of mavlink comports to which details of this object should be sent
        OA_DbItemImportance importance;
    };

    void init(const PlanningConf &conf);
    void update();

    // push an object into the database.  Pos is the offset in meters from the EKF origin, angle is in degrees, distance in meters
    void queue_push(const  Vector3f &pos, uint32_t timestamp_ms, float distance);

    // returns true if database is healthy
    bool healthy() const { return (_queue.items != nullptr) && (_database.items != nullptr); }

    // fetch an item in database. Undefined result when i >= _database.count.
    const OA_DbItem& get_item(uint32_t i) const { return _database.items[i]; }

    // get number of items in the database
    uint16_t database_count() const { return _database.count; }

    // empty queue and try and put into database. Return true if there's more work to do
    bool process_queue();

    // send ADSB_VEHICLE mavlink messages
    void send_adsb_vehicle(void);

 
private:

    // initialise
    void init_queue();
    void init_database();

    // database item management
    void database_item_add(const OA_DbItem &item);
    void database_item_refresh(const uint16_t index, const uint32_t timestamp_ms, const float radius);
    void database_item_remove(const uint16_t index);
    void database_items_remove_all_expired();

    // get bitmask of gcs channels item should be sent to based on its importance
    // returns 0xFF (send to all channels) if should be sent or 0 if it should not be sent
    uint8_t get_send_to_gcs_flags(const OA_DbItemImportance importance);

    // returns true if database item "index" is close to "item"
    bool is_close_to_item_in_database(const uint16_t index, const OA_DbItem &item) const;

    // enum for use with _OUTPUT parameter
    enum class OA_DbOutputLevel {
        OUTPUT_LEVEL_DISABLED = 0,
        OUTPUT_LEVEL_SEND_HIGH = 1,
        OUTPUT_LEVEL_SEND_HIGH_AND_NORMAL = 2,
        OUTPUT_LEVEL_SEND_ALL = 3
    };

    struct {
        ObjectBuffer<OA_DbItem> *items;                     // thread safe incoming queue of points from proximity sensor to be put into database
        uint16_t        size;                               // cached value of _queue_size_param.
        std::mutex   sem;                                   // semaphore for multi-thread use of queue
    } _queue;
    
    float dist_to_radius_scalar;                            // scalar to convert the distance and beam width to an object radius

    struct {
        OA_DbItem       *items;                             // array of objects in the database
        uint16_t        count;                              // number of objects in the items array
        uint16_t        size;                               // cached value of _database_size_param that sticks after initialized
    } _database;

    uint16_t _next_index_to_send[MAVLINK_COMM_NUM_BUFFERS]; // index of next object in _database to send to GCS
    uint16_t _highest_index_sent[MAVLINK_COMM_NUM_BUFFERS]; // highest index in _database sent to GCS
    uint32_t _last_send_to_gcs_ms[MAVLINK_COMM_NUM_BUFFERS];// system time that send_adsb_vehicle was last called

    static AP_OADatabase *_singleton;

private:
    // parameters
    int16_t      _queue_size_param{360};                       // queue size
    int16_t      _database_size_param{720};                    // db size
    int16_t      _database_expiry_seconds{5};                  // objects expire after this timeout
    int16_t      _output_level{3};                             // controls which items should be sent to GCS
    float        _beam_width{10.0};                            // beam width used when converting lidar readings to object radius
    float        _radius_min{1.5f};                            // objects minimum radius (in meters)
    float        _dist_max{50.0f};                             // objects maximum distance (in meters)
    float        _min_alt{0.0f};                               // OADatabase minimum vehicle height check (in meters)
};

namespace AP {
    AP_OADatabase *oadatabase();
};


