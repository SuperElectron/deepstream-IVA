#pragma once

#include <uuid/uuid.h>
#include <chrono>
#include <string>
#include "date/tz.h"
#include <algorithm>

/**
 * @namespace processUtils
 * @brief utilities for Processing module
 */
namespace processUtils
{

/**
 * @struct BoundingBox
 * @brief bounding box holder for IOU calculation
 * @var xmin
 * placeholder for bounding box value
 * @var xmax
 * placeholder for bounding box value
 * @var ymin
 * placeholder for bounding box value
 * @var ymax
 * placeholder for bounding box value
 */

struct BoundingBox {
  float xmin;
  float xmax;
  float ymin;
  float ymax;
};

/**
 * @brief converts millimeters to feet and inches
 *
 * @param millimeters the distance value in millimeters
 * @return std::string      value that in form of X'Y''
 */
inline std::string convert_mm_to_feet_and_inches(double millimeters) {
  // There are 25.4 millimeters in an inch.
  int inches = static_cast<int>(millimeters / 25.4);

  // There are 12 inches in a foot.
  int feet = inches / 12;

  return std::to_string(feet) + " ' " + std::to_string(inches % 12) + " ''";
}

/**
 * @brief generates a unix timestamp
 *
 * @return  long        unix timestamp (milliseconds since 1970-01-01)
 *
 */
inline long generate_ts_epoch()
{
    auto p1 = std::chrono::system_clock::now();
    auto epoch_ts = std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch()).count();
    return epoch_ts;
}

/**
 * @brief generates a human readable timestamp
 *
 * @return  std::string  generates a human readable timestamp (e.g. 2023-05-01T14:22:55.954:PST )
 */
inline std::string generate_timestamp()
{
    // export TZ=$(cat /etc/timezone) in terminal before running the application!
    char *TZ = getenv("TZ");
    auto current_time = std::chrono::system_clock::now();
    auto myzone_time = date::make_zoned(TZ, floor<std::chrono::milliseconds>(current_time));
    auto t_formatted = date::format("%Y-%m-%dT%H:%M:%S:%Z", myzone_time);
    return t_formatted;
}

/**
 * @brief   generate a 32 bit uuid
 *
 * @return std::string  a new uuid
 */
inline std::string generate_uuid()
{
    uuid_t id;
    uuid_generate(id);
    char *uuid_string = new char[100];
    uuid_unparse(id, uuid_string);
    return std::string(uuid_string);
}


/**
 * @brief calculate the intersection over union between two bounding boxes
 * @reference https://machinelearningspace.com/intersection-over-union-iou-a-comprehensive-guide/
 *
 * @param bbox1 the bounding box of ground truth (detections on this module)
 * @param bbox2 the bounding box to be compared (spyder detections)
 * @return int the IOU value ranging between 0-100
 */
inline int calculate_iou(BoundingBox bbox1, BoundingBox bbox2)
{
    float xmin_i = std::max(bbox1.xmin, bbox2.xmin);
    float xmax_i = std::min(bbox1.xmax, bbox2.xmax);

    float ymin_i = std::max(bbox1.ymin, bbox2.ymin);
    float ymax_i = std::min(bbox1.ymax, bbox2.ymax);

    float intersection_area = (xmax_i - xmin_i + (float) 1) * (ymax_i - ymin_i + (float) 1);

    float area_1 = (bbox1.xmax - bbox1.xmin + (float) 1 ) * (bbox1.ymax - bbox1.ymin + (float) 1);
    float area_2 = (bbox2.xmax - bbox2.xmin + (float) 1 ) * (bbox2.ymax - bbox2.ymin + (float) 1);
    float denominator = area_1 + area_2 - intersection_area;

    float iou = intersection_area/denominator;


    // error checking
    float width_i = xmax_i - xmin_i;
    float height_i = ymax_i - ymin_i;
    if(width_i < 0)
    {
      VLOG(DEEP) << "BBox doesn't overlap (width_i < 0)";
      return 0;
    }
    if(height_i < 0)
    {
      VLOG(DEEP) << "BBox doesn't overlap (height_i < 0)";
      return 0;
    }
    return (int) iou*100;
}

/**
 * @brief prints out Gstreamer buffer 'caps' fields
 *
 * @param field the key name of the value
 * @param value the value of the caps
 * @param prefix formatting for the print statement
 * @return true  boolean as return from the function call
 */
inline static gboolean print_field(GQuark field, const GValue *value, gpointer prefix)
{
    gchar *str = gst_value_serialize(value);

    g_print("%s  %15s: %s\n", (gchar *)prefix, g_quark_to_string(field), str);
    g_free(str);
    return TRUE;
}

} // namespace processUtils